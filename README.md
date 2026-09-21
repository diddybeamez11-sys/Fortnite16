# E-Client

E-Client is being rebuilt as a **Minecraft-hosted native client** instead of an
external overlay or a fake secondary Minecraft process.

## Current direction

The new native host library (`libeclient_host.so`) is designed to be loaded by
the actual Minecraft process. Its `JNI_OnLoad` starts a closed-fail bootstrap
that waits for `libminecraftpe.so`, verifies the known runtime profile, and only
then enables the in-game GUI layer.

The `tools/patch-minecraft-apk.sh` script contains the repackaging mechanism for
a user's own Minecraft APK. It injects a tiny Android `ContentProvider` loader,
adds `libeclient_host.so`, rebuilds and signs the APK.

## Important

The uploaded Apollon project is used as an architectural reference for native
ImGui integration, Android lifecycle integration, and ARM64 native tooling. Its
version-specific offsets are **not** reused for another Minecraft build.

The uploaded Minecraft Education 1.21.131 IDA database is treated as analysis
input. A matching 1.21.131 ARM64 `libminecraftpe.so` is still required before
1.21.131-specific offsets/signatures are enabled.

Until that exact binary/profile is verified, unknown versions fail closed rather
than crashing from stale offsets.

## Memory-client bootstrap (eclient32)

`eclient32` is the first revision that treats E-Client as an **in-process memory client** rather than a launcher or separate overlay process. The host library is loaded by an injected `ContentProvider` inside the Minecraft process, waits for the real `libminecraftpe.so`, validates the exact verified 1.21.80.3 ARM64 Build ID, installs the render hook, and exposes native module-base/range scanning primitives.

The project intentionally does not ship Minecraft. Use `tools/build-and-patch.sh` with your own verified Minecraft APK. The script checks the known 1.21.80.3 APK SHA-256 before patching.

## 1.21.80.3 verified workflow

1. Supply your own Minecraft 1.21.80.3 ARM64 APK to `tools/build-and-patch.sh`.
2. The script verifies the exact APK SHA-256 before patching.
3. Gradle/NDK builds `libeclient_host.so`.
4. `tools/patch-minecraft-apk.sh` inserts the host library and loader provider into that APK and signs the resulting user-owned build.
5. Minecraft remains the host application/process; E-Client does not launch a synthetic copy of `libminecraftpe.so` in a second process.
6. Once loaded, the native host validates the 1.21.80.3 Build ID, installs the render hook, and installs the `nativeKeyHandler` hotkey hook. Press **Volume Up** to show/hide the E-Client GUI.
