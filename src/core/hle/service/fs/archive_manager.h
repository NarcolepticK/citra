// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/file_sys/archive_backend.h"
#include "core/hle/service/fs/archive.h"
#include "core/hle/service/fs/archive_registry.h"
#include "core/hle/service/fs/directory.h"
#include "core/hle/service/fs/file.h"

namespace Service {
namespace FS {

using FileSys::ArchiveBackend;
using FileSys::ArchiveFormatInfo;
using FileSys::Mode;
using FileSys::Path;
using Service::FS::Archive;
using Service::FS::Directory;
using Service::FS::File;

/// The unique system identifier hash, also known as ID0
static constexpr char SYSTEM_ID[]{"00000000000000000000000000000000"};
/// The scrambled SD card CID, also known as ID1
static constexpr char SDCARD_ID[]{"00000000000000000000000000000000"};

/// Media types for the archives
enum class MediaType : u32 { NAND = 0, SDMC = 1, GameCard = 2 };

typedef u64 ArchiveHandle;

class ArchiveManager final {
public:
    ArchiveManager();
    ~ArchiveManager();

    ResultCode CloseArchive(const ArchiveHandle handle);

    ResultCode CreateDirectoryFromArchive(const ArchiveHandle handle, const Path& path);
    ResultCode CreateFileInArchive(const ArchiveHandle handle, const Path& path,
                                   const u64 file_size);

    ResultCode DeleteFileFromArchive(const ArchiveHandle handle, const Path& path);
    ResultCode DeleteDirectoryFromArchive(const ArchiveHandle handle, const Path& path);
    ResultCode DeleteDirectoryRecursivelyFromArchive(const ArchiveHandle handle,
                                                     const Path& path);

    ResultCode FormatArchive(const ArchiveIdCode id_code, const ArchiveFormatInfo& format_info,
                             const Path& path = FileSys::Path());

    Archive* GetArchive(const ArchiveHandle handle);
    ResultVal<ArchiveFormatInfo> GetArchiveFormatInfo(const ArchiveIdCode id_code,
                                                      const Path& path);
    ResultVal<u64> GetFreeBytesInArchive(const ArchiveHandle handle);

    ResultVal<ArchiveHandle> OpenArchive(const ArchiveIdCode id_code, const Path& path);
    ResultVal<std::shared_ptr<Directory>> OpenDirectoryFromArchive(const ArchiveHandle handle,
                                                                   const Path& path);
    ResultVal<std::shared_ptr<File>> OpenFileFromArchive(const ArchiveHandle handle,
                                                         const Path& path, const Mode mode);

    ResultCode RenameDirectoryBetweenArchives(const ArchiveHandle src_handle, const Path& src_path,
                                              const ArchiveHandle dest_handle,
                                              const Path& dest_path);
    ResultCode RenameFileBetweenArchives(const ArchiveHandle src_handle, const Path& src_path,
                                         const ArchiveHandle dest_handle, const Path& dest_path);


    ResultCode CreateExtSaveData(const MediaType media_type, const u32 high, const u32 low,
                                 const std::vector<u8>& smdh_icon,
                                 const ArchiveFormatInfo& format_info);
    ResultCode DeleteExtSaveData(const MediaType media_type, const u32 high, const u32 low);

    ResultCode CreateSystemSaveData(const u32 high, const u32 low);
    ResultCode DeleteSystemSaveData(const u32 high, const u32 low);

private:
    std::shared_ptr<ArchiveRegistry> archive_registry;
    /**
     * Map of active archive handles. Values are pointers to the archives in `idcode_map`.
     */
    std::unordered_map<ArchiveHandle, std::unique_ptr<Archive>> archive_handle_map;
    ArchiveHandle next_handle;
};

} // namespace FS
} // namespace Service