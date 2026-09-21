# E-Client memory-client start

This revision changes the native runtime from a launcher/side-process model to an **in-process host** model.

## Runtime path

1. A user supplies their own Minecraft ARM64 APK.
2. `tools/patch-minecraft-apk.sh` injects `libeclient_host.so` and an early `ContentProvider` loader.
3. Android creates the provider in Minecraft's own process.
4. `JNI_OnLoad` starts `MinecraftHost`.
5. The host waits for `libminecraftpe.so`, validates the exact 1.21.80.3 build ID, and installs the EGL render hook.
6. The native memory layer exposes module base/ranges and pattern scanning for version-specific work.
7. Memory-backed modules remain fail-closed until offsets/patterns have been verified against the exact ARM64 binary.

## Verified 1.21.80.3 input

- APK SHA-256: `95c01125cf43942f55020c9015bdbe76a8b986d723de9a84869e25190a8d9c72`
- `libminecraftpe.so` Build ID: `665d33595ddec90fe7347a45cd4da65e6f0871aa`
- ABI: `arm64-v8a`

The 1.21.131 files previously supplied to the project are desktop/IDA artifacts, not the Android ARM64 `libminecraftpe.so`, so this revision deliberately does not pretend that 1.21.131 offsets are verified.
