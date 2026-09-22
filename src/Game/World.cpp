#include "Game/World.hpp"

#include <cmath>

#include "Game/Memory.hpp"
#include "Game/Offsets.hpp"

namespace game {
namespace {

// A UE FTransform as it sits in memory: a rotation quaternion, then the
// translation, then the scale. We only need the rotation and translation to lift
// a bone from component space into world space.
struct Transform {
    float rotation[4];      // x, y, z, w
    float translation[3];
    float pad;              // translation is padded to 16 bytes
    float scale[3];
};

// Rotate `v` by the quaternion in `t` and add the translation. This is the
// component-to-world step, done in-process so we never have to ask the engine
// for a world-space bone.
Vector ApplyTransform(const Transform& t, Vector v) noexcept
{
    const float qx = t.rotation[0], qy = t.rotation[1], qz = t.rotation[2], qw = t.rotation[3];

    // v' = v + 2 * cross(q.xyz, cross(q.xyz, v) + w * v)
    const float cx = qy * v.z - qz * v.y + qw * v.x;
    const float cy = qz * v.x - qx * v.z + qw * v.y;
    const float cz = qx * v.y - qy * v.x + qw * v.z;

    Vector out;
    out.x = v.x + 2.0f * (qy * cz - qz * cy) + t.translation[0];
    out.y = v.y + 2.0f * (qz * cx - qx * cz) + t.translation[1];
    out.z = v.z + 2.0f * (qx * cy - qy * cx) + t.translation[2];
    return out;
}

// A quick sanity test on a transform we just read. A live pose has a unit-length
// rotation quaternion; a mesh caught mid-rebuild reads back nonsense, and this
// rejects it before we draw a skeleton stretched across the screen.
bool LooksLikeTransform(const Transform& t) noexcept
{
    const float len = t.rotation[0] * t.rotation[0] + t.rotation[1] * t.rotation[1] +
                      t.rotation[2] * t.rotation[2] + t.rotation[3] * t.rotation[3];
    return std::isfinite(len) && len > 0.9f && len < 1.1f;
}

// The live UWorld*, following the direct global.
std::uintptr_t WorldPtr() noexcept
{
    const std::uintptr_t global = mem::Resolve(off::kGWorld);
    if (global == 0) {
        return 0;
    }
    return mem::ReadPointer(global).value_or(0);
}

}  // namespace

std::optional<View> Camera() noexcept
{
    const std::uintptr_t world = WorldPtr();
    if (world == 0) {
        return std::nullopt;
    }

    const auto instance = mem::ReadPointer(world + off::kGameInstance);
    if (!instance) {
        return std::nullopt;
    }
    // LocalPlayers[0]: read the array's data pointer, then the first element.
    const auto playersData = mem::ReadPointer(*instance + off::kLocalPlayers + off::kArrayData);
    if (!playersData) {
        return std::nullopt;
    }
    const auto localPlayer = mem::ReadPointer(*playersData);
    if (!localPlayer) {
        return std::nullopt;
    }
    const auto controller = mem::ReadPointer(*localPlayer + off::kPlayerController);
    if (!controller) {
        return std::nullopt;
    }
    const auto cameraManager = mem::ReadPointer(*controller + off::kCameraManager);
    if (!cameraManager) {
        return std::nullopt;
    }

    const std::uintptr_t pov = *cameraManager + off::kCameraPov;
    const auto location = mem::Read<Vector>(pov + off::kPovLocation);
    const auto rotation = mem::Read<Rotator>(pov + off::kPovRotation);
    const auto fov = mem::Read<float>(pov + off::kPovFov);
    if (!location || !rotation || !fov || !std::isfinite(*fov) || *fov < 1.0f || *fov > 179.0f) {
        return std::nullopt;
    }

    View view;
    view.location = *location;
    view.rotation = *rotation;
    view.fov = *fov;
    return view;
}

std::vector<std::uintptr_t> Actors() noexcept
{
    std::vector<std::uintptr_t> actors;

    const std::uintptr_t world = WorldPtr();
    if (world == 0) {
        return actors;
    }
    const auto level = mem::ReadPointer(world + off::kPersistentLevel);
    if (!level) {
        return actors;
    }

    const auto data = mem::ReadPointer(*level + off::kLevelActors + off::kArrayData);
    const auto count = mem::Read<std::int32_t>(*level + off::kLevelActors + off::kArrayCount);
    if (!data || !count || *count <= 0 || *count > 100000) {
        return actors;
    }

    actors.reserve(static_cast<std::size_t>(*count));
    for (std::int32_t i = 0; i < *count; ++i) {
        const auto actor = mem::ReadPointer(*data + static_cast<std::size_t>(i) * sizeof(void*));
        if (actor) {
            actors.push_back(*actor);
        }
    }
    return actors;
}

bool IsCharacter(std::uintptr_t actor) noexcept
{
    if (!mem::IsPlausiblePointer(actor)) {
        return false;
    }
    // The cheap test: a character has a mesh component pointer, scenery does not.
    // That is enough to filter the list before we do the heavier bone read.
    const auto mesh = mem::ReadPointer(actor + off::kMesh);
    return mesh.has_value();
}

bool Bones(std::uintptr_t actor, std::vector<Vector>& out) noexcept
{
    out.clear();

    const auto mesh = mem::ReadPointer(actor + off::kMesh);
    if (!mesh) {
        return false;
    }

    const auto component = mem::Read<Transform>(*mesh + off::kComponentToWorld);
    if (!component || !LooksLikeTransform(*component)) {
        return false;
    }

    // Pick the current pose buffer, then read its {data, count}.
    const auto bufIndex = mem::Read<std::int32_t>(*mesh + off::kSpaceBasesIndex);
    const std::int32_t idx = (bufIndex && *bufIndex >= 0 && *bufIndex < 2) ? *bufIndex : 0;
    const std::uintptr_t slot =
        *mesh + off::kSpaceBasesArray + off::kSpaceBasesStride * static_cast<std::size_t>(idx);

    const auto data = mem::ReadPointer(slot + off::kSpaceBasesDataInSlot);
    const auto count = mem::Read<std::int32_t>(slot + off::kSpaceBasesCountInSlot);
    if (!data || !count || *count <= 0 || *count > 512) {
        return false;
    }

    // Read the whole bone-transform block in one syscall, then apply the
    // component transform to each. Reading it bone by bone would be dozens of
    // syscalls per character per frame, which is the difference between a smooth
    // overlay and a slideshow.
    const auto n = static_cast<std::size_t>(*count);
    std::vector<Transform> raw(n);
    if (!mem::ReadRaw(*data, raw.data(), n * sizeof(Transform))) {
        return false;
    }

    out.reserve(n);
    for (const Transform& bone : raw) {
        out.push_back(ApplyTransform(
            *component, Vector{bone.translation[0], bone.translation[1], bone.translation[2]}));
    }
    return !out.empty();
}

std::optional<Vector2> WorldToScreen(const View& view, Vector world, float screenWidth,
                                     float screenHeight) noexcept
{
    constexpr float kDegToRad = 3.14159265358979323846f / 180.0f;

    const float sp = std::sin(view.rotation.pitch * kDegToRad);
    const float cp = std::cos(view.rotation.pitch * kDegToRad);
    const float sy = std::sin(view.rotation.yaw * kDegToRad);
    const float cy = std::cos(view.rotation.yaw * kDegToRad);
    const float sr = std::sin(view.rotation.roll * kDegToRad);
    const float cr = std::cos(view.rotation.roll * kDegToRad);

    // The camera's basis, built the way UE's FRotationMatrix does: X forward,
    // Y right, Z up.
    const Vector forward{cp * cy, cp * sy, sp};
    const Vector right{sp * sr * cy - cr * sy, sp * sr * sy + cr * cy, -sr * cp};
    const Vector up{-sp * cr * cy - sr * sy, -sp * cr * sy + sr * cy, cp * cr};

    const Vector delta{world.x - view.location.x, world.y - view.location.y,
                       world.z - view.location.z};

    const float depth = delta.x * forward.x + delta.y * forward.y + delta.z * forward.z;
    if (depth < 1.0f) {
        return std::nullopt;  // behind the camera
    }

    const float sideways = delta.x * right.x + delta.y * right.y + delta.z * right.z;
    const float vertical = delta.x * up.x + delta.y * up.y + delta.z * up.z;

    // A single scale from the horizontal FOV keeps both axes agreeing as the
    // camera turns, which is what stops the skeleton sliding off its target.
    const float half = screenWidth * 0.5f;
    const float tanHalfFov = std::tan(view.fov * 0.5f * kDegToRad);
    if (tanHalfFov < 0.0001f) {
        return std::nullopt;
    }
    const float scale = half / tanHalfFov;

    Vector2 screen;
    screen.x = half + sideways * scale / depth;
    screen.y = screenHeight * 0.5f - vertical * scale / depth;
    return screen;
}

}  // namespace game
