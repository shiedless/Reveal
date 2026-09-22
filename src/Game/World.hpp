#pragma once

#include <cstdint>
#include <optional>
#include <vector>

// The game side of the ESP: reading the world, the camera and each character's
// bones, and turning a world point into a screen point. Everything is a plain
// memory read, nothing calls back into the engine.
namespace game {

struct Vector {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct Vector2 {
    float x = 0.0f;
    float y = 0.0f;
};

// UE rotators are in degrees.
struct Rotator {
    float pitch = 0.0f;
    float yaw = 0.0f;
    float roll = 0.0f;
};

// The camera's point of view for this frame. We read it once and hand it to
// WorldToScreen so every point in a frame is projected against the same view.
struct View {
    Vector location{};
    Rotator rotation{};
    float fov = 90.0f;
};

// The current camera POV, or nothing while the chain to the local player has not
// come up yet (loading screens, the first frames of a match).
[[nodiscard]] std::optional<View> Camera() noexcept;

// Every actor pointer in the persistent level. This is the raw list, scenery
// included; the caller decides which of them are characters worth drawing.
[[nodiscard]] std::vector<std::uintptr_t> Actors() noexcept;

// True if this actor is a skeletal character (it has a readable mesh with a bone
// pose). Cheap enough to call while filtering the actor list.
[[nodiscard]] bool IsCharacter(std::uintptr_t actor) noexcept;

// The actor's bones in world space, in the order the pose stores them. Empty if
// the mesh or its pose could not be read this frame. `out` is filled, not
// appended to.
[[nodiscard]] bool Bones(std::uintptr_t actor, std::vector<Vector>& out) noexcept;

// Project a world point onto the screen. std::nullopt when the point is behind
// the camera or the projection is degenerate.
[[nodiscard]] std::optional<Vector2> WorldToScreen(const View& view, Vector world,
                                                   float screenWidth,
                                                   float screenHeight) noexcept;

}  // namespace game
