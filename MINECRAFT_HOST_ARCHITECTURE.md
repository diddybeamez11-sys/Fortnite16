# E-Client Minecraft-hosted architecture

This revision changes the design from an E-Client-created Minecraft process to a
**Minecraft-hosted native library**. The native library is intended to be loaded
by the actual Minecraft process. It must not load `libminecraftpe.so` into an
unrelated E-Client process and call that an attachment.

## Bootstrap

1. The modified Minecraft APK starts its normal Activity/Application.
2. A small loader component calls `System.loadLibrary("eclient_host")`.
3. `JNI_OnLoad` starts `MinecraftHost`.
4. `MinecraftHost` waits for the real `libminecraftpe.so` mapping.
5. `GameBridge` records the build ID.
6. Only a verified version profile may enable game-specific native integration.
7. GUI and modules are initialized after the host is proven to be inside the
   Minecraft process.

## Version rule

Never copy Apollon 1.21.111 offsets into another Minecraft version. The uploaded
1.21.131 IDA database is a research input, not proof that any offset works.
A corresponding 1.21.131 ARM64 `libminecraftpe.so` is required before adding
version-specific offsets/signatures.

## GitHub research used

Public projects were inspected for architecture ideas, including Apollon's
native ImGui/Dobby/KittyMemory structure and the Aprism research around Bedrock
loaders. Their code is not copied wholesale into E-Client.

## Current milestone

The native host library now has a real `JNI_OnLoad` entry point and a closed-fail
Minecraft runtime gate. Rendering/game-object integration remains disabled until
we have the exact target binary/profile.
