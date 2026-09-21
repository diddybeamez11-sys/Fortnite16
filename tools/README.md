# Minecraft APK host patch

`minecraft-host-loader.smali` is the tiny ContentProvider used by the eventual
repackaging step. Android initializes providers before the launch Activity, so
loading `eclient_host` here puts the native library in the **Minecraft process**
rather than starting a fake secondary Minecraft process.

The final patcher should:

- decode the user's own Minecraft APK with apktool;
- add the smali class under `smali/com/rubidiumclient/host/`;
- add the provider to the decoded Minecraft manifest;
- copy `lib/arm64-v8a/libeclient_host.so`;
- rebuild and sign the resulting APK.

Do not redistribute Mojang's APK or assets. The input APK should be supplied by
the user and the patched APK should be used only where permitted.
