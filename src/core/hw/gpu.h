// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <cstddef>
#include <type_traits>
#include "common/assert.h"
#include "common/bit_field.h"
#include "common/common_funcs.h"
#include "common/common_types.h"
#include "common/vector_math.h"
#include "core/core_timing.h"
#include "core/mmio.h"

// Returns index corresponding to the Regs member labeled by field_name
// TODO: Due to Visual studio bug 209229, offsetof does not return constant expressions
//       when used with array elements (e.g. GPU_REG_INDEX(memory_fill_config[0])).
//       For details cf.
//       https://connect.microsoft.com/VisualStudio/feedback/details/209229/offsetof-does-not-produce-a-constant-expression-for-array-members
//       Hopefully, this will be fixed sometime in the future.
//       For lack of better alternatives, we currently hardcode the offsets when constant
//       expressions are needed via GPU_REG_INDEX_WORKAROUND (on sane compilers, static_asserts
//       will then make sure the offsets indeed match the automatically calculated ones).
#define GPU_REG_INDEX(field_name) (offsetof(HW::GPU::Regs, field_name) / sizeof(u32))
#if defined(_MSC_VER)
#define GPU_REG_INDEX_WORKAROUND(field_name, backup_workaround_index) (backup_workaround_index)
#else
// NOTE: Yeah, hacking in a static_assert here just to workaround the lacking MSVC compiler
//       really is this annoying. This macro just forwards its first argument to GPU_REG_INDEX
//       and then performs a (no-op) cast to std::size_t iff the second argument matches the
//       expected field offset. Otherwise, the compiler will fail to compile this code.
#define GPU_REG_INDEX_WORKAROUND(field_name, backup_workaround_index)                              \
    ((typename std::enable_if<backup_workaround_index == GPU_REG_INDEX(field_name),                \
                              std::size_t>::type) GPU_REG_INDEX(field_name))
#endif

namespace Core {
class System;
}

namespace Service {
namespace GSP {
class GSP_GPU;
} // namespace GSP
} // namespace Service

namespace HW::GPU {

// Components are laid out in reverse byte order, most significant bits first.
enum class PixelFormat : u32 {
    RGBA8 = 0,
    RGB8 = 1,
    RGB565 = 2,
    RGB5A1 = 3,
    RGBA4 = 4,
};

enum class ScalingMode : u32 {
    NoScale = 0, // Doesn't scale the image
    ScaleX = 1,  // Downscales the image in half in the X axis and applies a box filter
    ScaleXY = 2, // Downscales the image in half in both the X and Y axes and applies a box filter
};

struct MemoryFillConfig {
    u32 address_start;
    u32 address_end;

    union {
        u32 value_32bit;

        BitField<0, 16, u32> value_16bit;

        // TODO: Verify component order
        BitField<0, 8, u32> value_24bit_r;
        BitField<8, 8, u32> value_24bit_g;
        BitField<16, 8, u32> value_24bit_b;
    };

    union {
        u32 control;

        // Setting this field to 1 triggers the memory fill.
        // This field also acts as a status flag, and gets reset to 0 upon completion.
        BitField<0, 1, u32> trigger;

        // Set to 1 upon completion.
        BitField<1, 1, u32> finished;

        // If both of these bits are unset, then it will fill the memory with a 16 bit value
        // 1: fill with 24-bit wide values
        BitField<8, 1, u32> fill_24bit;
        // 1: fill with 32-bit wide values
        BitField<9, 1, u32> fill_32bit;
    };
};
static_assert(sizeof(MemoryFillConfig) == 0x10, "MemoryFillConfig struct has incorrect size.");

struct FramebufferConfig {
    union {
        u32 size;

        BitField<0, 16, u32> width;
        BitField<16, 16, u32> height;
    };

    INSERT_PADDING_WORDS(0x2);
    u32 address_left1;
    u32 address_left2;

    union {
        u32 format;

        BitField<0, 3, PixelFormat> color_format;
    };

    INSERT_PADDING_WORDS(0x1);

    union {
        u32 active_fb;

        // 0: Use parameters ending with "1"
        // 1: Use parameters ending with "2"
        BitField<0, 1, u32> second_fb_active;
    };

    INSERT_PADDING_WORDS(0x5);
    u32 stride; // Distance between two pixel rows, in bytes
    u32 address_right1;
    u32 address_right2;
    INSERT_PADDING_WORDS(0x30);
};
static_assert(sizeof(FramebufferConfig) == 0x100, "FramebufferConfig struct has incorrect size.");

struct TextureCopyConfig {
    u32 size; // The lower 4 bits are ignored

    union {
        u32 input_size;

        BitField<0, 16, u32> input_width;
        BitField<16, 16, u32> input_gap;
    };

    union {
        u32 output_size;

