// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <array>
#include <cstddef>
#include <memory>
#include <utility>
#include "common/assert.h"
#include "common/logging/log.h"
#include "common/microprofile.h"
#include "core/core.h"
#include "core/hle/service/gsp/gsp.h"
#include "core/hw/hw.h"
#include "core/hw/pica.h"
#include "core/hw/pica/command_processor.h"
#include "core/hw/pica/pica_state.h"
#include "core/hw/pica/pica_types.h"
#include "core/hw/pica/regs.h"
#include "core/hw/pica/regs_pipeline.h"
#include "core/hw/pica/regs_texturing.h"
#include "core/memory.h"
#include "core/tracer/recorder.h"
#include "video_core/debugger/debugger.h"
#include "video_core/primitive_assembly.h"
#include "video_core/rasterizer_interface.h"
#include "video_core/renderer_base.h"
#include "video_core/shader/shader.h"
#include "video_core/vertex_loader.h"
#include "video_core/video_core.h"

namespace Pica {

namespace CommandProcessor {

MICROPROFILE_DEFINE(GPU_Drawing, "GPU", "Drawing", MP_RGB(50, 50, 240));

CommandProcessor::CommandProcessor(Core::System& system) : system(system) {}

const char* CommandProcessor::GetShaderSetupTypeName(const Shader::ShaderSetup& setup) {
    const auto& pica_state = system.HardwareManager().Pica().State();
    if (&setup == &pica_state.vs) {
        return "vertex shader";
    }
    if (&setup == &pica_state.gs) {
        return "geometry shader";
    }
    return "unknown shader";
}

void CommandProcessor::WriteUniformBoolReg(Shader::ShaderSetup& setup, const u32 value) {
    for (unsigned i = 0; i < setup.uniforms.b.size(); ++i)
        setup.uniforms.b[i] = (value & (1 << i)) != 0;
}

void CommandProcessor::WriteUniformIntReg(Shader::ShaderSetup& setup, const unsigned index,
                                          const Math::Vec4<u8>& values) {
    ASSERT(index < setup.uniforms.i.size());
    setup.uniforms.i[index] = values;
    LOG_TRACE(HW_GPU, "Set {} integer uniform {} to {:02x} {:02x} {:02x} {:02x}",
              GetShaderSetupTypeName(setup), index, values.x, values.y, values.z, values.w);
}

void CommandProcessor::WriteUniformFloatReg(ShaderRegs& config, Shader::ShaderSetup& setup,
                                            int& float_regs_counter, u32 uniform_write_buffer[4],
                                            const u32 value) {
    auto& uniform_setup = config.uniform_setup;

    // TODO: Does actual hardware indeed keep an intermediate buffer or does
    //       it directly write the values?
    uniform_write_buffer[float_regs_counter++] = value;

    // Uniforms are written in a packed format such that four float24 values are encoded in
    // three 32-bit numbers. We write to internal memory once a full such vector is
    // written.
    if ((float_regs_counter >= 4 && uniform_setup.IsFloat32()) ||
        (float_regs_counter >= 3 && !uniform_setup.IsFloat32())) {
        float_regs_counter = 0;

        auto& uniform = setup.uniforms.f[uniform_setup.index];

        if (uniform_setup.index >= 96) {
            LOG_TRACE(HW_GPU, "Invalid {} float uniform index {}", GetShaderSetupTypeName(setup),
                      static_cast<int>(uniform_setup.index));
        } else {

            // NOTE: The destination component order indeed is "backwards"
            if (uniform_setup.IsFloat32()) {
                for (auto i : {0, 1, 2, 3})
                    uniform[3 - i] = float24::FromFloat32(*(float*)(&uniform_write_buffer[i]));
            } else {
                // TODO: Untested
                uniform.w = float24::FromRaw(uniform_write_buffer[0] >> 8);
                uniform.z = float24::FromRaw(((uniform_write_buffer[0] & 0xFF) << 16) |
                                             ((uniform_write_buffer[1] >> 16) & 0xFFFF));
                uniform.y = float24::FromRaw(((uniform_write_buffer[1] & 0xFFFF) << 8) |
                                             ((uniform_write_buffer[2] >> 24) & 0xFF));
                uniform.x = float24::FromRaw(uniform_write_buffer[2] & 0xFFFFFF);
            }

            LOG_TRACE(HW_GPU, "Set {} float uniform {:x} to ({} {} {} {})",
                      GetShaderSetupTypeName(setup), static_cast<int>(uniform_setup.index),
                      uniform.x.ToFloat32(), uniform.y.ToFloat32(), uniform.z.ToFloat32(),
                      uniform.w.ToFloat32());

            // TODO: Verify that this actually modifies the register!
            uniform_setup.index.Assign(uniform_setup.index + 1);
        }
    }
}

void CommandProcessor::WritePicaReg(u32 id, const u32 value, const u32 mask) {
    auto& video_core = system.VideoCore();
    auto& pica_state = system.HardwareManager().Pica().State();
    auto& regs = pica_state.regs;
    const auto& settings = video_core.Settings();
    const auto rasterizer = video_core.Renderer().Rasterizer();
    const auto debug_context = &system.DebuggerManager().PicaDebugContext();
    auto& pica_tracer = system.DebuggerManager().PicaTracer();

    if (id >= Regs::NUM_REGS) {
        LOG_ERROR(
            HW_GPU,
            "Commandlist tried to write to invalid register 0x{:03X} (value: {:08X}, mask: {:X})",
            id, value, mask);
        return;
    }

    // TODO: Figure out how register masking acts on e.g. vs.uniform_setup.set_value
    const u32 old_value = regs.reg_array[id];
    const u32 write_mask = expand_bits_to_bytes[mask];

    regs.reg_array[id] = (old_value & ~write_mask) | (value & write_mask);

    // Double check for is_pica_tracing to avoid call overhead
    if (pica_tracer.IsPicaTracing()) {
        pica_tracer.OnPicaRegWrite(
            {static_cast<u16>(id), static_cast<u16>(mask), regs.reg_array[id]});
    }

    if (debug_context)
        debug_context->OnEvent(DebugContext::Event::PicaCommandLoaded,
                               reinterpret_cast<void*>(&id));

    switch (id) {
    // Trigger IRQ
    case PICA_REG_INDEX(trigger_irq):
        video_core.SignalInterrupt(Service::GSP::InterruptId::P3D);
        break;

    case PICA_REG_INDEX(pipeline.triangle_topology):
        pica_state.primitive_assembler.Reconfigure(regs.pipeline.triangle_topology);
        break;

    case PICA_REG_INDEX(pipeline.restart_primitive):
        pica_state.primitive_assembler.Reset();
        break;

    case PICA_REG_INDEX(pipeline.vs_default_attributes_setup.index):
        pica_state.immediate.current_attribute = 0;
        pica_state.immediate.reset_geometry_pipeline = true;
        default_attr_counter = 0;
        break;

    // Load default vertex input attributes
    case PICA_REG_INDEX_WORKAROUND(pipeline.vs_default_attributes_setup.set_value[0], 0x233):
    case PICA_REG_INDEX_WORKAROUND(pipeline.vs_default_attributes_setup.set_value[1], 0x234):
    case PICA_REG_INDEX_WORKAROUND(pipeline.vs_default_attributes_setup.set_value[2], 0x235): {
        // TODO: Does actual hardware indeed keep an intermediate buffer or does
        //       it directly write the values?
        default_attr_write_buffer[default_attr_counter++] = value;

        // Default attributes are written in a packed format such that four float24 values are
        // encoded in
        // three 32-bit numbers. We write to internal memory once a full such vector is
        // written.
        if (default_attr_counter >= 3) {
            default_attr_counter = 0;

            auto& setup = regs.pipeline.vs_default_attributes_setup;

            if (setup.index >= 16) {
                LOG_ERROR(HW_GPU, "Invalid VS default attribute index {}",
                          static_cast<int>(setup.index));
                break;
            }

            Math::Vec4<float24> attribute;

            // NOTE: The destination component order indeed is "backwards"
            attribute.w = float24::FromRaw(default_attr_write_buffer[0] >> 8);
            attribute.z = float24::FromRaw(((default_attr_write_buffer[0] & 0xFF) << 16) |
                                           ((default_attr_write_buffer[1] >> 16) & 0xFFFF));
            attribute.y = float24::FromRaw(((default_attr_write_buffer[1] & 0xFFFF) << 8) |
                                           ((default_attr_write_buffer[2] >> 24) & 0xFF));
            attribute.x = float24::FromRaw(default_attr_write_buffer[2] & 0xFFFFFF);

            LOG_TRACE(HW_GPU, "Set default VS attribute {:x} to ({} {} {} {})",
                      static_cast<int>(setup.index), attribute.x.ToFloat32(),
                      attribute.y.ToFloat32(), attribute.z.ToFloat32(), attribute.w.ToFloat32());

            // TODO: Verify that this actually modifies the register!
            if (setup.index < 15) {
                pica_state.input_default_attributes.attr[setup.index] = attribute;
                setup.index++;
            } else {
                // Put each attribute into an immediate input buffer.  When all specified immediate
                // attributes are present, the Vertex Shader is invoked and everything is sent to
                // the primitive assembler.

                auto& immediate_input = pica_state.immediate.input_vertex;
                auto& immediate_attribute_id = pica_state.immediate.current_attribute;

                immediate_input.attr[immediate_attribute_id] = attribute;

                if (immediate_attribute_id < regs.pipeline.max_input_attrib_index) {
                    immediate_attribute_id += 1;
                } else {
                    MICROPROFILE_SCOPE(GPU_Drawing);
                    immediate_attribute_id = 0;

                    Shader::OutputVertex::ValidateSemantics(regs.rasterizer);

                    auto* shader_engine = Shader::GetEngine();
                    shader_engine->SetupBatch(pica_state.vs, regs.vs.main_offset);

                    // Send to vertex shader
                    if (debug_context)
                        debug_context->OnEvent(DebugContext::Event::VertexShaderInvocation,
                                               reinterpret_cast<void*>(&immediate_input));
                    Shader::UnitState shader_unit;
                    Shader::AttributeBuffer output{};

                    shader_unit.LoadInput(regs.vs, immediate_input);
                    shader_engine->Run(pica_state.vs, shader_unit);
                    shader_unit.WriteOutput(regs.vs, output);

                    // Send to geometry pipeline
                    if (pica_state.immediate.reset_geometry_pipeline) {
                        pica_state.geometry_pipeline.Reconfigure();
                        pica_state.immediate.reset_geometry_pipeline = false;
                    }
                    ASSERT(!pica_state.geometry_pipeline.NeedIndexInput());
                    pica_state.geometry_pipeline.Setup(shader_engine);
                    pica_state.geometry_pipeline.SubmitVertex(output);

                    // TODO: If drawing after every immediate mode triangle kills performance,
                    // change it to flush triangles whenever a drawing config register changes
                    // See: https://github.com/citra-emu/citra/pull/2866#issuecomment-327011550
                    rasterizer->DrawTriangles();
                    if (debug_context) {
                        debug_context->OnEvent(DebugContext::Event::FinishedPrimitiveBatch,
                                               nullptr);
                    }
                }
            }
        }
        break;
    }

    case PICA_REG_INDEX(pipeline.gpu_mode):
        // This register likely just enables vertex processing and doesn't need any special handling
        break;

    case PICA_REG_INDEX_WORKAROUND(pipeline.command_buffer.trigger[0], 0x23c):
    case PICA_REG_INDEX_WORKAROUND(pipeline.command_buffer.trigger[1], 0x23d): {
        const unsigned index =
            static_cast<unsigned>(id - PICA_REG_INDEX(pipeline.command_buffer.trigger[0]));
        u32* head_ptr = reinterpret_cast<u32*>(system.Memory().GetPhysicalPointer(
            regs.pipeline.command_buffer.GetPhysicalAddress(index)));
        pica_state.cmd_list.head_ptr = pica_state.cmd_list.current_ptr = head_ptr;
        pica_state.cmd_list.length = regs.pipeline.command_buffer.GetSize(index) / sizeof(u32);
        break;
    }

    // It seems like these trigger vertex rendering
    case PICA_REG_INDEX(pipeline.trigger_draw):
    case PICA_REG_INDEX(pipeline.trigger_draw_indexed): {
        MICROPROFILE_SCOPE(GPU_Drawing);

#if PICA_LOG_TEV
        DebugUtils::DumpTevStageConfig(regs.GetTevStages());
#endif
        if (debug_context)
            debug_context->OnEvent(DebugContext::Event::IncomingPrimitiveBatch, nullptr);

        const PrimitiveAssembler<Shader::OutputVertex>& primitive_assembler =
            pica_state.primitive_assembler;

        bool accelerate_draw = settings.hw_shader_enabled && primitive_assembler.IsEmpty();

        if (regs.pipeline.use_gs == PipelineRegs::UseGS::No) {
            const auto topology = primitive_assembler.GetTopology();
            if (topology == PipelineRegs::TriangleTopology::Shader ||
                topology == PipelineRegs::TriangleTopology::List) {
                accelerate_draw = accelerate_draw && (regs.pipeline.num_vertices % 3) == 0;
            }
            // TODO (wwylele): for Strip/Fan topology, if the primitive assember is not restarted
            // after this draw call, the buffered vertex from this draw should "leak" to the next
            // draw, in which case we should buffer the vertex into the software primitive assember,
            // or disable accelerate draw completely. However, there is not game found yet that does
            // this, so this is left unimplemented for now. Revisit this when an issue is found in
            // games.
        } else {
            if (settings.hw_shader_accurate_gs) {
                accelerate_draw = false;
            }
        }

        const bool is_indexed = (id == PICA_REG_INDEX(pipeline.trigger_draw_indexed));

        if (accelerate_draw && rasterizer->AccelerateDrawBatch(is_indexed)) {
            if (debug_context) {
                debug_context->OnEvent(DebugContext::Event::FinishedPrimitiveBatch, nullptr);
            }
            break;
        }

        // Processes information about internal vertex attributes to figure out how a vertex is
        // loaded.
        // Later, these can be compiled and cached.
        const u32 base_address = regs.pipeline.vertex_attributes.GetPhysicalBaseAddress();
        VertexLoader loader(regs.pipeline);
        Shader::OutputVertex::ValidateSemantics(regs.rasterizer);

        // Load vertices
        const auto& index_info = regs.pipeline.index_array;
        const u8* index_address_8 =
            system.Memory().GetPhysicalPointer(base_address + index_info.offset);
        const u16* index_address_16 = reinterpret_cast<const u16*>(index_address_8);
        const bool index_u16 = index_info.format != 0;

        if (debug_context && debug_context->recorder) {
            for (int i = 0; i < 3; ++i) {
                const auto texture = regs.texturing.GetTextures()[i];
                if (!texture.enabled)
                    continue;

                const u8* texture_data = system.Memory().GetPhysicalPointer(texture.config.GetPhysicalAddress());
                debug_context->recorder->MemoryAccessed(
                    texture_data,
                    TexturingRegs::NibblesPerPixel(texture.format) * texture.config.width / 2 *
                        texture.config.height,
                    texture.config.GetPhysicalAddress());
            }
        }

        DebugUtils::MemoryAccessTracker memory_accesses;

        // Simple circular-replacement vertex cache
        // The size has been tuned for optimal balance between hit-rate and the cost of lookup
        const std::size_t VERTEX_CACHE_SIZE = 32;
        std::array<bool, VERTEX_CACHE_SIZE> vertex_cache_valid{};
        std::array<u16, VERTEX_CACHE_SIZE> vertex_cache_ids;
        std::array<Shader::AttributeBuffer, VERTEX_CACHE_SIZE> vertex_cache;
        Shader::AttributeBuffer vs_output;

        unsigned int vertex_cache_pos = 0;

        auto* shader_engine = Shader::GetEngine();
        Shader::UnitState shader_unit;

        shader_engine->SetupBatch(pica_state.vs, regs.vs.main_offset);

        pica_state.geometry_pipeline.Reconfigure();
        pica_state.geometry_pipeline.Setup(shader_engine);
        if (pica_state.geometry_pipeline.NeedIndexInput())
            ASSERT(is_indexed);

        for (unsigned int index = 0; index < regs.pipeline.num_vertices; ++index) {
            // Indexed rendering doesn't use the start offset
            const unsigned int vertex =
                is_indexed ? (index_u16 ? index_address_16[index] : index_address_8[index])
                           : (index + regs.pipeline.vertex_offset);

            bool vertex_cache_hit = false;

            if (is_indexed) {
                if (pica_state.geometry_pipeline.NeedIndexInput()) {
                    pica_state.geometry_pipeline.SubmitIndex(vertex);
                    continue;
                }

                if (debug_context && debug_context->recorder) {
                    const int size = index_u16 ? 2 : 1;
                    memory_accesses.AddAccess(base_address + index_info.offset + size * index,
                                              size);
                }

                for (unsigned int i = 0; i < VERTEX_CACHE_SIZE; ++i) {
                    if (vertex_cache_valid[i] && vertex == vertex_cache_ids[i]) {
                        vs_output = vertex_cache[i];
                        vertex_cache_hit = true;
                        break;
                    }
                }
            }

            if (!vertex_cache_hit) {
                // Initialize data for the current vertex
                Shader::AttributeBuffer input;
                loader.LoadVertex(base_address, index, vertex, input, memory_accesses);

                // Send to vertex shader
                if (debug_context)
                    debug_context->OnEvent(DebugContext::Event::VertexShaderInvocation,
                                           reinterpret_cast<void*>(&input));
                shader_unit.LoadInput(regs.vs, input);
                shader_engine->Run(pica_state.vs, shader_unit);
                shader_unit.WriteOutput(regs.vs, vs_output);

                if (is_indexed) {
                    vertex_cache[vertex_cache_pos] = vs_output;
                    vertex_cache_valid[vertex_cache_pos] = true;
                    vertex_cache_ids[vertex_cache_pos] = vertex;
                    vertex_cache_pos = (vertex_cache_pos + 1) % VERTEX_CACHE_SIZE;
                }
            }

            // Send to geometry pipeline
            pica_state.geometry_pipeline.SubmitVertex(vs_output);
        }

        for (auto& range : memory_accesses.ranges) {
            debug_context->recorder->MemoryAccessed(system.Memory().GetPhysicalPointer(range.first),
                                                      range.second, range.first);
        }

        rasterizer->DrawTriangles();
        if (debug_context) {
            debug_context->OnEvent(DebugContext::Event::FinishedPrimitiveBatch, nullptr);
        }

        break;
    }

    case PICA_REG_INDEX(gs.bool_uniforms):
        WriteUniformBoolReg(pica_state.gs, pica_state.regs.gs.bool_uniforms.Value());
        break;

    case PICA_REG_INDEX_WORKAROUND(gs.int_uniforms[0], 0x281):
    case PICA_REG_INDEX_WORKAROUND(gs.int_uniforms[1], 0x282):
    case PICA_REG_INDEX_WORKAROUND(gs.int_uniforms[2], 0x283):
    case PICA_REG_INDEX_WORKAROUND(gs.int_uniforms[3], 0x284): {
        const unsigned index = (id - PICA_REG_INDEX_WORKAROUND(gs.int_uniforms[0], 0x281));
        const auto values = regs.gs.int_uniforms[index];
        WriteUniformIntReg(pica_state.gs, index,
                           Math::Vec4<u8>(values.x, values.y, values.z, values.w));
        break;
    }

    case PICA_REG_INDEX_WORKAROUND(gs.uniform_setup.set_value[0], 0x291):
    case PICA_REG_INDEX_WORKAROUND(gs.uniform_setup.set_value[1], 0x292):
    case PICA_REG_INDEX_WORKAROUND(gs.uniform_setup.set_value[2], 0x293):
    case PICA_REG_INDEX_WORKAROUND(gs.uniform_setup.set_value[3], 0x294):
    case PICA_REG_INDEX_WORKAROUND(gs.uniform_setup.set_value[4], 0x295):
    case PICA_REG_INDEX_WORKAROUND(gs.uniform_setup.set_value[5], 0x296):
    case PICA_REG_INDEX_WORKAROUND(gs.uniform_setup.set_value[6], 0x297):
    case PICA_REG_INDEX_WORKAROUND(gs.uniform_setup.set_value[7], 0x298): {
        WriteUniformFloatReg(pica_state.regs.gs, pica_state.gs, gs_float_regs_counter,
                             gs_uniform_write_buffer, value);
        break;
    }

    case PICA_REG_INDEX_WORKAROUND(gs.program.set_word[0], 0x29c):
    case PICA_REG_INDEX_WORKAROUND(gs.program.set_word[1], 0x29d):
    case PICA_REG_INDEX_WORKAROUND(gs.program.set_word[2], 0x29e):
    case PICA_REG_INDEX_WORKAROUND(gs.program.set_word[3], 0x29f):
    case PICA_REG_INDEX_WORKAROUND(gs.program.set_word[4], 0x2a0):
    case PICA_REG_INDEX_WORKAROUND(gs.program.set_word[5], 0x2a1):
    case PICA_REG_INDEX_WORKAROUND(gs.program.set_word[6], 0x2a2):
    case PICA_REG_INDEX_WORKAROUND(gs.program.set_word[7], 0x2a3): {
        u32& offset = pica_state.regs.gs.program.offset;
        if (offset >= 4096) {
            LOG_ERROR(HW_GPU, "Invalid GS program offset {}", offset);
        } else {
            pica_state.gs.program_code[offset] = value;
            pica_state.gs.MarkProgramCodeDirty();
            offset++;
        }
        break;
    }

    case PICA_REG_INDEX_WORKAROUND(gs.swizzle_patterns.set_word[0], 0x2a6):
    case PICA_REG_INDEX_WORKAROUND(gs.swizzle_patterns.set_word[1], 0x2a7):
    case PICA_REG_INDEX_WORKAROUND(gs.swizzle_patterns.set_word[2], 0x2a8):
    case PICA_REG_INDEX_WORKAROUND(gs.swizzle_patterns.set_word[3], 0x2a9):
    case PICA_REG_INDEX_WORKAROUND(gs.swizzle_patterns.set_word[4], 0x2aa):
    case PICA_REG_INDEX_WORKAROUND(gs.swizzle_patterns.set_word[5], 0x2ab):
    case PICA_REG_INDEX_WORKAROUND(gs.swizzle_patterns.set_word[6], 0x2ac):
    case PICA_REG_INDEX_WORKAROUND(gs.swizzle_patterns.set_word[7], 0x2ad): {
        u32& offset = pica_state.regs.gs.swizzle_patterns.offset;
        if (offset >= pica_state.gs.swizzle_data.size()) {
            LOG_ERROR(HW_GPU, "Invalid GS swizzle pattern offset {}", offset);
        } else {
            pica_state.gs.swizzle_data[offset] = value;
            pica_state.gs.MarkSwizzleDataDirty();
            offset++;
        }
        break;
    }

    case PICA_REG_INDEX(vs.bool_uniforms):
        // TODO (wwylele): does regs.pipeline.gs_unit_exclusive_configuration affect this?
        WriteUniformBoolReg(pica_state.vs, pica_state.regs.vs.bool_uniforms.Value());
        break;

    case PICA_REG_INDEX_WORKAROUND(vs.int_uniforms[0], 0x2b1):
    case PICA_REG_INDEX_WORKAROUND(vs.int_uniforms[1], 0x2b2):
    case PICA_REG_INDEX_WORKAROUND(vs.int_uniforms[2], 0x2b3):
    case PICA_REG_INDEX_WORKAROUND(vs.int_uniforms[3], 0x2b4): {
        // TODO (wwylele): does regs.pipeline.gs_unit_exclusive_configuration affect this?
        const unsigned index = (id - PICA_REG_INDEX_WORKAROUND(vs.int_uniforms[0], 0x2b1));
        const auto values = regs.vs.int_uniforms[index];
        WriteUniformIntReg(pica_state.vs, index,
                           Math::Vec4<u8>(values.x, values.y, values.z, values.w));
        break;
    }

    case PICA_REG_INDEX_WORKAROUND(vs.uniform_setup.set_value[0], 0x2c1):
    case PICA_REG_INDEX_WORKAROUND(vs.uniform_setup.set_value[1], 0x2c2):
    case PICA_REG_INDEX_WORKAROUND(vs.uniform_setup.set_value[2], 0x2c3):
    case PICA_REG_INDEX_WORKAROUND(vs.uniform_setup.set_value[3], 0x2c4):
    case PICA_REG_INDEX_WORKAROUND(vs.uniform_setup.set_value[4], 0x2c5):
    case PICA_REG_INDEX_WORKAROUND(vs.uniform_setup.set_value[5], 0x2c6):
    case PICA_REG_INDEX_WORKAROUND(vs.uniform_setup.set_value[6], 0x2c7):
    case PICA_REG_INDEX_WORKAROUND(vs.uniform_setup.set_value[7], 0x2c8): {
        // TODO (wwylele): does regs.pipeline.gs_unit_exclusive_configuration affect this?
        WriteUniformFloatReg(pica_state.regs.vs, pica_state.vs, vs_float_regs_counter,
                             vs_uniform_write_buffer, value);
        break;
    }

    case PICA_REG_INDEX_WORKAROUND(vs.program.set_word[0], 0x2cc):
    case PICA_REG_INDEX_WORKAROUND(vs.program.set_word[1], 0x2cd):
    case PICA_REG_INDEX_WORKAROUND(vs.program.set_word[2], 0x2ce):
    case PICA_REG_INDEX_WORKAROUND(vs.program.set_word[3], 0x2cf):
    case PICA_REG_INDEX_WORKAROUND(vs.program.set_word[4], 0x2d0):
    case PICA_REG_INDEX_WORKAROUND(vs.program.set_word[5], 0x2d1):
    case PICA_REG_INDEX_WORKAROUND(vs.program.set_word[6], 0x2d2):
    case PICA_REG_INDEX_WORKAROUND(vs.program.set_word[7], 0x2d3): {
        u32& offset = pica_state.regs.vs.program.offset;
        if (offset >= 512) {
            LOG_ERROR(HW_GPU, "Invalid VS program offset {}", offset);
        } else {
            pica_state.vs.program_code[offset] = value;
            pica_state.vs.MarkProgramCodeDirty();
            if (!pica_state.regs.pipeline.gs_unit_exclusive_configuration) {
                pica_state.gs.program_code[offset] = value;
                pica_state.gs.MarkProgramCodeDirty();
            }
            offset++;
        }
        break;
    }

    case PICA_REG_INDEX_WORKAROUND(vs.swizzle_patterns.set_word[0], 0x2d6):
    case PICA_REG_INDEX_WORKAROUND(vs.swizzle_patterns.set_word[1], 0x2d7):
    case PICA_REG_INDEX_WORKAROUND(vs.swizzle_patterns.set_word[2], 0x2d8):
    case PICA_REG_INDEX_WORKAROUND(vs.swizzle_patterns.set_word[3], 0x2d9):
    case PICA_REG_INDEX_WORKAROUND(vs.swizzle_patterns.set_word[4], 0x2da):
    case PICA_REG_INDEX_WORKAROUND(vs.swizzle_patterns.set_word[5], 0x2db):
    case PICA_REG_INDEX_WORKAROUND(vs.swizzle_patterns.set_word[6], 0x2dc):
    case PICA_REG_INDEX_WORKAROUND(vs.swizzle_patterns.set_word[7], 0x2dd): {
        u32& offset = pica_state.regs.vs.swizzle_patterns.offset;
        if (offset >= pica_state.vs.swizzle_data.size()) {
            LOG_ERROR(HW_GPU, "Invalid VS swizzle pattern offset {}", offset);
        } else {
            pica_state.vs.swizzle_data[offset] = value;
            pica_state.vs.MarkSwizzleDataDirty();
            if (!pica_state.regs.pipeline.gs_unit_exclusive_configuration) {
                pica_state.gs.swizzle_data[offset] = value;
                pica_state.gs.MarkSwizzleDataDirty();
            }
            offset++;
        }
        break;
    }

    case PICA_REG_INDEX_WORKAROUND(lighting.lut_data[0], 0x1c8):
    case PICA_REG_INDEX_WORKAROUND(lighting.lut_data[1], 0x1c9):
    case PICA_REG_INDEX_WORKAROUND(lighting.lut_data[2], 0x1ca):
    case PICA_REG_INDEX_WORKAROUND(lighting.lut_data[3], 0x1cb):
    case PICA_REG_INDEX_WORKAROUND(lighting.lut_data[4], 0x1cc):
    case PICA_REG_INDEX_WORKAROUND(lighting.lut_data[5], 0x1cd):
    case PICA_REG_INDEX_WORKAROUND(lighting.lut_data[6], 0x1ce):
    case PICA_REG_INDEX_WORKAROUND(lighting.lut_data[7], 0x1cf): {
        auto& lut_config = regs.lighting.lut_config;

        ASSERT_MSG(lut_config.index < 256, "lut_config.index exceeded maximum value of 255!");

        pica_state.lighting.luts[lut_config.type][lut_config.index].raw = value;
        lut_config.index.Assign(lut_config.index + 1);
        break;
    }

    case PICA_REG_INDEX_WORKAROUND(texturing.fog_lut_data[0], 0xe8):
    case PICA_REG_INDEX_WORKAROUND(texturing.fog_lut_data[1], 0xe9):
    case PICA_REG_INDEX_WORKAROUND(texturing.fog_lut_data[2], 0xea):
    case PICA_REG_INDEX_WORKAROUND(texturing.fog_lut_data[3], 0xeb):
    case PICA_REG_INDEX_WORKAROUND(texturing.fog_lut_data[4], 0xec):
    case PICA_REG_INDEX_WORKAROUND(texturing.fog_lut_data[5], 0xed):
    case PICA_REG_INDEX_WORKAROUND(texturing.fog_lut_data[6], 0xee):
    case PICA_REG_INDEX_WORKAROUND(texturing.fog_lut_data[7], 0xef): {
        pica_state.fog.lut[regs.texturing.fog_lut_offset % 128].raw = value;
        regs.texturing.fog_lut_offset.Assign(regs.texturing.fog_lut_offset + 1);
        break;
    }

    case PICA_REG_INDEX_WORKAROUND(texturing.proctex_lut_data[0], 0xb0):
    case PICA_REG_INDEX_WORKAROUND(texturing.proctex_lut_data[1], 0xb1):
    case PICA_REG_INDEX_WORKAROUND(texturing.proctex_lut_data[2], 0xb2):
    case PICA_REG_INDEX_WORKAROUND(texturing.proctex_lut_data[3], 0xb3):
    case PICA_REG_INDEX_WORKAROUND(texturing.proctex_lut_data[4], 0xb4):
    case PICA_REG_INDEX_WORKAROUND(texturing.proctex_lut_data[5], 0xb5):
    case PICA_REG_INDEX_WORKAROUND(texturing.proctex_lut_data[6], 0xb6):
    case PICA_REG_INDEX_WORKAROUND(texturing.proctex_lut_data[7], 0xb7): {
        auto& index = regs.texturing.proctex_lut_config.index;
        auto& pt = pica_state.proctex;

        switch (regs.texturing.proctex_lut_config.ref_table.Value()) {
        case TexturingRegs::ProcTexLutTable::Noise:
            pt.noise_table[index % pt.noise_table.size()].raw = value;
            break;
        case TexturingRegs::ProcTexLutTable::ColorMap:
            pt.color_map_table[index % pt.color_map_table.size()].raw = value;
            break;
        case TexturingRegs::ProcTexLutTable::AlphaMap:
            pt.alpha_map_table[index % pt.alpha_map_table.size()].raw = value;
            break;
        case TexturingRegs::ProcTexLutTable::Color:
            pt.color_table[index % pt.color_table.size()].raw = value;
            break;
        case TexturingRegs::ProcTexLutTable::ColorDiff:
            pt.color_diff_table[index % pt.color_diff_table.size()].raw = value;
            break;
        }
        index.Assign(index + 1);
        break;
    }
    default:
        break;
    }

    rasterizer->NotifyPicaRegisterChanged(id);

    if (debug_context)
        debug_context->OnEvent(DebugContext::Event::PicaCommandProcessed,
                               reinterpret_cast<void*>(&id));
}

void CommandProcessor::ProcessCommandList(const u32* list, const u32 size) {
    auto& pica_state = system.HardwareManager().Pica().State();

    pica_state.cmd_list.head_ptr = pica_state.cmd_list.current_ptr = list;
    pica_state.cmd_list.length = size / sizeof(u32);

    while (pica_state.cmd_list.current_ptr <
           pica_state.cmd_list.head_ptr + pica_state.cmd_list.length) {

        // Align read pointer to 8 bytes
        if ((pica_state.cmd_list.head_ptr - pica_state.cmd_list.current_ptr) % 2 != 0)
            ++pica_state.cmd_list.current_ptr;

        const u32 value = *pica_state.cmd_list.current_ptr++;
        const CommandHeader header = {*pica_state.cmd_list.current_ptr++};

        WritePicaReg(header.cmd_id, value, header.parameter_mask);

        for (unsigned i = 0; i < header.extra_data_length; ++i) {
            const u32 cmd = header.cmd_id + (header.group_commands ? i + 1 : 0);
            WritePicaReg(cmd, *pica_state.cmd_list.current_ptr++, header.parameter_mask);
        }
    }
}

} // namespace CommandProcessor

} // namespace Pica
