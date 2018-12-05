// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/gsp/gsp_gpu.h"
#include "core/hle/service/gsp/gsp_lcd.h"
#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Service::GSP {

void InstallInterfaces(Core::System& system);
} // namespace Service::GSP
