// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/file_sys/directory_backend.h"
#include "core/file_sys/file_backend.h"
#include "core/hle/service/fs/archive.h"

using FileSys::ArchiveBackend;
using FileSys::Mode;
using FileSys::Path;
using Service::FS::Directory;
using Service::FS::File;

namespace Service {
namespace FS {

Archive::Archive(std::unique_ptr<ArchiveBackend> backend, const Path& path) {
    archive_backend = std::move(backend);
    archive_path = path;
}

Archive::~Archive() {}

ResultCode Archive::CreateDirectory(const Path& path) {
    return archive_backend->CreateDirectory(path);
}

ResultCode Archive::CreateFile(const Path& path, const u64 file_size) {
    return archive_backend->CreateFile(path, file_size);
}

ResultCode Archive::DeleteFile(const Path& path) {
    return archive_backend->DeleteFile(path);
}

ResultCode Archive::DeleteDirectory(const Path& path) {
    return archive_backend->DeleteDirectory(path);
}

ResultCode Archive::DeleteDirectoryRecursively(const Path& path) {
    return archive_backend->DeleteDirectoryRecursively(path);
}

ResultVal<u64> Archive::GetFreeBytes() {
    return MakeResult<u64>(archive_backend->GetFreeBytes());
}

ResultVal<std::shared_ptr<Directory>> Archive::OpenDirectory(const Path& path) {
    auto directory_backend = archive_backend->OpenDirectory(path);
    if (directory_backend.Failed())
        return directory_backend.Code();

    auto directory = std::make_shared<Directory>(std::move(directory_backend.Unwrap()), path);
    return MakeResult<std::shared_ptr<Directory>>(std::move(directory));
}

ResultVal<std::shared_ptr<File>> Archive::OpenFile(const Path& path, const Mode mode) {
    auto file_backend = archive_backend->OpenFile(path, mode);
    if (file_backend.Failed())
        return file_backend.Code();

    auto file = std::make_shared<File>(std::move(file_backend.Unwrap()), path);
    return MakeResult<std::shared_ptr<File>>(std::move(file));
}

ResultCode Archive::RenameDirectory(const Path& src_path, const Path& dest_path) {
    return archive_backend->RenameDirectory(src_path, dest_path);
}

ResultCode Archive::RenameFile(const Path& src_path, const Path& dest_path) {
    return archive_backend->RenameFile(src_path, dest_path);
}

} // namespace FS
} // namespace Service
