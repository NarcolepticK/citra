// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "video_core/debugger/debugger.h"

namespace Debugger {

DebuggerManager::DebuggerManager() {
    graphics_debugger = std::make_shared<Debugger::GraphicsDebugger>();
    pica_debug_context = Pica::DebugContext::Construct();
    pica_tracer = std::make_shared<Pica::PicaTracer>();
}

void DebuggerManager::Reset() {
    pica_debug_context.reset();
}

Debugger::GraphicsDebugger& DebuggerManager::GraphicsDebugger() {
    return *graphics_debugger;
}

const Debugger::GraphicsDebugger& DebuggerManager::GraphicsDebugger() const {
    return *graphics_debugger;
}

Pica::DebugContext& DebuggerManager::PicaDebugContext() {
    return *pica_debug_context;
}

const Pica::DebugContext& DebuggerManager::PicaDebugContext() const {
    return *pica_debug_context;
}

Pica::PicaTracer& DebuggerManager::PicaTracer() {
    return *pica_tracer;
}

const Pica::PicaTracer& DebuggerManager::PicaTracer() const {
    return *pica_tracer;
}

std::shared_ptr<Debugger::GraphicsDebugger> DebuggerManager::SharedGraphicsDebugger() {
    return graphics_debugger;
}

std::shared_ptr<Pica::DebugContext> DebuggerManager::SharedPicaDebugContext() {
    return pica_debug_context;
}

std::shared_ptr<Pica::PicaTracer> DebuggerManager::SharedPicaTracer() {
    return pica_tracer;
}

} // namespace Debugger
