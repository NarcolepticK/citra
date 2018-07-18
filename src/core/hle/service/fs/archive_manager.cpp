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
#include "core/hle/service/fs/archive_manager.h"

using FileSys::ArchiveBackend;
using FileSys::ArchiveFormatInfo;
using FileSys::Mode;
using FileSys::Path;
using Service::FS::Archive;
using Service::FS::Directory;
using Service::FS::File;
using std::string;

namespace Service {
namespace FS {

ArchiveManager::ArchiveManager() {
    next_handle = 1;
    archive_registry = ArchiveRegistry::GetShared(); // Initializes the Archive Registry, if needed
}

ArchiveManager::~ArchiveManager() {
    archive_handle_map.clear();
}

ResultCode ArchiveManager::CloseArchive(const ArchiveHandle handle) {
    if (archive_handle_map.erase(handle) == 0)
        return FileSys::ERR_INVALID_ARCHIVE_HANDLE;
    return RESULT_SUCCESS;
}

ResultCode ArchiveManager::CreateDirectoryFromArchive(const ArchiveHandle handle,
                                                      const Path& path) {
    const auto archive = GetArchive(handle);

    if (archive == nullptr)
        return FileSys::ERR_INVALID_ARCHIVE_HANDLE;
    return archive->CreateDirectory(path);
}

ResultCode ArchiveManager::CreateFileInArchive(const ArchiveHandle handle, const Path& path,
                                               const u64 file_size) {
    const auto archive = GetArchive(handle);

    if (archive == nullptr)
        return FileSys::ERR_INVALID_ARCHIVE_HANDLE;
    return archive->CreateFile(path, file_size);
}

ResultCode ArchiveManager::DeleteFileFromArchive(const ArchiveHandle handle, const Path& path) {
    const auto archive = GetArchive(handle);

    if (archive == nullptr)
        return FileSys::ERR_INVALID_ARCHIVE_HANDLE;
    return archive->DeleteFile(path);
}

ResultCode ArchiveManager::DeleteDirectoryFromArchive(const ArchiveHandle handle,
                                                      const Path& path) {
    const auto archive = GetArchive(handle);

    if (archive == nullptr)
        return FileSys::ERR_INVALID_ARCHIVE_HANDLE;
    return archive->DeleteDirectory(path);
}

ResultCode ArchiveManager::DeleteDirectoryRecursivelyFromArchive(const ArchiveHandle handle,
                                                                 const Path& path) {
    const auto archive = GetArchive(handle);

    if (archive == nullptr)
        return FileSys::ERR_INVALID_ARCHIVE_HANDLE;
    return archive->DeleteDirectoryRecursively(path);
}

ResultCode ArchiveManager::FormatArchive(const ArchiveIdCode id_code,
                                         const ArchiveFormatInfo& format_info, const Path& path) {
    const auto registered_archive = archive_registry->GetRegisteredArchive(id_code);

    if (registered_archive == nullptr)
        return UnimplementedFunction(ErrorModule::FS); // TODO(Subv): Find the right error
    return registered_archive->Format(path, format_info);
}

Archive* ArchiveManager::GetArchive(const ArchiveHandle handle) {
    const auto itr = archive_handle_map.find(handle);
    return (itr == archive_handle_map.end()) ? nullptr : itr->second.get();
}

ResultVal<ArchiveFormatInfo> ArchiveManager::GetArchiveFormatInfo(const ArchiveIdCode id_code,
                                                                  const Path& path) {
    const auto registered_archive = archive_registry->GetRegisteredArchive(id_code);

    if (registered_archive == nullptr)
        return UnimplementedFunction(ErrorModule::FS); // TODO(Subv): Find the right error
    return registered_archive->GetFormatInfo(path);
}

ResultVal<u64> ArchiveManager::GetFreeBytesInArchive(const ArchiveHandle handle) {
    auto archive = GetArchive(handle);

    if (archive == nullptr)
        return FileSys::ERR_INVALID_ARCHIVE_HANDLE;
    return archive->GetFreeBytes();
}

ResultVal<ArchiveHandle> ArchiveManager::OpenArchive(const ArchiveIdCode id_code,
                                                     const Path& path) {
    LOG_TRACE(Service_FS, "Opening archive with id code 0x{:08X}", static_cast<u32>(id_code));

    const auto registered_archive = archive_registry->GetRegisteredArchive(id_code);
    if (registered_archive == nullptr)
        return FileSys::ERROR_NOT_FOUND;

    auto archive = std::make_unique<Archive>(registered_archive->Open(path).Unwrap(), path);

    // This should never even happen in the first place with 64-bit handles,
    while (archive_handle_map.count(next_handle) != 0) {
        ++next_handle;
    }
    archive_handle_map.emplace(next_handle, std::move(archive));

    return MakeResult<ArchiveHandle>(next_handle++);
}

ResultVal<std::shared_ptr<Directory>> ArchiveManager::OpenDirectoryFromArchive(
                                                                        const ArchiveHandle handle,
                                                                        const Path& path) {
    const auto archive = GetArchive(handle);

    if (archive == nullptr)
        return FileSys::ERR_INVALID_ARCHIVE_HANDLE;
    return archive->OpenDirectory(path);
}

ResultVal<std::shared_ptr<File>> ArchiveManager::OpenFileFromArchive(const ArchiveHandle handle,
                                                                     const Path& path,
                                                                     const Mode mode) {
    const auto archive = GetArchive(handle);

    if (archive == nullptr)
        return FileSys::ERR_INVALID_ARCHIVE_HANDLE;
    return archive->OpenFile(path, mode);
}

ResultCode ArchiveManager::RenameDirectoryBetweenArchives(const ArchiveHandle src_handle,
                                                          const Path& src_path,
                                                          const ArchiveHandle dest_handle,
                                                          const Path& dest_path) {
    const auto src_archive = GetArchive(src_handle);
    const auto dest_archive = GetArchive(dest_handle);

    if (src_archive == nullptr || dest_archive == nullptr)
        return FileSys::ERR_INVALID_ARCHIVE_HANDLE;

    if (src_archive == dest_archive)
        return src_archive->RenameDirectory(src_path, dest_path);

    src_archive->DeleteDirectory(src_path);
    dest_archive->CreateDirectory(dest_path);
    return RESULT_SUCCESS;
}

ResultCode ArchiveManager::RenameFileBetweenArchives(const ArchiveHandle src_handle,
                                                     const Path& src_path,
                                                     const ArchiveHandle dest_handle,
                                                     const Path& dest_path) {
    const auto src_archive = GetArchive(src_handle);
    const auto dest_archive = GetArchive(dest_handle);

    if (src_archive == nullptr || dest_archive == nullptr)
        return FileSys::ERR_INVALID_ARCHIVE_HANDLE;

    if (src_archive == dest_archive)
        return src_archive->RenameFile(src_path, dest_path);

    Mode write_mode = {};
    write_mode.write_flag.Assign(1);
    write_mode.create_flag.Assign(1);

    Mode read_mode = {};
    read_mode.read_flag.Assign(1);

    auto src_file = src_archive->OpenFile(src_path, read_mode).Unwrap().get();
    auto dest_file = dest_archive->OpenFile(dest_path, write_mode).Unwrap().get();

    const size_t size = src_file->backend->GetSize();

    std::vector<u8> buffer(size);
    src_file->backend->Read(0, size, buffer.data());

    dest_file->backend->Write(0, size, true, buffer.data());

    src_file->backend->Close();
    dest_file->backend->Close();

    return RESULT_SUCCESS;
}

ResultCode ArchiveManager::CreateExtSaveData(const MediaType media_type, const u32 high,
                                             const u32 low, const std::vector<u8>& smdh_icon,
                                             const ArchiveFormatInfo& format_info) {
    // Construct the binary path to the archive first
    const Path path = FileSys::ConstructExtDataBinaryPath(static_cast<u32>(media_type), high, low);
    const auto archive = archive_registry->GetRegisteredArchive(media_type == MediaType::NAND ?
                                                                ArchiveIdCode::SharedExtSaveData :
                                                                ArchiveIdCode::ExtSaveData);

    if (archive == nullptr)
        return UnimplementedFunction(ErrorModule::FS); // TODO(Subv): Find the right error

    auto ext_savedata = static_cast<FileSys::ArchiveFactory_ExtSaveData*>(archive);

    ResultCode result = ext_savedata->Format(path, format_info);
    if (result.IsError())
        return result;

    ext_savedata->WriteIcon(path, smdh_icon.data(), smdh_icon.size());
    return RESULT_SUCCESS;
}

ResultCode ArchiveManager::DeleteExtSaveData(const MediaType media_type, const u32 high,
                                             const u32 low) {
    // Construct the binary path to the archive first
    const Path path = FileSys::ConstructExtDataBinaryPath(static_cast<u32>(media_type), high, low);
    string media_type_directory;

    if (media_type == MediaType::NAND) {
        media_type_directory = FileUtil::GetUserPath(D_NAND_IDX);
    } else if (media_type == MediaType::SDMC) {
        media_type_directory = FileUtil::GetUserPath(D_SDMC_IDX);
    } else {
        LOG_ERROR(Service_FS, "Unsupported media type {}", static_cast<u32>(media_type));
        return ResultCode(-1); // TODO(Subv): Find the right error code
    }

    // Delete all directories (/user, /boss) and the icon file.
    const string base_path =
        FileSys::GetExtDataContainerPath(media_type_directory, media_type == MediaType::NAND);
    const string extsavedata_path = FileSys::GetExtSaveDataPath(base_path, path);

    if (FileUtil::Exists(extsavedata_path) && !FileUtil::DeleteDirRecursively(extsavedata_path))
        return ResultCode(-1); // TODO(Subv): Find the right error code
    return RESULT_SUCCESS;
}

ResultCode ArchiveManager::CreateSystemSaveData(const u32 high, const u32 low) {
    // Construct the binary path to the archive first
    const Path path = FileSys::ConstructSystemSaveDataBinaryPath(high, low);

    const string nand_directory = FileUtil::GetUserPath(D_NAND_IDX);
    const string base_path = FileSys::GetSystemSaveDataContainerPath(nand_directory);
    const string system_save_data_path = FileSys::GetSystemSaveDataPath(base_path, path);

    if (!FileUtil::CreateFullPath(system_save_data_path))
        return ResultCode(-1); // TODO(Subv): Find the right error code
    return RESULT_SUCCESS;
}

ResultCode ArchiveManager::DeleteSystemSaveData(const u32 high, const u32 low) {
    // Construct the binary path to the archive first
    const Path path = FileSys::ConstructSystemSaveDataBinaryPath(high, low);

    const string nand_directory = FileUtil::GetUserPath(D_NAND_IDX);
    const string base_path = FileSys::GetSystemSaveDataContainerPath(nand_directory);
    const string system_save_data_path = FileSys::GetSystemSaveDataPath(base_path, path);

    if (!FileUtil::DeleteDirRecursively(system_save_data_path))
        return ResultCode(-1); // TODO(Subv): Find the right error code
    return RESULT_SUCCESS;
}

} // namespace FS
} // namespace Service
