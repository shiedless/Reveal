#include "Render/Skeleton.hpp"

#include <optional>
#include <vector>

#include "Game/World.hpp"

namespace render {
namespace {

constexpr ImU32 kBoneColor = IM_COL32(255, 255, 255, 255);
constexpr float kBoneThickness = 1.4f;

// The figure as pairs of bone indices to connect. These are the standard UE4
// mannequin joint indices, which ShadowTrackerExtra uses, head down through the
// spine to the pelvis, then out to each limb. If a build renumbers its rig you
// only touch this table, the rest of the drawing does not care what a bone means.
struct BonePair {
    int a;
    int b;
};

constexpr BonePair kSkeleton[] = {
    {5, 4},   {4, 3},   {3, 2},   {2, 1},          // head -> neck -> spine -> pelvis
    {4, 34},  {34, 35}, {35, 36},                   // right arm: clavicle, upper, lower, hand
    {4, 11},  {11, 12}, {12, 13},                   // left arm
    {1, 62},  {62, 63}, {63, 64},                   // right leg: thigh, calf, foot
    {1, 56},  {56, 57}, {57, 58},                   // left leg
};

// The highest bone index the table touches. If a pose has fewer bones than this
// it is not the rig we expect, so we skip it rather than index past the array.
constexpr int kMaxBoneIndex = 64;

}  // namespace

void DrawSkeletons(ImDrawList* drawList, float screenWidth, float screenHeight) noexcept
{
    if (drawList == nullptr) {
        return;
    }

    const std::optional<game::View> view = game::Camera();
    if (!view) {
        return;  // no camera yet, nothing to project against
    }

    // Reused across actors so we are not reallocating a bone buffer every frame.
    std::vector<game::Vector> bones;

    for (const std::uintptr_t actor : game::Actors()) {
        if (!game::IsCharacter(actor)) {
            continue;
        }
        if (!game::Bones(actor, bones) || static_cast<int>(bones.size()) <= kMaxBoneIndex) {
            continue;
        }

        for (const BonePair& pair : kSkeleton) {
            const std::optional<game::Vector2> from =
                game::WorldToScreen(*view, bones[pair.a], screenWidth, screenHeight);
            const std::optional<game::Vector2> to =
                game::WorldToScreen(*view, bones[pair.b], screenWidth, screenHeight);
            // Both ends have to be in front of the camera for the line to mean
            // anything; one behind would draw a streak across the screen.
            if (!from || !to) {
                continue;
            }
            drawList->AddLine(ImVec2(from->x, from->y), ImVec2(to->x, to->y), kBoneColor,
                              kBoneThickness);
        }
    }
}

}  // namespace render
