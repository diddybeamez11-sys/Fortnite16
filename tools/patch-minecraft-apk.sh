#!/usr/bin/env bash
set -euo pipefail

# Usage:
#   patch-minecraft-apk.sh minecraft.apk libeclient_host.so output.apk
# Requires: apktool, zipalign, apksigner, python3

INPUT_APK=${1:?input Minecraft APK required}
HOST_SO=${2:?libeclient_host.so required}
OUTPUT_APK=${3:?output APK required}
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

apktool d -f "$INPUT_APK" -o "$WORK/mc" >/dev/null
mkdir -p "$WORK/mc/smali/com/rubidiumclient/host"
cp "$(dirname "$0")/minecraft-host-loader.smali" "$WORK/mc/smali/com/rubidiumclient/host/EClientLoaderProvider.smali"
mkdir -p "$WORK/mc/lib/arm64-v8a"
cp "$HOST_SO" "$WORK/mc/lib/arm64-v8a/libeclient_host.so"

python3 - "$WORK/mc/AndroidManifest.xml" <<'PY'
import sys
import xml.etree.ElementTree as ET
p=sys.argv[1]
ET.register_namespace('android', 'http://schemas.android.com/apk/res/android')
root=ET.parse(p)
manifest=root.getroot()
app=manifest.find('application')
if app is None:
    raise SystemExit('application element missing')
ns='{http://schemas.android.com/apk/res/android}'
name='com.rubidiumclient.host.EClientLoaderProvider'
for provider in app.findall('provider'):
    if provider.get(ns+'name') == name:
        break
else:
    provider=ET.Element('provider')
    provider.set(ns+'name', name)
    provider.set(ns+'exported', 'false')
    provider.set(ns+'authorities', 'com.rubidiumclient.eclientloader')
    app.append(provider)
root.write(p, encoding='utf-8', xml_declaration=True)
PY

apktool b "$WORK/mc" -o "$WORK/unsigned.apk" >/dev/null
zipalign -f 4 "$WORK/unsigned.apk" "$WORK/aligned.apk" >/dev/null
if [[ -n "${ANDROID_HOME:-}" && -f "$ANDROID_HOME/build-tools/35.0.0/apksigner" ]]; then
  APKSIGNER="$ANDROID_HOME/build-tools/35.0.0/apksigner"
else
  APKSIGNER=$(command -v apksigner)
fi
if [[ -z "$APKSIGNER" ]]; then
  echo 'apksigner not found' >&2
  exit 2
fi
KEYSTORE=${ECLIENT_KEYSTORE:-"$HOME/.android/debug.keystore"}
if [[ ! -f "$KEYSTORE" ]]; then
  keytool -genkeypair -v -keystore "$KEYSTORE" -storepass android -alias androiddebugkey \
    -keypass android -keyalg RSA -keysize 2048 -validity 10000 \
    -dname 'CN=Android Debug,O=Android,C=US' >/dev/null 2>&1
fi
"$APKSIGNER" sign --ks "$KEYSTORE" --ks-pass pass:android --key-pass pass:android \
  --ks-key-alias androiddebugkey --out "$OUTPUT_APK" "$WORK/aligned.apk" >/dev/null
"$APKSIGNER" verify --verbose "$OUTPUT_APK" >/dev/null
echo "Patched APK: $OUTPUT_APK"
echo "Note: this replaces the APK signature; uninstall the signed original before installing the patched build if Android rejects the signature change."
