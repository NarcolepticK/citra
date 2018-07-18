// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/fs/fs.h"

namespace Service {
namespace FS {

class FS_REG final : public Module::Interface {
public:
    explicit FS_REG(std::shared_ptr<Module> fs);
    ~FS_REG() = default;

protected:
    /**
     * FS::Register service function
     *  Inputs:
     *      0 : Header code [0x040103C0]
     *      1 : Process ID to register
     *    2-3 : u64, Program Handle
     *    4-7 : Program Information
     *   8-15 : Storage Information
     *  Outputs:
     *      1 : Result of function, 0 on success, otherwise error code
     */
    void Register(Kernel::HLERequestContext& ctx);

    /**
     * FS::Unregister service function
     *  Inputs:
     *      0 : Header code [0x04020040]
     *      1 : Process ID of program to unregister
     *  Outputs:
     *      1 : Result of function, 0 on success, otherwise error code
     */
    void Unregister(Kernel::HLERequestContext& ctx);

    /**
     * FS::GetProgramInfo service function
     *  Inputs:
     *      0 : Header code [0x040300C0]
     *      1 : Entry Count
     *    2-3 : u64, Program Handle
     *  Outputs:
     *      1 : Result of function, 0 on success, otherwise error code
     */
    void GetProgramInfo(Kernel::HLERequestContext& ctx);

    /**
     * FS::LoadProgram service function
     *  Inputs:
     *      0 : Header code [0x04040100]
     *    1-4 : Program Information
     *  Outputs:
     *      1 : Result of function, 0 on success, otherwise error code
     *    2-3 : u64, Program Handle
     */
    void LoadProgram(Kernel::HLERequestContext& ctx);

    /**
     * FS::UnloadProgram service function
     *  Inputs:
     *      0 : Header code [0x04050080]
     *    1-2 : u64, Program Handle
     *  Outputs:
     *      1 : Result of function, 0 on success, otherwise error code
     */
    void UnloadProgram(Kernel::HLERequestContext& ctx);

    /**
     * FS::CheckHostLoadId service function
     *  Inputs:
     *      0 : Header code [0x04060080]
     *    1-2 : u64, Program Handle
     *  Outputs:
     *      1 : Result of function, 0 on success, otherwise error code
     */
    void CheckHostLoadId(Kernel::HLERequestContext& ctx);
};

} // namespace FS
} // namespace Service
