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
#include "core/file_sys/file_backend.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/am/am.h"
#include "core/hle/service/fs/fs_reg.h"

namespace Service {
namespace FS {

using FileSys::ArchiveBackend;
using FileSys::ArchiveFactory;
using Service::FS::Directory;
using Service::FS::File;

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

    RegisterArchiveTypes();
    RegisterHandlers(functions);
}

FS_REG::~FS_REG() {
    UnregisterArchiveTypes();
}

ArchiveBackend* FS_REG::GetArchive(ArchiveHandle handle) {
    auto itr = handle_map.find(handle);
    return (itr == handle_map.end()) ? nullptr : itr->second.get();
}

ArchiveFactory* FS_REG::GetRegisteredArchive(ArchiveIdCode id_code) {
    auto itr = id_code_map.find(id_code);
    return (itr == id_code_map.end()) ? nullptr : itr->second.get();
}

void FS_REG::RegisterSelfNCCH(Loader::AppLoader& app_loader) {
    auto itr = id_code_map.find(ArchiveIdCode::SelfNCCH);
    if (itr == id_code_map.end()) {
        LOG_ERROR(Service_FS,
                  "Could not register a new NCCH because the SelfNCCH archive hasn't been created");
        return;
    }

    auto* factory = static_cast<FileSys::ArchiveFactory_SelfNCCH*>(itr->second.get());
    factory->Register(app_loader);
}

void FS_REG::RegisterArchiveTypes() {
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
    next_handle = 1;
}

void FS_REG::UnregisterArchiveTypes() {
    id_code_map.clear();
    handle_map.clear();
}

ResultVal<ArchiveHandle> FS_REG::OpenArchive(ArchiveIdCode id_code,
                                             const FileSys::Path& archive_path) {
    auto itr = id_code_map.find(id_code);
    if (itr == id_code_map.end()) {
        return FileSys::ERROR_NOT_FOUND;
    }

    CASCADE_RESULT(std::unique_ptr<ArchiveBackend> res, itr->second->Open(archive_path));

    // This should never even happen in the first place with 64-bit handles,
    while (handle_map.count(next_handle) != 0) {
        ++next_handle;
    }

    handle_map.emplace(next_handle, std::move(res));
    return MakeResult<ArchiveHandle>(next_handle++);
}

ResultCode FS_REG::CloseArchive(ArchiveHandle handle) {
    if (handle_map.erase(handle) == 0)
        return FileSys::ERR_INVALID_ARCHIVE_HANDLE;
    return RESULT_SUCCESS;
}

ResultVal<std::shared_ptr<File>> FS_REG::OpenFileFromArchive(ArchiveHandle archive_handle,
                                                             const FileSys::Path& path,
                                                             const FileSys::Mode mode) {
    ArchiveBackend* archive = GetArchive(archive_handle);
    if (archive == nullptr)
        return FileSys::ERR_INVALID_ARCHIVE_HANDLE;

    auto backend = archive->OpenFile(path, mode);
    if (backend.Failed())
        return backend.Code();

    auto file = std::shared_ptr<File>(new File(std::move(backend).Unwrap(), path));
    return MakeResult<std::shared_ptr<File>>(std::move(file));
}

ResultCode FS_REG::DeleteFileFromArchive(ArchiveHandle archive_handle, const FileSys::Path& path) {
    ArchiveBackend* archive = GetArchive(archive_handle);
    if (archive == nullptr)
        return FileSys::ERR_INVALID_ARCHIVE_HANDLE;
    return archive->DeleteFile(path);
}

ResultCode FS_REG::RenameFileBetweenArchives(ArchiveHandle src_archive_handle,
                                             const FileSys::Path& src_path,
                                             ArchiveHandle dest_archive_handle,
                                             const FileSys::Path& dest_path) {
    ArchiveBackend* src_archive = GetArchive(src_archive_handle);
    ArchiveBackend* dest_archive = GetArchive(dest_archive_handle);
    if (src_archive == nullptr || dest_archive == nullptr)
        return FileSys::ERR_INVALID_ARCHIVE_HANDLE;

    if (src_archive == dest_archive)
        return src_archive->RenameFile(src_path, dest_path);
    return UnimplementedFunction(ErrorModule::FS); // TODO: Implement renaming across archives
}

ResultCode FS_REG::DeleteDirectoryFromArchive(ArchiveHandle archive_handle,
                                              const FileSys::Path& path) {
    ArchiveBackend* archive = GetArchive(archive_handle);
    if (archive == nullptr)
        return FileSys::ERR_INVALID_ARCHIVE_HANDLE;
    return archive->DeleteDirectory(path);
}

ResultCode FS_REG::DeleteDirectoryRecursivelyFromArchive(ArchiveHandle archive_handle,
                                                         const FileSys::Path& path) {
    ArchiveBackend* archive = GetArchive(archive_handle);
    if (archive == nullptr)
        return FileSys::ERR_INVALID_ARCHIVE_HANDLE;
    return archive->DeleteDirectoryRecursively(path);
}

ResultCode FS_REG::CreateFileInArchive(ArchiveHandle archive_handle, const FileSys::Path& path,
                                       u64 file_size) {
    ArchiveBackend* archive = GetArchive(archive_handle);
    if (archive == nullptr)
        return FileSys::ERR_INVALID_ARCHIVE_HANDLE;
    return archive->CreateFile(path, file_size);
}

ResultCode FS_REG::CreateDirectoryFromArchive(ArchiveHandle archive_handle,
                                              const FileSys::Path& path) {
    ArchiveBackend* archive = GetArchive(archive_handle);
    if (archive == nullptr)
        return FileSys::ERR_INVALID_ARCHIVE_HANDLE;
    return archive->CreateDirectory(path);
}

ResultCode FS_REG::RenameDirectoryBetweenArchives(ArchiveHandle src_archive_handle,
                                                  const FileSys::Path& src_path,
                                                  ArchiveHandle dest_archive_handle,
                                                  const FileSys::Path& dest_path) {
    ArchiveBackend* src_archive = GetArchive(src_archive_handle);
    ArchiveBackend* dest_archive = GetArchive(dest_archive_handle);
    if (src_archive == nullptr || dest_archive == nullptr)
        return FileSys::ERR_INVALID_ARCHIVE_HANDLE;

    if (src_archive == dest_archive)
        return src_archive->RenameDirectory(src_path, dest_path);
    return UnimplementedFunction(ErrorModule::FS); // TODO: Implement renaming across archives
}

ResultVal<std::shared_ptr<Directory>> FS_REG::OpenDirectoryFromArchive(ArchiveHandle archive_handle,
                                                                       const FileSys::Path& path) {
    ArchiveBackend* archive = GetArchive(archive_handle);
    if (archive == nullptr)
        return FileSys::ERR_INVALID_ARCHIVE_HANDLE;

    auto backend = archive->OpenDirectory(path);
    if (backend.Failed())
        return backend.Code();

    auto directory = std::shared_ptr<Directory>(new Directory(std::move(backend).Unwrap(), path));
    return MakeResult<std::shared_ptr<Directory>>(std::move(directory));
}

ResultVal<u64> FS_REG::GetFreeBytesInArchive(ArchiveHandle archive_handle) {
    ArchiveBackend* archive = GetArchive(archive_handle);
    if (archive == nullptr)
        return FileSys::ERR_INVALID_ARCHIVE_HANDLE;
    return MakeResult<u64>(archive->GetFreeBytes());
}

ResultCode FS_REG::FormatArchive(ArchiveIdCode id_code,
                                 const FileSys::ArchiveFormatInfo& format_info,
                                 const FileSys::Path& path) {
    auto archive_itr = id_code_map.find(id_code);
    if (archive_itr == id_code_map.end())
        return UnimplementedFunction(ErrorModule::FS); // TODO(Subv): Find the right error
    return archive_itr->second->Format(path, format_info);
}

ResultVal<FileSys::ArchiveFormatInfo> FS_REG::GetArchiveFormatInfo(
    ArchiveIdCode id_code, const FileSys::Path& archive_path) {
    auto archive = id_code_map.find(id_code);
    if (archive == id_code_map.end())
        return UnimplementedFunction(ErrorModule::FS); // TODO(Subv): Find the right error
    return archive->second->GetFormatInfo(archive_path);
}

ResultCode FS_REG::CreateExtSaveData(MediaType media_type, u32 high, u32 low,
                                     const std::vector<u8>& smdh_icon,
                                     const FileSys::ArchiveFormatInfo& format_info) {
    // Construct the binary path to the archive first
    const FileSys::Path path =
        FileSys::ConstructExtDataBinaryPath(static_cast<u32>(media_type), high, low);

    auto archive = id_code_map.find(media_type == MediaType::NAND ? ArchiveIdCode::SharedExtSaveData
                                                                  : ArchiveIdCode::ExtSaveData);
    if (archive == id_code_map.end())
        return UnimplementedFunction(ErrorModule::FS); // TODO(Subv): Find the right error

    auto ext_savedata = static_cast<FileSys::ArchiveFactory_ExtSaveData*>(archive->second.get());
    ResultCode result = ext_savedata->Format(path, format_info);
    if (result.IsError())
        return result;

    ext_savedata->WriteIcon(path, smdh_icon.data(), smdh_icon.size());
    return RESULT_SUCCESS;
}

ResultCode FS_REG::DeleteExtSaveData(MediaType media_type, u32 high, u32 low) {
    // Construct the binary path to the archive first
    const FileSys::Path path =
        FileSys::ConstructExtDataBinaryPath(static_cast<u32>(media_type), high, low);

    std::string media_type_directory;
    if (media_type == MediaType::NAND) {
        media_type_directory = FileUtil::GetUserPath(D_NAND_IDX);
    } else if (media_type == MediaType::SDMC) {
        media_type_directory = FileUtil::GetUserPath(D_SDMC_IDX);
    } else {
        LOG_ERROR(Service_FS, "Unsupported media type {}", static_cast<u32>(media_type));
        return ResultCode(-1); // TODO(Subv): Find the right error code
    }

    // Delete all directories (/user, /boss) and the icon file.
    const std::string base_path =
        FileSys::GetExtDataContainerPath(media_type_directory, media_type == MediaType::NAND);
    const std::string extsavedata_path = FileSys::GetExtSaveDataPath(base_path, path);

    if (FileUtil::Exists(extsavedata_path) && !FileUtil::DeleteDirRecursively(extsavedata_path))
        return ResultCode(-1); // TODO(Subv): Find the right error code
    return RESULT_SUCCESS;
}

ResultCode FS_REG::DeleteSystemSaveData(u32 high, u32 low) {
    // Construct the binary path to the archive first
    const FileSys::Path path = FileSys::ConstructSystemSaveDataBinaryPath(high, low);

    const std::string nand_directory = FileUtil::GetUserPath(D_NAND_IDX);
    const std::string base_path = FileSys::GetSystemSaveDataContainerPath(nand_directory);
    const std::string systemsavedata_path = FileSys::GetSystemSaveDataPath(base_path, path);

    if (!FileUtil::DeleteDirRecursively(systemsavedata_path))
        return ResultCode(-1); // TODO(Subv): Find the right error code
    return RESULT_SUCCESS;
}

ResultCode FS_REG::CreateSystemSaveData(u32 high, u32 low) {
    // Construct the binary path to the archive first
    const FileSys::Path path = FileSys::ConstructSystemSaveDataBinaryPath(high, low);

    const std::string nand_directory = FileUtil::GetUserPath(D_NAND_IDX);
    const std::string base_path = FileSys::GetSystemSaveDataContainerPath(nand_directory);
    const std::string systemsavedata_path = FileSys::GetSystemSaveDataPath(base_path, path);

    if (!FileUtil::CreateFullPath(systemsavedata_path))
        return ResultCode(-1); // TODO(Subv): Find the right error code
    return RESULT_SUCCESS;
}

// TODO(yuriks): This might be what the fs:REG service is for. See the Register/Unregister calls in
// http://3dbrew.org/wiki/Filesystem_services#ProgramRegistry_service_.22fs:REG.22
ResultCode FS_REG::RegisterArchiveType(std::unique_ptr<FileSys::ArchiveFactory>&& factory,
                                       ArchiveIdCode id_code) {
    const auto result = id_code_map.emplace(id_code, std::move(factory));
    const bool inserted = result.second;
    ASSERT_MSG(inserted, "Tried to register more than one archive with same id code");

    const auto& archive = result.first->second;
    LOG_DEBUG(Service_FS, "Registered archive {} with id code 0x{:08X}", archive->GetName(),
              static_cast<u32>(id_code));
    return RESULT_SUCCESS;
}

boost::container::flat_map<ArchiveIdCode, std::unique_ptr<ArchiveFactory>> FS_REG::id_code_map;
std::unordered_map<ArchiveHandle, std::unique_ptr<ArchiveBackend>> FS_REG::handle_map;
ArchiveHandle FS_REG::next_handle;

} // namespace FS
} // namespace Service
