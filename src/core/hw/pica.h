// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hw/pica/command_processor.h"
#include "core/hw/pica/pica_state.h"

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
