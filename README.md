# Reveal

A minimal skeleton ESP for UE4 games on iOS. It draws a bone skeleton over the
characters in the level and nothing else. Small on purpose, the kind of codebase
you can read start to finish in one sitting.

Metal + Dear ImGui, drawn in a transparent overlay. Built as a Theos tweak.

## how it works

Four small pieces:

- `src/Game/Memory` reads the game through `vm_read_overwrite`. Read-only and
  fault-tolerant, a bad pointer gives a failed read instead of a crash.
- `src/Game/World` walks the engine: `GWorld` to the level to the actor list, the
  local camera, and each character's bone pose straight out of the skeletal mesh.
- `src/Render/Skeleton` connects the bones into a figure and projects each joint
  to the screen.
- `src/Overlay` is the Metal/ImGui host plus the loader that brings it up and
  wires the show/hide gestures.

Read the world, transform the bones, project, draw lines. That's the whole thing.

## controls

- three-finger double tap: show the menu
- two-finger double tap: hide it

Touches only reach the overlay while the menu is open, so the game stays playable
otherwise.

## building

Needs [Theos](https://theos.dev) with `$THEOS` set.

make package



The `.deb` lands in `packages/`.

## porting it

Everything build-specific is in `src/Game/Offsets.hpp`. Point it at another build
by updating the globals and the mesh/camera offsets there. If the rig is
renumbered, fix the bone pairs in `src/Render/Skeleton.cpp`.

## notes

Read-only visual code. It doesn't write game memory, move the camera, or send
anything anywhere. Meant as a clean reference for how a UE4 iOS overlay fits
together.

MIT.

— shiedless
