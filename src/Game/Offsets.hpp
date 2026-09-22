#pragma once

#include <cstddef>
#include <cstdint>

// The addresses and field offsets a skeleton ESP needs, and nothing else.
//
// Static addresses are given as the disassembler shows them (image linked at
// 0x100000000); mem::Resolve() applies the slide. Field offsets are byte offsets
// into the corresponding UE object. These are for a ShadowTrackerExtra 4.6 build;
// on a different build you re-find them, but the shape of the chain stays the
// same.
namespace off {

// The direct UWorld* global. There is an engine -> viewport -> world chain too,
// but the direct pointer is fewer reads and does not depend on the viewport
// being up, so it is what we use.
inline constexpr std::uintptr_t kGWorld = 0x10AA11EA0ULL;

// UWorld::PersistentLevel, and UWorld::OwningGameInstance which is our route to
// the local player and its camera.
inline constexpr std::size_t kPersistentLevel = 0x30;
inline constexpr std::size_t kGameInstance = 0x80;

// ULevel keeps its actor list as a TArray<AActor*>. This is the offset of that
// array header (data pointer + count) inside the level.
inline constexpr std::size_t kLevelActors = 0xA8;

// A TArray is a data pointer followed by an int32 element count.
inline constexpr std::size_t kArrayData = 0x0;
inline constexpr std::size_t kArrayCount = 0x8;

// Every UObject starts with a vtable then, a few fields in, its UClass pointer.
// We read the class to tell a character apart from all the scenery in the level.
inline constexpr std::size_t kObjectClass = 0x10;

// ACharacter::Mesh, the USkeletalMeshComponent we pull bone positions from.
inline constexpr std::size_t kMesh = 0x510;

// The mesh component's ComponentToWorld transform (a rotation quaternion then a
// translation). Bones come out of the pose in the component's own space, so we
// use this to lift them into world space.
inline constexpr std::size_t kComponentToWorld = 0x210;

// The live pose is a double-buffered array of bone transforms. One int32 index
// says which of the two buffers is current, and each buffer slot is itself
// {T* data; int32 count}. Reading this array directly, instead of asking the
// engine for each bone matrix, keeps the whole path to plain memory reads.
inline constexpr std::size_t kSpaceBasesArray = 0xA30;
inline constexpr std::size_t kSpaceBasesStride = 0x10;
inline constexpr std::size_t kSpaceBasesIndex = 0xAA4;
inline constexpr std::size_t kSpaceBasesDataInSlot = 0x0;
inline constexpr std::size_t kSpaceBasesCountInSlot = 0x8;

// The chain to the local camera: game instance to local player 0, to its
// controller, to the camera manager, to the cached point of view.
inline constexpr std::size_t kLocalPlayers = 0x48;
inline constexpr std::size_t kPlayerController = 0x30;
inline constexpr std::size_t kCameraManager = 0x548;

// APlayerCameraManager::CameraCache reaches the POV (an FMinimalViewInfo). Inside
// that struct the location, rotation and field of view sit at these offsets.
inline constexpr std::size_t kCameraPov = 0x530;
inline constexpr std::size_t kPovLocation = 0x0;
inline constexpr std::size_t kPovRotation = 0x18;
inline constexpr std::size_t kPovFov = 0x24;

}  // namespace off
