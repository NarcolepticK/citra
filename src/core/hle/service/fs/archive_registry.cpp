// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/assert.h"
#include "common/file_util.h"
#include "common/logging/log.h"
#include "core/file_sys/archive_backend.h"
#include "core/file_sys/archive_extsavedata.h"
#include "core/file_sys/archive_ncch.h"
#include "core/file_sys/archive_other_savedata.h"
#include "core/file_sys/archive_savedata.h"
#include "core/file_sys/archive_sdmc.h"
#include "core/file_sys/archive_sdmcwriteonly.h"
#include "core/file_sys/archive_selfncch.h"
#include "core/file_sys/archive_systemsavedata.h"
#include "core/file_sys/directory_backend.h"
#include "core/file_sys/errors.h"
#include "core/hle/service/fs/archive_registry.h"

using FileSys::ArchiveBackend;
using FileSys::ArchiveFactory;

namespace Service {
namespace FS {

ArchiveRegistry::ArchiveRegistry() {
    RegisterArchiveTypes();
}

ArchiveRegistry::~ArchiveRegistry() {
    id_code_map.clear();
    archive_registry.reset();
}

std::shared_ptr<ArchiveRegistry> ArchiveRegistry::GetShared() {
    if (!archive_registry)
        archive_registry = std::make_shared<ArchiveRegistry>();

    auto shared(archive_registry);
    return std::move(shared);
}

ArchiveFactory* ArchiveRegistry::GetRegisteredArchive(const ArchiveIdCode id_code) {
    auto itr = id_code_map.find(id_code);
    return (itr == id_code_map.end()) ? nullptr : itr->second.get();
}

void ArchiveRegistry::RegisterSelfNCCH(Loader::AppLoader& app_loader) {
    auto* factory = static_cast<FileSys::ArchiveFactory_SelfNCCH*>(GetRegisteredArchive(
                                                                   ArchiveIdCode::SelfNCCH));
    factory->Register(app_loader);
}

void ArchiveRegistry::RegisterArchiveTypes() {
    // TODO(Subv): Add the other archive types (see here for the known types:
    // http://3dbrew.org/wiki/FS:OpenArchive#Archive_idcodes).
    const std::string sdmc_directory = FileUtil::GetUserPath(D_SDMC_IDX);
    const std::string nand_directory = FileUtil::GetUserPath(D_NAND_IDX);

    auto sdmc_factory = std::make_unique<FileSys::ArchiveFactory_SDMC>(sdmc_directory);
    if (sdmc_factory->Initialize())
        RegisterArchiveType(std::move(sdmc_factory), ArchiveIdCode::SDMC);
    else
        LOG_ERROR(Service_FS, "Can't instantiate SDMC archive with path {}", sdmc_directory);

    auto sdmcwo_factory = std::make_unique<FileSys::ArchiveFactory_SDMCWriteOnly>(sdmc_directory);
    if (sdmcwo_factory->Initialize())
        RegisterArchiveType(std::move(sdmcwo_factory), ArchiveIdCode::SDMCWriteOnly);
    else
        LOG_ERROR(Service_FS, "Can't instantiate SDMCWriteOnly archive with path {}",
                  sdmc_directory);

    // Create the SaveData archive
    auto sd_savedata_source = std::make_shared<FileSys::ArchiveSource_SDSaveData>(sdmc_directory);
    auto savedata_factory = std::make_unique<FileSys::ArchiveFactory_SaveData>(sd_savedata_source);
    RegisterArchiveType(std::move(savedata_factory), ArchiveIdCode::SaveData);

    auto other_savedata_permitted_factory =
        std::make_unique<FileSys::ArchiveFactory_OtherSaveDataPermitted>(sd_savedata_source);
    RegisterArchiveType(std::move(other_savedata_permitted_factory),
                        ArchiveIdCode::OtherSaveDataPermitted);

    auto other_savedata_general_factory =
        std::make_unique<FileSys::ArchiveFactory_OtherSaveDataGeneral>(sd_savedata_source);
    RegisterArchiveType(std::move(other_savedata_general_factory),
                        ArchiveIdCode::OtherSaveDataGeneral);

    auto extsavedata_factory =
        std::make_unique<FileSys::ArchiveFactory_ExtSaveData>(sdmc_directory, false);
    if (extsavedata_factory->Initialize())
        RegisterArchiveType(std::move(extsavedata_factory), ArchiveIdCode::ExtSaveData);
    else
        LOG_ERROR(Service_FS, "Can't instantiate ExtSaveData archive with path {}",
                  extsavedata_factory->GetMountPoint());

    auto sharedextsavedata_factory =
        std::make_unique<FileSys::ArchiveFactory_ExtSaveData>(nand_directory, true);
    if (sharedextsavedata_factory->Initialize())
        RegisterArchiveType(std::move(sharedextsavedata_factory), ArchiveIdCode::SharedExtSaveData);
    else
        LOG_ERROR(Service_FS, "Can't instantiate SharedExtSaveData archive with path {}",
                  sharedextsavedata_factory->GetMountPoint());

    // Create the NCCH archive, basically a small variation of the RomFS archive
    auto savedatacheck_factory = std::make_unique<FileSys::ArchiveFactory_NCCH>();
    RegisterArchiveType(std::move(savedatacheck_factory), ArchiveIdCode::NCCH);

    auto systemsavedata_factory =
        std::make_unique<FileSys::ArchiveFactory_SystemSaveData>(nand_directory);
    RegisterArchiveType(std::move(systemsavedata_factory), ArchiveIdCode::SystemSaveData);

    auto selfncch_factory = std::make_unique<FileSys::ArchiveFactory_SelfNCCH>();
    RegisterArchiveType(std::move(selfncch_factory), ArchiveIdCode::SelfNCCH);
}

// TODO(yuriks): This might be what the fs:REG service is for. See the Register/Unregister calls in
// http://3dbrew.org/wiki/Filesystem_services#ProgramRegistry_service_.22fs:REG.22
ResultCode ArchiveRegistry::RegisterArchiveType(std::unique_ptr<FileSys::ArchiveFactory>&& factory,
                                                ArchiveIdCode id_code) {
    const auto result = id_code_map.emplace(id_code, std::move(factory));
    const bool inserted = result.second;
    ASSERT_MSG(inserted, "Tried to register more than one archive with same id code");

    const auto& archive = result.first->second;
    LOG_DEBUG(Service_FS, "Registered archive {} with id code 0x{:08X}", archive->GetName(),
              static_cast<u32>(id_code));
    return RESULT_SUCCESS;
}

//boost::container::flat_map<ArchiveIdCode, std::unique_ptr<ArchiveFactory>> ArchiveRegistry::id_code_map;
std::shared_ptr<ArchiveRegistry> ArchiveRegistry::archive_registry;

} // namespace FS
} // namespace Service
