// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <boost/container/flat_map.hpp>
#include "core/file_sys/archive_backend.h"
#include "core/hle/service/service.h"

namespace Loader {
class AppLoader;
}

namespace Service {
namespace FS {

using boost::container::flat_map;
using FileSys::ArchiveFactory;

/// Supported archive types
enum class ArchiveIdCode : u32 {
    SelfNCCH = 0x00000003,
    SaveData = 0x00000004,
    ExtSaveData = 0x00000006,
    SharedExtSaveData = 0x00000007,
    SystemSaveData = 0x00000008,
    SDMC = 0x00000009,
    SDMCWriteOnly = 0x0000000A,
    NCCH = 0x2345678A,
    OtherSaveDataGeneral = 0x567890B2,
    OtherSaveDataPermitted = 0x567890B4,
};

class ArchiveRegistry final {
public:
    ArchiveRegistry();
    ~ArchiveRegistry();

    static std::shared_ptr<ArchiveRegistry> GetShared();

    ArchiveFactory* GetRegisteredArchive(const ArchiveIdCode id_code);

    void RegisterSelfNCCH(Loader::AppLoader& app_loader);

private:
    void RegisterArchiveTypes();

    /**
     * Registers an Archive type, instances of which can later be opened using its IdCode.
     * @param factory File system backend interface to the archive
     * @param id_code Id code used to access this type of archive
     */
    ResultCode RegisterArchiveType(std::unique_ptr<FileSys::ArchiveFactory>&& factory,
                                   ArchiveIdCode id_code);

    /**
     * Map of registered archives, identified by id code. Once an archive is registered here, it is
     * never removed until UnregisterArchiveTypes is called.
     */
    flat_map<ArchiveIdCode, std::unique_ptr<ArchiveFactory>> id_code_map;

    static std::shared_ptr<ArchiveRegistry> archive_registry;
};

} // namespace FS
} // namespace Service
