// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <type_traits>
#include "common/bit_field.h"
#include "common/common_types.h"
#include "common/vector_math.h"
#include "video_core/regs_shader.h"

namespace Core {
class System;
} // namespace Core

namespace Pica {

namespace Shader {
struct ShaderSetup;
} // namespace Shader

namespace CommandProcessor {

union CommandHeader {
    u32 hex;

    // parameter_mask:
    // Mask applied to the input value to make it possible to update
    // parts of a register without overwriting its other fields.
    // first bit:  0x000000FF
    // second bit: 0x0000FF00
    // third bit:  0x00FF0000
    // fourth bit: 0xFF000000
    BitField<0, 16, u32> cmd_id;
    BitField<16, 4, u32> parameter_mask;
    BitField<20, 11, u32> extra_data_length;
    BitField<31, 1, u32> group_commands;
};
static_assert(std::is_standard_layout<CommandHeader>::value == true,
              "CommandHeader does not use standard layout");
static_assert(sizeof(CommandHeader) == sizeof(u32), "CommandHeader has incorrect size!");

class CommandProcessor {
public:
    explicit CommandProcessor(Core::System& system);
    ~CommandProcessor() = default;

    void ProcessCommandList(const u32* list, u32 size);
private:
    const char* GetShaderSetupTypeName(const Shader::ShaderSetup& setup);
    void WriteUniformBoolReg(Shader::ShaderSetup& setup, const u32 value);
    void WriteUniformIntReg(Shader::ShaderSetup& setup, const unsigned index,
                            const Math::Vec4<u8>& values);
    void WriteUniformFloatReg(ShaderRegs& config, Shader::ShaderSetup& setup,
                              int& float_regs_counter, u32 uniform_write_buffer[4],
                              const u32 value);
    void WritePicaReg(u32 id, const u32 value, const u32 mask);

    Core::System& system;

    int vs_float_regs_counter = 0;
    int gs_float_regs_counter = 0;
    int default_attr_counter = 0;

    u32 vs_uniform_write_buffer[4];
    u32 gs_uniform_write_buffer[4];
    u32 default_attr_write_buffer[3];

    // Expand a 4-bit mask to 4-byte mask, e.g. 0b0101 -> 0x00FF00FF
    static constexpr u32 expand_bits_to_bytes[] = {
        0x00000000, 0x000000ff, 0x0000ff00, 0x0000ffff, 0x00ff0000, 0x00ff00ff, 0x00ffff00, 0x00ffffff,
        0xff000000, 0xff0000ff, 0xff00ff00, 0xff00ffff, 0xffff0000, 0xffff00ff, 0xffffff00, 0xffffffff,
    };
};

} // namespace CommandProcessor

} // namespace Pica
