# Minecraft 1.21.80.3 ARM64 profile

This project has a verified Android ARM64 Minecraft payload available during development.

- Version: **1.21.80.3**
- ABI: **arm64-v8a**
- `libminecraftpe.so`: **247,512,600 bytes**
- `libminecraftpe.so` SHA-256: `84cc649545f95420212038dd483896d5d53507e25a309e68bd038b0256a976ae`
- GNU Build ID: `665d33595ddec90fe7347a45cd4da65e6f0871aa`
- APK SHA-256 used by the build script: `95c01125cf43942f55020c9015bdbe76a8b986d723de9a84869e25190a8d9c72`

The native library exports `Java_com_mojang_minecraftpe_MainActivity_nativeKeyHandler` at ELF value `0xCA9BDF8`. The E-Client input layer resolves this by symbol name at runtime rather than hard-coding that address.

The project deliberately does **not** treat strings or old client offsets as proof of a gameplay structure. Player/entity/movement hooks require exact 1.21.80 signatures or validated symbols before memory writes are enabled.
