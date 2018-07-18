// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/file_sys/archive_backend.h"
#include "core/hle/service/fs/directory.h"
#include "core/hle/service/fs/file.h"

using FileSys::ArchiveBackend;
using FileSys::Mode;
using FileSys::Path;
using Service::FS::Directory;
using Service::FS::File;

namespace Service {
namespace FS {

class Archive final {
public:
    Archive(std::unique_ptr<ArchiveBackend> backend, const Path& path);
    ~Archive();

    ResultCode CreateDirectory(const Path& path);
    ResultCode CreateFile(const Path& path, const u64 file_size);

    ResultCode DeleteDirectory(const Path& path);
    ResultCode DeleteDirectoryRecursively(const Path& path);
    ResultCode DeleteFile(const Path& path);

    ResultVal<u64> GetFreeBytes();

    ResultVal<std::shared_ptr<Directory>> OpenDirectory(const Path& path);
    ResultVal<std::shared_ptr<File>> OpenFile(const Path& path, const Mode mode);

    ResultCode RenameDirectory(const Path& src_path, const Path& dest_path);
    ResultCode RenameFile(const Path& src_path, const Path& dest_path);

private:
    Path archive_path; ///< Path of the Archive
    std::unique_ptr<ArchiveBackend> archive_backend; ///< Archive backend interface
};

} // namespace FS
} // namespace Service
