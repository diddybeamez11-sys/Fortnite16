# Fixes applied (static review; not compiled here - no Gradle/NDK/network in the sandbox)

Build
- CMake: removed `-Werror` (it applied to vendored ImGui/Dobby/And64 sources); vendored files now built with `-w`.
- CMake: defined `IMGUI_IMPL_OPENGL_ES3` (was falling back to the desktop-GL loader) and linked `libandroid` + `GLESv3`
  (ImGui's Android backend needs AInputEvent_* / ANativeWindow_* -> undefined symbols at link time).
- Gradle: pinned `ndkVersion = 28.2.13676358` to match CI.
- Gradle: release build falls back to the debug key if no release keystore exists (assembleRelease used by tools/build-and-patch.sh).
- gradle.properties: heap 2g -> 4g (R8 + Compose).
- proguard-rules.pro: rules pointed at `com.nexoraclient.*`; real package is `com.rubidiumclient`.
- tools/build-and-patch.sh: looked for the .so under `cxx/Release`; AGP writes `cxx/RelWithDebInfo/.../obj/arm64-v8a`.
- tools/patch-minecraft-apk.sh: ElementTree registered the android namespace as the default prefix, which produced an invalid manifest.

Runtime (in-process host)
- `JNI_OnLoad` was hidden by `-fvisibility=hidden`, so the host never started. Now `JNIEXPORT`.
- Build-ID check now reads the ELF note from mapped memory first (file path fallback).
- `dlsym(RTLD_DEFAULT, nativeKeyHandler)` cannot see libminecraftpe.so (loaded RTLD_LOCAL); now uses dlopen(RTLD_NOLOAD) handle.
- Input hook failure no longer tears down the render hook (retry would chain the eglSwapBuffers detour into itself).
- ImGui: GLSL `#version 300 es`, no imgui.ini, scaled font, touch input routed via AInputQueue_getEvent hook (unverified on device).

CI log follow-up (runs 96352922898 / 96352923032)
- Both failed in `:app:buildCMakeDebug` with -Werror errors in vendored And64InlineHook.cpp and imgui_impl_android.cpp
  (exactly what the CMake change removes) and used NDK 27.0.12077973 (fixed by pinning ndkVersion).
- Kotlin daemon crashed on startup (memory): added kotlin.daemon.jvmargs=-Xmx3g.
