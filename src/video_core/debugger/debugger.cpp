// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "video_core/debugger/debugger.h"

namespace Debugger {

DebuggerManager::DebuggerManager() {
    pica_debug_context = Pica::DebugContext::Construct();
    graphics_debugger = std::make_unique<Debugger::GraphicsDebugger>();
}

DebuggerManager::~DebuggerManager() {
    pica_debug_context.reset();
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

std::shared_ptr<Pica::DebugContext> DebuggerManager::SharedPicaDebugContext() {
    return pica_debug_context;
}

} // namespace Debugger