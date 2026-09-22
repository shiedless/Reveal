#include "Game/Memory.hpp"

#include <mach-o/dyld.h>
#include <mach/mach.h>

namespace {

// The link base every static address in Offsets.hpp is expressed against. The
// disassembler shows the image based at 0x100000000, so an RVA is (addr - this).
constexpr std::uintptr_t kLinkBase = 0x100000000ULL;

// arm64 iOS userland: real pointers sit above 4 GB and below the top of the
// 39-bit address space. Anything outside that is a misread field, not a pointer.
constexpr std::uintptr_t kMinUser = 0x0000000100000000ULL;
constexpr std::uintptr_t kMaxUser = 0x0000008000000000ULL;

// Work out the main executable's load address once. _dyld image 0 is always the
// main binary; its slide plus its Mach-O header address is where the image
// actually landed this launch.
std::uintptr_t LocateModuleBase() noexcept
{
    const std::uint32_t count = _dyld_image_count();
    for (std::uint32_t i = 0; i < count; ++i) {
        const mach_header* header = _dyld_get_image_header(i);
        if (header == nullptr) {
            continue;
        }
        // The main executable is the MH_EXECUTE image. Frameworks and dylibs are
        // MH_DYLIB, so this picks out the game binary itself.
        if (header->filetype == MH_EXECUTE) {
            return reinterpret_cast<std::uintptr_t>(header);
        }
    }
    return 0;
}

}  // namespace

namespace mem {

std::uintptr_t ModuleBase() noexcept
{
    // Cached: the base does not change for the life of the process, so we pay the
    // dyld walk exactly once.
    static const std::uintptr_t base = LocateModuleBase();
    return base;
}

std::uintptr_t Resolve(std::uintptr_t staticAddress) noexcept
{
    const std::uintptr_t base = ModuleBase();
    if (base == 0 || staticAddress < kLinkBase) {
        return 0;
    }
    return base + (staticAddress - kLinkBase);
}

bool IsPlausiblePointer(std::uintptr_t address) noexcept
{
    return address >= kMinUser && address < kMaxUser &&
           (address % alignof(void*)) == 0;
}

bool ReadRaw(std::uintptr_t address, void* out, std::size_t size) noexcept
{
    if (address == 0 || out == nullptr || size == 0) {
        return false;
    }
    // vm_read_overwrite copies straight into our buffer and, unlike a raw
    // dereference, returns an error for an unmapped or protected page instead of
    // taking the process down with us.
    vm_size_t got = 0;
    const kern_return_t rc = vm_read_overwrite(
        mach_task_self(), static_cast<vm_address_t>(address),
        static_cast<vm_size_t>(size), reinterpret_cast<vm_address_t>(out), &got);
    return rc == KERN_SUCCESS && got == size;
}

std::optional<std::uintptr_t> ReadPointer(std::uintptr_t address) noexcept
{
    const std::optional<std::uintptr_t> value = Read<std::uintptr_t>(address);
    if (!value || !IsPlausiblePointer(*value)) {
        return std::nullopt;
    }
    return value;
}

}  // namespace mem
