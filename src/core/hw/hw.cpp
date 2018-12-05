// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/common_types.h"
#include "common/logging/log.h"
#include "core/hw/aes/key.h"
#include "core/hw/gpu.h"
#include "core/hw/hw.h"

namespace HW {

/// Initialize hardware
void HardwareManager::Init() {
    AES::InitKeys();

    gpu->Init();
    lcd->Init();
    pica->Init();

    LOG_DEBUG(HW, "initialized OK");
}

/// Shutdown hardware
void HardwareManager::Shutdown() {
    gpu->Shutdown();
    lcd->Shutdown();
    pica->Shutdown();

    LOG_DEBUG(HW, "shutdown OK");
}

/// Update hardware
void HardwareManager::Update() {}

HardwareManager::HardwareManager(Core::System& system) : system(system) {
    gpu = std::make_shared<HW::GPU::Gpu>(system);
    lcd = std::make_shared<HW::LCD::Lcd>(system);
    pica = std::make_shared<Pica::Pica>(system);
}

template <typename T>
inline void HardwareManager::Read(T& var, const u32 addr) {
    switch (addr & 0xFFFFF000) {
    case HW::LCD::Lcd::VADDR_LCD: {
        lcd->Read(var, addr);
        break;
    }
    case GPU::Gpu::VADDR_GPU:
    case GPU::Gpu::VADDR_GPU + 0x1000:
    case GPU::Gpu::VADDR_GPU + 0x2000:
    case GPU::Gpu::VADDR_GPU + 0x3000:
    case GPU::Gpu::VADDR_GPU + 0x4000:
    case GPU::Gpu::VADDR_GPU + 0x5000:
    case GPU::Gpu::VADDR_GPU + 0x6000:
    case GPU::Gpu::VADDR_GPU + 0x7000:
    case GPU::Gpu::VADDR_GPU + 0x8000:
    case GPU::Gpu::VADDR_GPU + 0x9000:
    case GPU::Gpu::VADDR_GPU + 0xA000:
    case GPU::Gpu::VADDR_GPU + 0xB000:
    case GPU::Gpu::VADDR_GPU + 0xC000:
    case GPU::Gpu::VADDR_GPU + 0xD000:
    case GPU::Gpu::VADDR_GPU + 0xE000:
    case GPU::Gpu::VADDR_GPU + 0xF000: {
        gpu->Read(var, addr);
        break;
    }
    default:
        LOG_ERROR(HW_Memory, "unknown Read{} @ {:#010X}", sizeof(var) * 8, addr);
    }
}

template <typename T>
inline void HardwareManager::Write(u32 addr, const T data) {
    switch (addr & 0xFFFFF000) {
    case HW::LCD::Lcd::VADDR_LCD: {
        lcd->Write(addr, data);
        break;
    }
    case GPU::Gpu::VADDR_GPU:
    case GPU::Gpu::VADDR_GPU + 0x1000:
    case GPU::Gpu::VADDR_GPU + 0x2000:
    case GPU::Gpu::VADDR_GPU + 0x3000:
    case GPU::Gpu::VADDR_GPU + 0x4000:
    case GPU::Gpu::VADDR_GPU + 0x5000:
    case GPU::Gpu::VADDR_GPU + 0x6000:
    case GPU::Gpu::VADDR_GPU + 0x7000:
    case GPU::Gpu::VADDR_GPU + 0x8000:
    case GPU::Gpu::VADDR_GPU + 0x9000:
    case GPU::Gpu::VADDR_GPU + 0xA000:
    case GPU::Gpu::VADDR_GPU + 0xB000:
    case GPU::Gpu::VADDR_GPU + 0xC000:
    case GPU::Gpu::VADDR_GPU + 0xD000:
    case GPU::Gpu::VADDR_GPU + 0xE000:
    case GPU::Gpu::VADDR_GPU + 0xF000: {
        gpu->Write(addr, data);
        break;
    }
    default:
        LOG_ERROR(HW_Memory, "unknown Write{} {:#010X} @ {:#010X}", sizeof(data) * 8, (u32)data,
                  addr);
    }
}

template void HardwareManager::Read<u64>(u64& var, const u32 addr);
template void HardwareManager::Read<u32>(u32& var, const u32 addr);
template void HardwareManager::Read<u16>(u16& var, const u32 addr);
template void HardwareManager::Read<u8>(u8& var, const u32 addr);

template void HardwareManager::Write<u64>(u32 addr, const u64 data);
template void HardwareManager::Write<u32>(u32 addr, const u32 data);
template void HardwareManager::Write<u16>(u32 addr, const u16 data);
template void HardwareManager::Write<u8>(u32 addr, const u8 data);

HW::GPU::Gpu& HardwareManager::Gpu() {
    return *gpu;
};

const HW::GPU::Gpu& HardwareManager::Gpu() const {
    return *gpu;
};

HW::LCD::Lcd& HardwareManager::Lcd() {
    return *lcd;
};

const HW::LCD::Lcd& HardwareManager::Lcd() const {
    return *lcd;
};

Pica::Pica& HardwareManager::Pica() {
    return *pica;
};

const Pica::Pica& HardwareManager::Pica() const {
    return *pica;
};

} // namespace HW
