// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <cstddef>
#include <type_traits>
#include "common/bit_field.h"
#include "common/common_funcs.h"
#include "common/common_types.h"
#include "core/mmio.h"

#define LCD_REG_INDEX(field_name) (offsetof(HW::LCD::Regs, field_name) / sizeof(u32))

namespace Core {
class System;
}


namespace HW::LCD {

struct Regs {

    union ColorFill {
        u32 raw;

        BitField<0, 8, u32> color_r;
        BitField<8, 8, u32> color_g;
        BitField<16, 8, u32> color_b;
        BitField<24, 1, u32> is_enabled;
    };

    INSERT_PADDING_WORDS(0x81);
    ColorFill color_fill_top;
    INSERT_PADDING_WORDS(0xE);
    u32 backlight_top;
    INSERT_PADDING_WORDS(0x1F0);
    ColorFill color_fill_bottom;
    INSERT_PADDING_WORDS(0xE);
    u32 backlight_bottom;
    INSERT_PADDING_WORDS(0x16F);

    static constexpr std::size_t NumIds() {
        return sizeof(Regs) / sizeof(u32);
    }

    const u32& operator[](int index) const {
        const u32* content = reinterpret_cast<const u32*>(this);
        return content[index];
    }

    u32& operator[](int index) {
        u32* content = reinterpret_cast<u32*>(this);
        return content[index];
    }
};
static_assert(std::is_standard_layout<Regs>::value, "Regs Structure does not use standard layout");
static_assert(offsetof(Regs, color_fill_top) == 0x81 * 4,
              "color_fill_top has invalid position");
static_assert(offsetof(Regs, backlight_top) == 0x90 * 4,
              "backlight_top has invalid position");
static_assert(offsetof(Regs, color_fill_bottom) == 0x281 * 4,
              "color_fill_bottom has invalid position");
static_assert(offsetof(Regs, backlight_bottom) == 0x290 * 4,
              "backlight_bottom has invalid position");

class Lcd : public Memory::MMIORegion {
public:
    explicit Lcd(Core::System& system);
    ~Lcd() = default;

    void Init();
    void Shutdown();

    template <typename T>
    void Read(T& var, u32 addr);

    template <typename T>
    void Write(u32 addr, const T data);

    bool IsValidAddress(VAddr addr) override;

    u8 Read8(VAddr addr) override;
    u16 Read16(VAddr addr) override;
    u32 Read32(VAddr addr) override;
    u64 Read64(VAddr addr) override;

    bool ReadBlock(VAddr src_addr, void* dest_buffer, std::size_t size) override;

    void Write8(VAddr addr, u8 data) override;
    void Write16(VAddr addr, u16 data) override;
    void Write32(VAddr addr, u32 data) override;
    void Write64(VAddr addr, u64 data) override;

    bool WriteBlock(VAddr dest_addr, const void* src_buffer, std::size_t size) override;

    HW::LCD::Regs& Regs();
    const HW::LCD::Regs& Regs() const;

    static constexpr u32 VADDR_LCD = 0x1ED02000;
private:
    HW::LCD::Regs regs;
    Core::System& system;
};
} // namespace HW::LCD
