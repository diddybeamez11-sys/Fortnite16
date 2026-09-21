# E-Client native documentation

- `NATIVE_RUNTIME_1.21.80.md` — standalone Android native runtime target and exact 1.21.80.3 ARM64 fingerprint.

## 1.21.80.3 development target

`eclient32` targets the verified Minecraft Bedrock 1.21.80.3 ARM64 payload. The native host validates the GNU Build ID before enabling its render/input bootstrap. The in-game GUI can be opened with **Volume Up** after the native host is loaded. The gameplay module list is a real module registry, but modules that require player/entity/camera addresses remain disabled until their exact 1.21.80 mapping is verified; this prevents stale offsets from corrupting the Minecraft process.
