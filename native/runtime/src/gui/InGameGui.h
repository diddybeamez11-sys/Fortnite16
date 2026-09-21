#pragma once

namespace eclient_runtime::host::InGameGui {

bool initialize();
void shutdown();
void toggleMenu();
bool menuOpen();

// Thread-safe: may be called from Minecraft's input thread. The values are
// applied to ImGui on the render thread during the next frame.
// action: 0 = down, 1 = move, 2 = up/cancel.
void submitTouch(float x, float y, int action);

} // namespace eclient_runtime::host::InGameGui
