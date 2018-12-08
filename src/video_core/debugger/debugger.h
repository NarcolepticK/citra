// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "video_core/debugger/debug_utils.h"
#include "video_core/debugger/gpu_debugger.h"

namespace Debugger {

class DebuggerManager {
public:
    explicit DebuggerManager();
    ~DebuggerManager() = default;

    void Reset();

    Debugger::GraphicsDebugger& GraphicsDebugger();
    const Debugger::GraphicsDebugger& GraphicsDebugger() const;

    Pica::DebugContext& PicaDebugContext();
    const Pica::DebugContext& PicaDebugContext() const;

    Pica::PicaTracer& PicaTracer();
    const Pica::PicaTracer& PicaTracer() const;

    std::shared_ptr<Debugger::GraphicsDebugger> SharedGraphicsDebugger();
    std::shared_ptr<Pica::DebugContext> SharedPicaDebugContext();
    std::shared_ptr<Pica::PicaTracer> SharedPicaTracer();

private:
    std::shared_ptr<Debugger::GraphicsDebugger> graphics_debugger;
    std::shared_ptr<Pica::DebugContext> pica_debug_context;
    std::shared_ptr<Pica::PicaTracer> pica_tracer;
};

} // namespace Debugger