        BitField<0, 16, u32> output_width;
        BitField<16, 16, u32> output_gap;
    };
};
static_assert(sizeof(TextureCopyConfig) == 0x0C, "TextureCopy struct has incorrect size.");

struct DisplayTransferConfig {
    u32 input_address;
    u32 output_address;

    union {
        u32 output_size;

        BitField<0, 16, u32> output_width;
        BitField<16, 16, u32> output_height;
    };

    union {
        u32 input_size;

        BitField<0, 16, u32> input_width;
        BitField<16, 16, u32> input_height;
    };

    union {
        u32 flags;

        BitField<0, 1, u32> flip_vertically; // flips input data vertically
        BitField<1, 1, u32> input_linear;    // Converts from linear to tiled format
        BitField<2, 1, u32> crop_input_lines;
        BitField<3, 1, u32> is_texture_copy; // Copies the data without performing any
                                             // processing and respecting texture copy fields
        BitField<5, 1, u32> dont_swizzle;
        BitField<8, 3, PixelFormat> input_format;
        BitField<12, 3, PixelFormat> output_format;
        /// Uses some kind of 32x32 block swizzling mode, instead of the usual 8x8 one.
        BitField<16, 1, u32> block_32;        // TODO(yuriks): unimplemented
        BitField<24, 2, ScalingMode> scaling; // Determines the scaling mode of the transfer
    };

    INSERT_PADDING_WORDS(0x1);
    u32 trigger; // it seems that writing to this field triggers the display transfer
    INSERT_PADDING_WORDS(0x1);
    TextureCopyConfig texture_copy;
};
static_assert(sizeof(DisplayTransferConfig) == 0x2C,
              "DisplayTransferConfig struct has incorrect size.");

struct CommandProcessorConfig {
    u32 size; // command list size (in bytes)
    INSERT_PADDING_WORDS(0x1);
    u32 address; // command list address
    INSERT_PADDING_WORDS(0x1);
    u32 trigger; // it seems that writing to this field triggers command list processing
};
static_assert(sizeof(CommandProcessorConfig) == 0x14,
              "CommandProcessorConfig struct has incorrect size.");

// MMIO region 0x1EFxxxxx
struct Regs {
    INSERT_PADDING_WORDS(0x4);
    MemoryFillConfig memory_fill_config[2];
    INSERT_PADDING_WORDS(0x10b);
    FramebufferConfig framebuffer_config[2];
    INSERT_PADDING_WORDS(0x169);
    DisplayTransferConfig display_transfer_config;
    INSERT_PADDING_WORDS(0x32D);
    CommandProcessorConfig command_processor_config;
    INSERT_PADDING_WORDS(0x9c3);

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
static_assert(sizeof(Regs) == 0x1000 * sizeof(u32), "Invalid total size of register set");
static_assert(offsetof(Regs, memory_fill_config[0]) == 0x04 * 4,
              "memory_fill_config[0] has invalid position");
static_assert(offsetof(Regs, memory_fill_config[1]) == 0x08 * 4,
              "memory_fill_config[1] has invalid position");
static_assert(offsetof(Regs, framebuffer_config[0]) == 0x0117 * 4,
              "framebuffer_config[0] has invalid position");
static_assert(offsetof(Regs, framebuffer_config[1]) == 0x0157 * 4,
              "framebuffer_config[1] has invalid position");
static_assert(offsetof(Regs, display_transfer_config) == 0x0300 * 4,
              "display_transfer_config has invalid position");
static_assert(offsetof(Regs, command_processor_config) == 0x0638 * 4,
              "command_processor_config has invalid position");

class Gpu : public Memory::MMIORegion {
public:
    explicit Gpu(Core::System& system);
    ~Gpu() = default;

    void Init();
    void Shutdown();

    void VBlankCallback(u64 userdata, s64 cycles_late);

    static int BytesPerPixel(PixelFormat format);

    static inline u32 DecodeAddressRegister(u32 register_value) {
        return register_value * 8;
    };

    static constexpr std::size_t NumIds() {
        return sizeof(HW::GPU::Regs) / sizeof(u32);
    }

    Math::Vec4<u8> DecodePixel(PixelFormat input_format, const u8* src_pixel);
    void DisplayTransfer(const DisplayTransferConfig& config);
    void MemoryFill(const MemoryFillConfig& config);
    void TextureCopy(const DisplayTransferConfig& config);

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

    HW::GPU::Regs& Regs();
    const HW::GPU::Regs& Regs() const;

    static constexpr u32 VADDR_GPU = 0x1EF00000;
    static constexpr float SCREEN_REFRESH_RATE = 60;

private:
    HW::GPU::Regs regs;
    Core::System& system;

    /// 268MHz CPU clocks / 60Hz frames per second
    static constexpr u64 frame_ticks =
        static_cast<u64>(BASE_CLOCK_RATE_ARM11 / SCREEN_REFRESH_RATE);

    /// Event id for CoreTiming
    Core::TimingEventType* vblank_event;
};

} // namespace HW::GPU