#!/usr/bin/env bash
set -euo pipefail

# Build E-Client's native host, then inject it into a user-supplied Minecraft APK.
# This does not redistribute Minecraft; the APK is supplied by the user.
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
INPUT_APK="${1:?Usage: build-and-patch.sh <minecraft-1.21.80-arm64.apk> [output.apk]}"
OUTPUT_APK="${2:-$ROOT/eclient-minecraft-1.21.80-arm64.apk}"

EXPECTED_SHA="95c01125cf43942f55020c9015bdbe76a8b986d723de9a84869e25190a8d9c72"
ACTUAL_SHA="$(sha256sum "$INPUT_APK" | awk '{print $1}')"
if [[ "$ACTUAL_SHA" != "$EXPECTED_SHA" ]]; then
  echo "Refusing to patch: APK SHA-256 does not match the verified 1.21.80.3 ARM64 input." >&2
  echo "Expected: $EXPECTED_SHA" >&2
  echo "Actual:   $ACTUAL_SHA" >&2
  exit 3
fi

"$ROOT/gradlew" --no-daemon :app:assembleRelease
# AGP puts release CMake output under cxx/RelWithDebInfo (older setups: cxx/Release).
HOST_SO="$(find "$ROOT/app/build/intermediates/cxx" -type f -path "*/obj/arm64-v8a/libeclient_host.so" \
  \( -path "*/RelWithDebInfo/*" -o -path "*/Release/*" \) -print -quit 2>/dev/null || true)"
if [[ -z "$HOST_SO" || ! -f "$HOST_SO" ]]; then
  echo "libeclient_host.so was not produced. Build the release native target first." >&2
  exit 4
fi

"$ROOT/tools/patch-minecraft-apk.sh" "$INPUT_APK" "$HOST_SO" "$OUTPUT_APK"
echo "Ready: $OUTPUT_APK"
