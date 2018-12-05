// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "video_core/pica/command_processor.h"
#include "video_core/pica_state.h"
#include "video_core/regs_texturing.h"

namespace Core {
class System;
}

namespace Pica {

class Pica {
public:
    explicit Pica(Core::System& system);
    ~Pica();

    void Init();
    void Shutdown();

    void ProcessCommandList(const u32* list, const u32 size);

    ::Pica::State& State();
    const ::Pica::State& State() const;

private:
    Core::System& system;
    std::unique_ptr<CommandProcessor::CommandProcessor> command_processor;
    ::Pica::State state;
};

} // namespace Pica
