#include "gui/InGameGui.h"

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android/log.h>
#include <dlfcn.h>
#include <cfloat>
#include <algorithm>
#include <atomic>
#include <mutex>

#include "imgui.h"
#include "backends/imgui_impl_android.h"
#include "backends/imgui_impl_opengl3.h"
#include "dobby.h"
#include "bridge/GameBridge.h"
#include "modules/ModuleManager.h"

namespace eclient_runtime::host::InGameGui {
namespace {
constexpr const char* TAG = "EClientGui";
std::mutex g_mutex;
std::atomic_bool g_ready{false};
std::atomic_bool g_imguiReady{false};
std::atomic_bool g_menuOpen{false};
// Touch state written by the input thread, consumed by the render thread.
std::atomic<float> g_touchX{-1.0f};
std::atomic<float> g_touchY{-1.0f};
std::atomic_bool g_touchDown{false};
std::atomic_bool g_touchActive{false};
using SwapBuffersFn = EGLBoolean (*)(EGLDisplay, EGLSurface);
SwapBuffersFn g_originalSwap = nullptr;

EGLBoolean hookedSwap(EGLDisplay display, EGLSurface surface) {
    if (g_ready.load()) {
        EGLint width = 0;
        EGLint height = 0;
        eglQuerySurface(display, surface, EGL_WIDTH, &width);
        eglQuerySurface(display, surface, EGL_HEIGHT, &height);

        if (!g_imguiReady.exchange(true)) {
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGui::StyleColorsDark();
            // Never write imgui.ini into Minecraft's working directory.
            ImGui::GetIO().IniFilename = nullptr;
            // Built with IMGUI_IMPL_OPENGL_ES3 (Minecraft renders with GLES 3).
            ImGui_ImplOpenGL3_Init("#version 300 es");
            __android_log_print(ANDROID_LOG_INFO, TAG, "ImGui renderer initialized");
        }

        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2((float)width, (float)height);
        // The default 13px font is unreadable on phone-sized, high-density screens.
        io.FontGlobalScale = std::max(1.5f, static_cast<float>(std::min(width, height)) / 360.0f);

        // Apply touch input captured on the input thread.
        if (g_touchActive.load()) {
            io.AddMousePosEvent(g_touchX.load(), g_touchY.load());
            const bool down = g_touchDown.load();
            io.AddMouseButtonEvent(0, down);
            // Release has now been delivered; drop the hover position on the next frame.
            if (!down) g_touchActive.store(false);
        } else {
            io.AddMouseButtonEvent(0, false);
            io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
        }
        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();

        const bool open = g_menuOpen.load();
        if (open) {
            ImGui::SetNextWindowSize(ImVec2(440, 0), ImGuiCond_FirstUseEver);
            bool keepOpen = open;
            ImGui::Begin("E-Client Memory", &keepOpen, ImGuiWindowFlags_AlwaysAutoResize);
            const auto bridge = eclient_runtime::GameBridge::instance().snapshot();
            ImGui::Text("In-process Minecraft: %s", bridge.libraryLoaded ? "YES" : "NO");
            ImGui::Text("Build: %s", bridge.buildId.empty() ? "unknown" : bridge.buildId.c_str());
            ImGui::Text("Module base: 0x%llX", (unsigned long long)bridge.moduleBase);
            ImGui::Text("Executable ranges: %zu", bridge.executableRangeCount);
            ImGui::Separator();
            for (const auto& module : eclient_runtime::modules::ModuleManager::instance().all()) {
                bool enabled = module.enabled;
                if (ImGui::Checkbox(module.name.c_str(), &enabled)) {
                    eclient_runtime::modules::ModuleManager::instance().setEnabled(module.name, enabled);
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", module.description.c_str());
            }
            ImGui::Separator();
            ImGui::TextDisabled("Memory hooks are only enabled for a verified native profile.");
            ImGui::TextDisabled("Unknown offsets fail closed instead of writing arbitrary memory.");
            ImGui::End();
            if (!keepOpen) g_menuOpen.store(false);
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
    return g_originalSwap ? g_originalSwap(display, surface) : EGL_FALSE;
}

bool hookSwapBuffers() {
    if (g_originalSwap) return true;  // already hooked; never chain the detour into itself
    void* symbol = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    if (!symbol) symbol = reinterpret_cast<void*>(eglSwapBuffers);
    if (!symbol) return false;
    if (DobbyHook(symbol, reinterpret_cast<void*>(hookedSwap), reinterpret_cast<void**>(&g_originalSwap)) != RS_SUCCESS) {
        return false;
    }
    return g_originalSwap != nullptr;
}
} // namespace

void toggleMenu() { g_menuOpen.store(!g_menuOpen.load()); }

void submitTouch(float x, float y, int action) {
    g_touchX.store(x);
    g_touchY.store(y);
    if (action == 0) {
        g_touchActive.store(true);
        g_touchDown.store(true);
    } else if (action == 1) {
        if (g_touchDown.load()) g_touchActive.store(true);
    } else {
        // Release: the render thread delivers it at the final position, then clears.
        g_touchDown.store(false);
        g_touchActive.store(true);
    }
}
bool menuOpen() { return g_menuOpen.load(); }

bool initialize() {
    std::lock_guard lock(g_mutex);
    if (g_ready.load()) return true;
    if (!hookSwapBuffers()) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "eglSwapBuffers hook failed");
        return false;
    }
    g_ready.store(true);
    g_menuOpen.store(false);
    __android_log_print(ANDROID_LOG_INFO, TAG, "Render host ready");
    return true;
}

void shutdown() {
    std::lock_guard lock(g_mutex);
    g_ready.store(false);
    g_menuOpen.store(false);
    if (g_imguiReady.exchange(false)) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui::DestroyContext();
    }
}

} // namespace eclient_runtime::host::InGameGui
