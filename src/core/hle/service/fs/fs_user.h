// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/fs/fs.h"

namespace Service {
namespace FS {

class FS_USER final : public Module::Interface {
public:
    explicit FS_USER(std::shared_ptr<Module> fs);
    ~FS_USER() = default;
};

} // namespace FS
} // namespace Service
