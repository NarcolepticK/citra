// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cstring>
#include <numeric>
#include <type_traits>
#include "common/alignment.h"
#include "common/color.h"
#include "common/common_types.h"
#include "common/logging/log.h"
#include "common/microprofile.h"
#include "common/vector_math.h"
#include "core/core.h"
#include "core/hle/service/gsp/gsp.h"
#include "core/hw/gpu.h"
#include "core/hw/hw.h"
#include "core/hw/pica.h"
#include "core/memory.h"
#include "core/tracer/recorder.h"
#include "video_core/debugger/debugger.h"
#include "video_core/rasterizer_interface.h"
#include "video_core/renderer_base.h"
#include "video_core/utils.h"
#include "video_core/video_core.h"

MICROPROFILE_DEFINE(GPU_DisplayTransfer, "GPU", "DisplayTransfer", MP_RGB(100, 100, 255));
MICROPROFILE_DEFINE(GPU_CmdlistProcessing, "GPU", "Cmdlist Processing", MP_RGB(100, 255, 100));

namespace HW::GPU {

Gpu::Gpu(Core::System& system) : system(system) {}

/// Initialize hardware
void Gpu::Init() {
    memset(&regs, 0, sizeof(regs));

    auto& framebuffer_top = regs.framebuffer_config[0];
    auto& framebuffer_sub = regs.framebuffer_config[1];

    // Setup default framebuffer addresses (located in VRAM)
    // .. or at least these are the ones used by system applets.
    // There's probably a smarter way to come up with addresses
    // like this which does not require hardcoding.
    framebuffer_top.address_left1 = 0x181E6000;
    framebuffer_top.address_left2 = 0x1822C800;
    framebuffer_top.address_right1 = 0x18273000;
    framebuffer_top.address_right2 = 0x182B9800;
    framebuffer_sub.address_left1 = 0x1848F000;
    framebuffer_sub.address_left2 = 0x184C7800;

    framebuffer_top.width.Assign(240);
    framebuffer_top.height.Assign(400);
    framebuffer_top.stride = 3 * 240;
    framebuffer_top.color_format.Assign(PixelFormat::RGB8);
    framebuffer_top.active_fb = 0;

    framebuffer_sub.width.Assign(240);
    framebuffer_sub.height.Assign(320);
    framebuffer_sub.stride = 3 * 240;
    framebuffer_sub.color_format.Assign(PixelFormat::RGB8);
    framebuffer_sub.active_fb = 0;

    Core::Timing& timing = system.CoreTiming();
    vblank_event =
        timing.RegisterEvent("GPU::VBlankCallback", [this](u64 userdata, s64 cycles_late) {
            this->VBlankCallback(userdata, cycles_late);
        });
    timing.ScheduleEvent(frame_ticks, vblank_event);

    LOG_DEBUG(HW_GPU, "initialized OK");
}

/// Shutdown hardware
void Gpu::Shutdown() {
    LOG_DEBUG(HW_GPU, "shutdown OK");
}

/// Update hardware
void Gpu::VBlankCallback(u64 userdata, s64 cycles_late) {
    system.VideoCore().Renderer().SwapBuffers();

    // Signal to GSP that GPU interrupt has occurred
    // TODO(yuriks): hwtest to determine if PDC0 is for the Top screen and PDC1 for the Sub
    // screen, or if both use the same interrupts and these two instead determine the
    // beginning and end of the VBlank period. If needed, split the interrupt firing into
    // two different intervals.
    system.VideoCore().SignalInterrupt(Service::GSP::InterruptId::PDC0);
    system.VideoCore().SignalInterrupt(Service::GSP::InterruptId::PDC1);

    // Reschedule recurrent event
    system.CoreTiming().ScheduleEvent(frame_ticks - cycles_late, vblank_event);
}

int Gpu::BytesPerPixel(PixelFormat format) {
    switch (format) {
    case PixelFormat::RGBA8:
        return 4;
    case PixelFormat::RGB8:
        return 3;
    case PixelFormat::RGB565:
    case PixelFormat::RGB5A1:
    case PixelFormat::RGBA4:
        return 2;
    }

    UNREACHABLE();
}

Math::Vec4<u8> Gpu::DecodePixel(PixelFormat input_format, const u8* src_pixel) {
    switch (input_format) {
    case PixelFormat::RGBA8:
        return Color::DecodeRGBA8(src_pixel);
    case PixelFormat::RGB8:
        return Color::DecodeRGB8(src_pixel);
    case PixelFormat::RGB565:
        return Color::DecodeRGB565(src_pixel);
    case PixelFormat::RGB5A1:
        return Color::DecodeRGB5A1(src_pixel);
    case PixelFormat::RGBA4:
        return Color::DecodeRGBA4(src_pixel);
    default:
        LOG_ERROR(HW_GPU, "Unknown source framebuffer format {:x}", static_cast<u32>(input_format));
        return {0, 0, 0, 0};
    }
}

void Gpu::MemoryFill(const MemoryFillConfig& config) {
    const PAddr start_addr = DecodeAddressRegister(config.address_start);
    const PAddr end_addr = DecodeAddressRegister(config.address_end);

    // TODO: do hwtest with these cases
    if (!system.Memory().IsValidPhysicalAddress(start_addr)) {
        LOG_CRITICAL(HW_GPU, "invalid start address {:#010X}", start_addr);
        return;
    }

    if (!system.Memory().IsValidPhysicalAddress(end_addr)) {
        LOG_CRITICAL(HW_GPU, "invalid end address {:#010X}", end_addr);
        return;
    }

    if (end_addr <= start_addr) {
        LOG_CRITICAL(HW_GPU, "invalid memory range from {:#010X} to {:#010X}", start_addr,
                     end_addr);
        return;
    }

    u8* start = system.Memory().GetPhysicalPointer(start_addr);
    u8* end = system.Memory().GetPhysicalPointer(end_addr);

    if (system.VideoCore().Renderer().Rasterizer()->AccelerateFill(config))
        return;

    system.Memory().RasterizerInvalidateRegion(start_addr, end_addr - start_addr);

    if (config.fill_24bit) {
        // fill with 24-bit values
        for (u8* ptr = start; ptr < end; ptr += 3) {
            ptr[0] = config.value_24bit_r;
            ptr[1] = config.value_24bit_g;
            ptr[2] = config.value_24bit_b;
        }
    } else if (config.fill_32bit) {
        // fill with 32-bit values
        if (end > start) {
            const u32 value = config.value_32bit;
            std::size_t len = (end - start) / sizeof(u32);
            for (std::size_t i = 0; i < len; ++i)
                memcpy(&start[i * sizeof(u32)], &value, sizeof(u32));
        }
    } else {
        // fill with 16-bit values
        const u16 value_16bit = config.value_16bit.Value();
        for (u8* ptr = start; ptr < end; ptr += sizeof(u16))
            memcpy(ptr, &value_16bit, sizeof(u16));
    }
}

void Gpu::DisplayTransfer(const DisplayTransferConfig& config) {
    const PAddr src_addr = DecodeAddressRegister(config.input_address);
    const PAddr dst_addr = DecodeAddressRegister(config.output_address);

    // TODO: do hwtest with these cases
    if (!system.Memory().IsValidPhysicalAddress(src_addr)) {
        LOG_CRITICAL(HW_GPU, "invalid input address {:#010X}", src_addr);
        return;
    }

    if (!system.Memory().IsValidPhysicalAddress(dst_addr)) {
        LOG_CRITICAL(HW_GPU, "invalid output address {:#010X}", dst_addr);
        return;
    }

    if (config.input_width == 0) {
        LOG_CRITICAL(HW_GPU, "zero input width");
        return;
    }

    if (config.input_height == 0) {
        LOG_CRITICAL(HW_GPU, "zero input height");
        return;
    }

    if (config.output_width == 0) {
        LOG_CRITICAL(HW_GPU, "zero output width");
        return;
    }

    if (config.output_height == 0) {
        LOG_CRITICAL(HW_GPU, "zero output height");
        return;
    }

    if (system.VideoCore().Renderer().Rasterizer()->AccelerateDisplayTransfer(config))
        return;

    const u8* src_pointer = system.Memory().GetPhysicalPointer(src_addr);
    u8* dst_pointer = system.Memory().GetPhysicalPointer(dst_addr);

    if (config.scaling > ScalingMode::ScaleXY) {
        LOG_CRITICAL(HW_GPU, "Unimplemented display transfer scaling mode {}",
                     static_cast<u32>(config.scaling.Value()));
        UNIMPLEMENTED();
        return;
    }

    if (config.input_linear && config.scaling != ScalingMode::NoScale) {
        LOG_CRITICAL(HW_GPU, "Scaling is only implemented on tiled input");
        UNIMPLEMENTED();
        return;
    }

    const int horizontal_scale = config.scaling != ScalingMode::NoScale ? 1 : 0;
    const int vertical_scale = config.scaling == ScalingMode::ScaleXY ? 1 : 0;

    const u32 output_width = config.output_width >> horizontal_scale;
    const u32 output_height = config.output_height >> vertical_scale;

    const u32 input_size =
        config.input_width * config.input_height * BytesPerPixel(config.input_format);
    const u32 output_size = output_width * output_height * BytesPerPixel(config.output_format);

    system.Memory().RasterizerFlushRegion(src_addr, input_size);
    system.Memory().RasterizerInvalidateRegion(dst_addr, output_size);

    for (u32 y = 0; y < output_height; ++y) {
        for (u32 x = 0; x < output_width; ++x) {
            Math::Vec4<u8> src_color;

            // Calculate the [x,y] position of the input image
            // based on the current output position and the scale
            const u32 input_x = x << horizontal_scale;
            const u32 input_y = y << vertical_scale;

            u32 output_y;
            if (config.flip_vertically) {
                // Flip the y value of the output data,
                // we do this after calculating the [x,y] position of the input image
                // to account for the scaling options.
                output_y = output_height - y - 1;
            } else {
                output_y = y;
            }

            const u32 dst_bytes_per_pixel = BytesPerPixel(config.output_format);
            const u32 src_bytes_per_pixel = BytesPerPixel(config.input_format);
            u32 src_offset;
            u32 dst_offset;

            if (config.input_linear) {
                if (!config.dont_swizzle) {
                    // Interpret the input as linear and the output as tiled
                    const u32 coarse_y = output_y & ~7;
                    const u32 stride = output_width * dst_bytes_per_pixel;

                    src_offset = (input_x + input_y * config.input_width) * src_bytes_per_pixel;
                    dst_offset = VideoCore::GetMortonOffset(x, output_y, dst_bytes_per_pixel) +
                                 coarse_y * stride;
                } else {
                    // Both input and output are linear
                    src_offset = (input_x + input_y * config.input_width) * src_bytes_per_pixel;
                    dst_offset = (x + output_y * output_width) * dst_bytes_per_pixel;
                }
            } else {
                if (!config.dont_swizzle) {
                    // Interpret the input as tiled and the output as linear
                    const u32 coarse_y = input_y & ~7;
                    const u32 stride = config.input_width * src_bytes_per_pixel;

                    src_offset = VideoCore::GetMortonOffset(input_x, input_y, src_bytes_per_pixel) +
                                 coarse_y * stride;
                    dst_offset = (x + output_y * output_width) * dst_bytes_per_pixel;
                } else {
                    // Both input and output are tiled
                    const u32 out_coarse_y = output_y & ~7;
                    const u32 out_stride = output_width * dst_bytes_per_pixel;

                    const u32 in_coarse_y = input_y & ~7;
                    const u32 in_stride = config.input_width * src_bytes_per_pixel;

                    src_offset = VideoCore::GetMortonOffset(input_x, input_y, src_bytes_per_pixel) +
                                 in_coarse_y * in_stride;
                    dst_offset = VideoCore::GetMortonOffset(x, output_y, dst_bytes_per_pixel) +
                                 out_coarse_y * out_stride;
                }
            }

            const u8* src_pixel = src_pointer + src_offset;
            src_color = DecodePixel(config.input_format, src_pixel);
            if (config.scaling == ScalingMode::ScaleX) {
                const Math::Vec4<u8> pixel =
                    DecodePixel(config.input_format, src_pixel + src_bytes_per_pixel);
                src_color = ((src_color + pixel) / 2).Cast<u8>();
            } else if (config.scaling == ScalingMode::ScaleXY) {
                const Math::Vec4<u8> pixel1 =
                    DecodePixel(config.input_format, src_pixel + 1 * src_bytes_per_pixel);
                const Math::Vec4<u8> pixel2 =
                    DecodePixel(config.input_format, src_pixel + 2 * src_bytes_per_pixel);
                const Math::Vec4<u8> pixel3 =
                    DecodePixel(config.input_format, src_pixel + 3 * src_bytes_per_pixel);
                src_color = (((src_color + pixel1) + (pixel2 + pixel3)) / 4).Cast<u8>();
            }

            u8* dst_pixel = dst_pointer + dst_offset;
            switch (config.output_format) {
            case PixelFormat::RGBA8:
                Color::EncodeRGBA8(src_color, dst_pixel);
                break;

            case PixelFormat::RGB8:
                Color::EncodeRGB8(src_color, dst_pixel);
                break;

            case PixelFormat::RGB565:
                Color::EncodeRGB565(src_color, dst_pixel);
                break;

            case PixelFormat::RGB5A1:
                Color::EncodeRGB5A1(src_color, dst_pixel);
                break;

            case PixelFormat::RGBA4:
                Color::EncodeRGBA4(src_color, dst_pixel);
                break;

            default:
                LOG_ERROR(HW_GPU, "Unknown destination framebuffer format {:x}",
                          static_cast<u32>(config.output_format.Value()));
                break;
            }
        }
    }
}

void Gpu::TextureCopy(const DisplayTransferConfig& config) {
    const PAddr src_addr = DecodeAddressRegister(config.input_address);
    const PAddr dst_addr = DecodeAddressRegister(config.output_address);

    // TODO: do hwtest with invalid addresses
    if (!system.Memory().IsValidPhysicalAddress(src_addr)) {
        LOG_CRITICAL(HW_GPU, "invalid input address {:#010X}", src_addr);
        return;
    }

    if (!system.Memory().IsValidPhysicalAddress(dst_addr)) {
        LOG_CRITICAL(HW_GPU, "invalid output address {:#010X}", dst_addr);
        return;
    }

    if (system.VideoCore().Renderer().Rasterizer()->AccelerateTextureCopy(config))
        return;

    u8* src_pointer = system.Memory().GetPhysicalPointer(src_addr);
    u8* dst_pointer = system.Memory().GetPhysicalPointer(dst_addr);

    u32 remaining_size = Common::AlignDown(config.texture_copy.size, 16);

    if (remaining_size == 0) {
        LOG_CRITICAL(HW_GPU, "zero size. Real hardware freezes on this.");
        return;
    }

    const u32 input_gap = config.texture_copy.input_gap * 16;
    const u32 output_gap = config.texture_copy.output_gap * 16;

    // Zero gap means contiguous input/output even if width = 0. To avoid infinite loop below, width
    // is assigned with the total size if gap = 0.
    const u32 input_width = input_gap == 0 ? remaining_size : config.texture_copy.input_width * 16;
    const u32 output_width =
        output_gap == 0 ? remaining_size : config.texture_copy.output_width * 16;

    if (input_width == 0) {
        LOG_CRITICAL(HW_GPU, "zero input width. Real hardware freezes on this.");
        return;
    }

    if (output_width == 0) {
        LOG_CRITICAL(HW_GPU, "zero output width. Real hardware freezes on this.");
        return;
    }

    const std::size_t contiguous_input_size =
        config.texture_copy.size / input_width * (input_width + input_gap);
    system.Memory().RasterizerFlushRegion(src_addr, static_cast<u32>(contiguous_input_size));

    const std::size_t contiguous_output_size =
        config.texture_copy.size / output_width * (output_width + output_gap);
    // Only need to flush output if it has a gap
    const auto FlushInvalidate_fn = (output_gap != 0) ? Memory::RasterizerFlushAndInvalidateRegion
                                                      : Memory::RasterizerInvalidateRegion;
    FlushInvalidate_fn(dst_addr, static_cast<u32>(contiguous_output_size));

    u32 remaining_input = input_width;
    u32 remaining_output = output_width;
    while (remaining_size > 0) {
        u32 copy_size = std::min({remaining_input, remaining_output, remaining_size});

        std::memcpy(dst_pointer, src_pointer, copy_size);
        src_pointer += copy_size;
        dst_pointer += copy_size;

        remaining_input -= copy_size;
        remaining_output -= copy_size;
        remaining_size -= copy_size;

        if (remaining_input == 0) {
            remaining_input = input_width;
            src_pointer += input_gap;
        }
        if (remaining_output == 0) {
            remaining_output = output_width;
            dst_pointer += output_gap;
        }
    }
}

template <typename T>
inline void Gpu::Read(T& var, u32 addr) {
    addr -= VADDR_GPU;
    const u32 index = addr / 4;

    // Reads other than u32 are untested, so I'd rather have them abort than silently fail
    if (index >= NumIds() || !std::is_same<T, u32>::value) {
        LOG_ERROR(HW_GPU, "unknown Read{} @ {:#010X}", sizeof(var) * 8, addr);
        return;
    }

    var = regs[index];
}

template <typename T>
inline void Gpu::Write(u32 addr, const T data) {
    addr -= VADDR_GPU;
    const u32 index = addr / 4;

    // Writes other than u32 are untested, so I'd rather have them abort than silently fail
    if (index >= NumIds() || !std::is_same<T, u32>::value) {
        LOG_ERROR(HW_GPU, "unknown Write{} {:#010X} @ {:#010X}", sizeof(data) * 8, (u32)data, addr);
        return;
    }

    regs[index] = static_cast<u32>(data);

    switch (index) {

    // Memory fills are triggered once the fill value is written.
    case GPU_REG_INDEX_WORKAROUND(memory_fill_config[0].trigger, 0x00004 + 0x3):
    case GPU_REG_INDEX_WORKAROUND(memory_fill_config[1].trigger, 0x00008 + 0x3): {
        const bool is_second_filler = (index != GPU_REG_INDEX(memory_fill_config[0].trigger));
        auto& config = regs.memory_fill_config[is_second_filler];
        const PAddr start_addr = DecodeAddressRegister(config.address_start);
        const PAddr end_addr = DecodeAddressRegister(config.address_end);

        if (config.trigger) {
            MemoryFill(config);
            LOG_TRACE(HW_GPU, "MemoryFill from {:#010X} to {:#010X}", start_addr, end_addr);

            // It seems that it won't signal interrupt if "address_start" is zero.
            // TODO: hwtest this
            if (start_addr != 0) {
                if (!is_second_filler) {
                    system.VideoCore().SignalInterrupt(Service::GSP::InterruptId::PSC0);
                } else {
                    system.VideoCore().SignalInterrupt(Service::GSP::InterruptId::PSC1);
                }
            }

            // Reset "trigger" flag and set the "finish" flag
            // NOTE: This was confirmed to happen on hardware even if "address_start" is zero.
            config.trigger.Assign(0);
            config.finished.Assign(1);
        }
        break;
    }

    case GPU_REG_INDEX(display_transfer_config.trigger): {
        MICROPROFILE_SCOPE(GPU_DisplayTransfer);

        const auto& config = regs.display_transfer_config;
        const PAddr input_addr = DecodeAddressRegister(config.input_address);
        const PAddr output_addr = DecodeAddressRegister(config.output_address);
        if (config.trigger & 1) {

            const auto debug_context = &system.DebuggerManager().PicaDebugContext();
            if (debug_context)
                debug_context->OnEvent(Pica::DebugContext::Event::IncomingDisplayTransfer, nullptr);

            if (config.is_texture_copy) {
                TextureCopy(config);
                LOG_TRACE(HW_GPU,
                          "TextureCopy: {:#X} bytes from {:#010X}({}+{})-> "
                          "{:#010X}({}+{}), flags {:#010X}",
                          config.texture_copy.size, input_addr,
                          config.texture_copy.input_width * 16, config.texture_copy.input_gap * 16,
                          output_addr, config.texture_copy.output_width * 16,
                          config.texture_copy.output_gap * 16, config.flags);
            } else {
                DisplayTransfer(config);
                LOG_TRACE(HW_GPU,
                          "DisplayTransfer: {:#010X}({}x{})-> "
                          "{:#010X}({}x{}), dst format {:x}, flags {:#010X}",
                          input_addr, config.input_width.Value(), config.input_height.Value(),
                          output_addr, config.output_width.Value(), config.output_height.Value(),
                          static_cast<u32>(config.output_format.Value()), config.flags);
            }

            regs.display_transfer_config.trigger = 0;
            system.VideoCore().SignalInterrupt(Service::GSP::InterruptId::PPF);
        }
        break;
    }

    // Seems like writing to this register triggers processing
    case GPU_REG_INDEX(command_processor_config.trigger): {
        const auto& config = regs.command_processor_config;
        const PAddr address = DecodeAddressRegister(config.address);
        if (config.trigger & 1) {
            MICROPROFILE_SCOPE(GPU_CmdlistProcessing);

            const u32* buffer = reinterpret_cast<u32*>(system.Memory().GetPhysicalPointer(address));

            const auto debug_context = &system.DebuggerManager().PicaDebugContext();
            if (debug_context && debug_context->recorder) {
                debug_context->recorder->MemoryAccessed(reinterpret_cast<const u8*>(buffer),
                                                        config.size, address);
            }

            system.HardwareManager().Pica().ProcessCommandList(buffer, config.size);

            regs.command_processor_config.trigger = 0;
        }
        break;
    }

    default:
        break;
    }

    // Notify tracer about the register write
    // This is happening *after* handling the write to make sure we properly catch all memory reads.
    const auto debug_context = &system.DebuggerManager().PicaDebugContext();
    if (debug_context && debug_context->recorder) {
        // addr + GPU VBase - IO VBase + IO PBase
        debug_context->recorder->RegisterWritten<T>(addr + VADDR_GPU - VADDR_BASE + PADDR_BASE,
                                                    data);
    }
}

template void Gpu::Read<u64>(u64& var, const u32 addr);
template void Gpu::Read<u32>(u32& var, const u32 addr);
template void Gpu::Read<u16>(u16& var, const u32 addr);
template void Gpu::Read<u8>(u8& var, const u32 addr);

template void Gpu::Write<u64>(u32 addr, const u64 data);
template void Gpu::Write<u32>(u32 addr, const u32 data);
template void Gpu::Write<u16>(u32 addr, const u16 data);
template void Gpu::Write<u8>(u32 addr, const u8 data);

bool Gpu::IsValidAddress(VAddr addr) {
    if (addr < VADDR_GPU || addr >= VADDR_GPU + 0x10000)
        return false;
    return true;
};

u8 Gpu::Read8(VAddr addr) {
    u8 var;
    Read<u8>(var, addr);
    return var;
};

u16 Gpu::Read16(VAddr addr) {
    u16 var;
    Read<u16>(var, addr);
    return var;
};

u32 Gpu::Read32(VAddr addr) {
    u32 var;
    Read<u32>(var, addr);
    return var;
};

u64 Gpu::Read64(VAddr addr) {
    u64 var;
    Read<u64>(var, addr);
    return var;
};

bool Gpu::ReadBlock(VAddr src_addr, void* dest_buffer, std::size_t size) {
    if (IsValidAddress(src_addr)) {
        src_addr -= VADDR_GPU;
        std::memcpy(dest_buffer, &regs + src_addr, size);
        return true;
    }
    return false;
};

void Gpu::Write8(VAddr addr, u8 data) {
    Write<u8>(addr, data);
};

void Gpu::Write16(VAddr addr, u16 data) {
    Write<u16>(addr, data);
};

void Gpu::Write32(VAddr addr, u32 data) {
    Write<u32>(addr, data);
};

void Gpu::Write64(VAddr addr, u64 data) {
    Write<u64>(addr, data);
};

bool Gpu::WriteBlock(VAddr dest_addr, const void* src_buffer, std::size_t size) {
    if (IsValidAddress(dest_addr)) {
        dest_addr -= VADDR_GPU;
        std::memcpy(&regs + dest_addr, src_buffer, size);
    }
    return false;
};

HW::GPU::Regs& Gpu::Regs() {
    return regs;
};

const HW::GPU::Regs& Gpu::Regs() const {
    return regs;
};

} // namespace HW::GPU
