// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "video_core/pica_state.h"
#include "video_core/regs_texturing.h"

namespace Pica {

class Pica {
public:
    explicit Pica();
    ~Pica();

    void Init();
    void Shutdown();

    ::Pica::State& State();
    const ::Pica::State& State() const;

private:
    ::Pica::State state;
};

} // namespace Pica
