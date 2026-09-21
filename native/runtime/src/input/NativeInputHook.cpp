#include "input/NativeInputHook.h"

#include <android/input.h>
#include <android/keycodes.h>
#include <android/log.h>
#include <dlfcn.h>
#include <atomic>

#include "dobby.h"
#include "gui/InGameGui.h"
#include "bridge/GameBridge.h"

namespace eclient_runtime::input {
namespace {
constexpr const char* TAG = "EClientInput";
using NativeKeyHandler = bool (*)(void*, void*, int, int);
using GetEventFn = int32_t (*)(AInputQueue*, AInputEvent**);

std::atomic_bool g_keyInstalled{false};
std::atomic_bool g_touchInstalled{false};
NativeKeyHandler g_original = nullptr;
GetEventFn g_originalGetEvent = nullptr;

bool hookedNativeKeyHandler(void* a1, void* a2, int keyCode, int keyAction) {
    // Android key action 0 is ACTION_DOWN. Volume-up is deliberately used as
    // the default mobile GUI hotkey because it does not require an on-screen
    // keyboard. Minecraft still receives the original event.
    if (keyCode == AKEYCODE_VOLUME_UP && keyAction == 0) {
        eclient_runtime::host::InGameGui::toggleMenu();
    }
    return g_original ? g_original(a1, a2, keyCode, keyAction) : false;
}

// While the menu is open, touch events are routed to ImGui and hidden from Minecraft.
int32_t hookedGetEvent(AInputQueue* queue, AInputEvent** outEvent) {
    const int32_t result = g_originalGetEvent ? g_originalGetEvent(queue, outEvent) : -1;
    if (result < 0 || !outEvent || !*outEvent) return result;
    if (!eclient_runtime::host::InGameGui::menuOpen()) return result;

    AInputEvent* ev = *outEvent;
    if (AInputEvent_getType(ev) != AINPUT_EVENT_TYPE_MOTION) return result;

    const float x = AMotionEvent_getX(ev, 0);
    const float y = AMotionEvent_getY(ev, 0);
    switch (AMotionEvent_getAction(ev) & AMOTION_EVENT_ACTION_MASK) {
        case AMOTION_EVENT_ACTION_DOWN:
            eclient_runtime::host::InGameGui::submitTouch(x, y, 0);
            break;
        case AMOTION_EVENT_ACTION_MOVE:
            eclient_runtime::host::InGameGui::submitTouch(x, y, 1);
            break;
        case AMOTION_EVENT_ACTION_UP:
        case AMOTION_EVENT_ACTION_CANCEL:
            eclient_runtime::host::InGameGui::submitTouch(x, y, 2);
            break;
        default:
            break;
    }
    // Consume the event so the game camera does not move behind the menu.
    AInputQueue_finishEvent(queue, ev, 1);
    *outEvent = nullptr;
    return -1;
}

// libminecraftpe.so is loaded by the app class loader (RTLD_LOCAL), so dlsym(RTLD_DEFAULT)
// cannot see its JNI exports. Look the symbol up on the already-loaded library instead.
void* findInMinecraft(const char* symbol) {
    const auto snap = eclient_runtime::GameBridge::instance().snapshot();
    const char* candidates[2] = {snap.libraryPath.empty() ? nullptr : snap.libraryPath.c_str(),
                                 "libminecraftpe.so"};
    for (const char* lib : candidates) {
        if (!lib) continue;
        void* handle = dlopen(lib, RTLD_NOW | RTLD_NOLOAD);
        if (!handle) continue;
        void* sym = dlsym(handle, symbol);
        dlclose(handle);
        if (sym) return sym;
    }
    return dlsym(RTLD_DEFAULT, symbol);
}

bool installKeyHook() {
    if (g_keyInstalled.load()) return true;
    void* symbol = findInMinecraft("Java_com_mojang_minecraftpe_MainActivity_nativeKeyHandler");
    if (!symbol) {
        __android_log_print(ANDROID_LOG_WARN, TAG, "nativeKeyHandler export not found");
        return false;
    }
    if (DobbyHook(symbol, reinterpret_cast<void*>(hookedNativeKeyHandler),
                  reinterpret_cast<void**>(&g_original)) != RS_SUCCESS || !g_original) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "nativeKeyHandler hook failed");
        g_original = nullptr;
        return false;
    }
    g_keyInstalled.store(true);
    __android_log_print(ANDROID_LOG_INFO, TAG, "Mobile GUI hotkey ready: Volume Up");
    return true;
}

bool installTouchHook() {
    if (g_touchInstalled.load()) return true;
    void* symbol = dlsym(RTLD_DEFAULT, "AInputQueue_getEvent");
    if (!symbol) symbol = reinterpret_cast<void*>(&AInputQueue_getEvent);
    if (!symbol) return false;
    if (DobbyHook(symbol, reinterpret_cast<void*>(hookedGetEvent),
                  reinterpret_cast<void**>(&g_originalGetEvent)) != RS_SUCCESS ||
        !g_originalGetEvent) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "AInputQueue_getEvent hook failed");
        g_originalGetEvent = nullptr;
        return false;
    }
    g_touchInstalled.store(true);
    __android_log_print(ANDROID_LOG_INFO, TAG, "Touch routing for the GUI ready");
    return true;
}
} // namespace

bool initialize() {
    // Input is best-effort: the render hook is what matters for the GUI to appear.
    // Report success if at least one input path is available.
    const bool key = installKeyHook();
    const bool touch = installTouchHook();
    if (!key) {
        // No hotkey available: start with the menu open so it is still reachable.
        eclient_runtime::host::InGameGui::toggleMenu();
    }
    return key || touch;
}

void shutdown() {
    // Dobby's unhook API varies by vendored revision. We intentionally keep the
    // hooks installed for the lifetime of the Minecraft process; the host only
    // clears its own state on shutdown.
    g_keyInstalled.store(false);
    g_touchInstalled.store(false);
}

} // namespace eclient_runtime::input
