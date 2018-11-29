// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cstring>
#include "common/common_types.h"
#include "common/logging/log.h"
#include "core/hw/lcd.h"
#include "core/tracer/recorder.h"
#include "video_core/debug_utils/debug_utils.h"

namespace HW::LCD {

Lcd::Lcd(Core::System& system) : system(system) {}

/// Initialize hardware
void Lcd::Init() {
    memset(&regs, 0, sizeof(regs));
    LOG_DEBUG(HW_LCD, "initialized OK");
}

/// Shutdown hardware
void Lcd::Shutdown() {
    LOG_DEBUG(HW_LCD, "shutdown OK");
}

template <typename T>
inline void Lcd::Read(T& var, u32 addr) {
    addr -= VADDR_LCD;
    const u32 index = addr / 4;

    // Reads other than u32 are untested, so I'd rather have them abort than silently fail
    if (index >= 0x400 || !std::is_same<T, u32>::value) {
        LOG_ERROR(HW_LCD, "unknown Read{} @ {:#010X}", sizeof(var) * 8, addr);
        return;
    }

    var = regs[index];

    LOG_DEBUG(HW_LCD, "Read{} @ {:#010X} = {:#010X}", sizeof(var) * 8, addr, static_cast<u32>(var));
}

template <typename T>
inline void Lcd::Write(u32 addr, const T data) {
    addr -= VADDR_LCD;
    const u32 index = addr / 4;

    // Writes other than u32 are untested, so I'd rather have them abort than silently fail
    if (index >= 0x400 || !std::is_same<T, u32>::value) {
        LOG_ERROR(HW_LCD, "unknown Write{} {:#010X} @ {:#010X}", sizeof(data) * 8, (u32)data, addr);
        return;
    }

    regs[index] = static_cast<u32>(data);

    // Notify tracer about the register write
    // This is happening *after* handling the write to make sure we properly catch all memory reads.
    if (Pica::g_debug_context && Pica::g_debug_context->recorder) {
        // addr + GPU VBase - IO VBase + IO PBase
        Pica::g_debug_context->recorder->RegisterWritten<T>(
            addr + VADDR_LCD - VADDR_BASE + PADDR_BASE, data);
    }

    LOG_DEBUG(HW_LCD, "Write{} @ {:#010X} = {:#010X}", sizeof(data) * 8, addr, static_cast<u32>(data));
}

// Explicitly instantiate template functions because we aren't defining this in the header:

template void Lcd::Read<u64>(u64& var, const u32 addr);
template void Lcd::Read<u32>(u32& var, const u32 addr);
template void Lcd::Read<u16>(u16& var, const u32 addr);
template void Lcd::Read<u8>(u8& var, const u32 addr);

template void Lcd::Write<u64>(u32 addr, const u64 data);
template void Lcd::Write<u32>(u32 addr, const u32 data);
template void Lcd::Write<u16>(u32 addr, const u16 data);
template void Lcd::Write<u8>(u32 addr, const u8 data);

bool Lcd::IsValidAddress(VAddr addr) {
    return true;
};

u8 Lcd::Read8(VAddr addr) {
    u8 var;
    Read<u8>(var, addr);
    return var;
};

u16 Lcd::Read16(VAddr addr) {
    u16 var;
    Read<u16>(var, addr);
    return var;
};

u32 Lcd::Read32(VAddr addr) {
    u32 var;
    Read<u32>(var, addr);
    return var;
};

u64 Lcd::Read64(VAddr addr) {
    u64 var;
    Read<u64>(var, addr);
    return var;
};

bool Lcd::ReadBlock(VAddr src_addr, void* dest_buffer, std::size_t size) {
    return true;
};

void Lcd::Write8(VAddr addr, u8 data) {
    Write<u8>(addr, data);
};

void Lcd::Write16(VAddr addr, u16 data) {
    Write<u16>(addr, data);
};

void Lcd::Write32(VAddr addr, u32 data) {
    Write<u32>(addr, data);
};

void Lcd::Write64(VAddr addr, u64 data) {
    Write<u64>(addr, data);
};

bool Lcd::WriteBlock(VAddr dest_addr, const void* src_buffer, std::size_t size) {
    return true;
};

HW::LCD::Regs& Lcd::Regs() {
    return regs;
};

const HW::LCD::Regs& Lcd::Regs() const {
    return regs;
};

} // namespace HW::LCD
