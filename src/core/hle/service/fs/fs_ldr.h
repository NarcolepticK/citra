// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/fs/fs.h"

namespace Service {
namespace FS {

class FS_LDR final : public Module::Interface {
public:
    explicit FS_LDR(std::shared_ptr<Module> fs);
    ~FS_LDR() = default;
};

} // namespace FS
} // namespace Service
