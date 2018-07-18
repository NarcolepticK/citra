// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/service/fs/fs_reg.h"

namespace Service {
namespace FS {

FS_REG::FS_REG(std::shared_ptr<Module> fs) : Module::Interface(std::move(fs), "fs:REG", 2) {
    static const FunctionInfo functions[] = {
        // clang-format off
        // fs: common commands
        {0x000100C6, nullptr, "Dummy1"},
        // fs:REG commands
        {0x040103C0, nullptr, "Register"},
        {0x04020040, nullptr, "Unregister"},
        {0x040300C0, nullptr, "GetProgramInfo"},
        {0x04040100, nullptr, "LoadProgram"},
        {0x04050080, nullptr, "UnloadProgram"},
        {0x04060080, nullptr, "CheckHostLoadId"},
        // clang-format on
    };

    RegisterHandlers(functions);
}

} // namespace FS
} // namespace Service
