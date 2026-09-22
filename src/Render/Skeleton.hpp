#pragma once

#include "imgui/imgui.h"

// Drawing the skeleton overlay. One call per frame, from the render loop, once
// the ImGui frame is open. It reads the world itself, so the caller only has to
// hand it a draw list and the screen size.
namespace render {

// Draw a skeleton on every character in the level. `drawList` is usually the
// background draw list so the lines sit under the menu.
void DrawSkeletons(ImDrawList* drawList, float screenWidth, float screenHeight) noexcept;

}  // namespace render
