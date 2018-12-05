// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/core.h"
#include "core/hle/kernel/shared_memory.h"
#include "core/hle/service/gsp/gsp.h"

namespace Service::GSP {

void InstallInterfaces(Core::System& system) {
    auto& service_manager = system.ServiceManager();
    auto gpu = std::make_shared<GSP_GPU>(system);
    gpu->InstallAsService(service_manager);

    std::make_shared<GSP_LCD>()->InstallAsService(service_manager);
}

} // namespace Service::GSP
