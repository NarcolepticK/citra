// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cstring>
#include "common/logging/log.h"
#include "core/core.h"
#include "core/hw/pica.h"
#include "video_core/geometry_pipeline.h"
#include "video_core/renderer_base.h"
#include "video_core/shader/shader.h"
#include "video_core/video_core.h"

namespace Pica {

Pica::Pica(Core::System& system) : system(system) {
    command_processor = std::make_unique<CommandProcessor::CommandProcessor>(system);
}
Pica::~Pica() {}

void Pica::Init() {
    state.Reset();
    LOG_DEBUG(HW_PICA, "initialized OK");
}

void Pica::Shutdown() {
    Shader::Shutdown();
    LOG_DEBUG(HW_PICA, "shutdown OK");
}

void Pica::ProcessCommandList(const u32* list, const u32 size) {
    command_processor->ProcessCommandList(list, size);
}

::Pica::State& Pica::State() {
    return state;
}

const ::Pica::State& Pica::State() const {
    return state;
}

State::State() : geometry_pipeline(*this) {
    auto SubmitVertex = [this](const Shader::AttributeBuffer& vertex) {
        using Shader::OutputVertex;
        auto AddTriangle = [this](const OutputVertex& v0, const OutputVertex& v1,
                                  const OutputVertex& v2) {
            const auto rasterizer = Core::System::GetInstance().VideoCore().Renderer().Rasterizer();
            rasterizer->AddTriangle(v0, v1, v2);
        };
        primitive_assembler.SubmitVertex(
            Shader::OutputVertex::FromAttributeBuffer(regs.rasterizer, vertex), AddTriangle);
    };

    auto SetWinding = [this]() { primitive_assembler.SetWinding(); };

    gs_unit.SetVertexHandler(SubmitVertex, SetWinding);
    geometry_pipeline.SetVertexHandler(SubmitVertex);
}

template <typename T>
void Zero(T& o) {
    memset(&o, 0, sizeof(o));
}

void State::Reset() {
    Zero(regs);
    Zero(vs);
    Zero(gs);
    Zero(cmd_list);
    Zero(immediate);
    primitive_assembler.Reconfigure(PipelineRegs::TriangleTopology::List);
}
} // namespace Pica
