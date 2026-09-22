#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

// Reading the game's memory.
//
// Everything here is read-only. We never write to the game's pages, and we go
// through mach_vm_read_overwrite rather than dereferencing raw pointers, so a
// stale or wild address gives us a failed read instead of a crash inside our own
// draw loop. That one rule is what keeps an overlay stable while the game is
// constantly freeing and reallocating the objects we're looking at.
namespace mem {

// The main image's load address (the ASLR slide is already applied). All the
// static addresses in Offsets.hpp are relative to a link base of 0x100000000,
// so ModuleBase() + (address - 0x100000000) gives the live location. Resolve()
// does that for you.
[[nodiscard]] std::uintptr_t ModuleBase() noexcept;

// A static address from the disassembler (linked at 0x100000000) turned into a
// live address in this process. Returns 0 before the image is located.
[[nodiscard]] std::uintptr_t Resolve(std::uintptr_t staticAddress) noexcept;

// Is this even worth reading? A real userland heap/code pointer on arm64 iOS
// lives above 4 GB and is pointer-aligned. Cheap junk-rejection before a syscall.
[[nodiscard]] bool IsPlausiblePointer(std::uintptr_t address) noexcept;

// Copy `size` bytes from the game into `out`. False on any fault; `out` is left
// untouched on failure.
[[nodiscard]] bool ReadRaw(std::uintptr_t address, void* out, std::size_t size) noexcept;

// Typed read. std::nullopt on a failed read, so a bad pointer never becomes a
// garbage value that silently poisons everything downstream.
template <typename T>
[[nodiscard]] std::optional<T> Read(std::uintptr_t address) noexcept
{
    T value{};
    if (!ReadRaw(address, &value, sizeof(T))) {
        return std::nullopt;
    }
    return value;
}

// Read a pointer field and reject the obvious garbage in one step.
[[nodiscard]] std::optional<std::uintptr_t> ReadPointer(std::uintptr_t address) noexcept;

}  // namespace mem
