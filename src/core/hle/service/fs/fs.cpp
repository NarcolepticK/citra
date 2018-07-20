// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/assert.h"
#include "common/file_util.h"
#include "common/logging/log.h"
#include "common/scope_exit.h"
#include "core/file_sys/errors.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/client_port.h"
#include "core/hle/kernel/client_session.h"
#include "core/hle/kernel/event.h"
#include "core/hle/kernel/process.h"
#include "core/hle/kernel/server_session.h"
#include "core/hle/service/am/am.h"
#include "core/hle/service/fs/directory.h"
#include "core/hle/service/fs/file.h"
#include "core/hle/service/fs/fs.h"
#include "core/hle/service/fs/fs_ldr.h"
#include "core/hle/service/fs/fs_reg.h"
#include "core/hle/service/fs/fs_user.h"
#include "core/settings.h"

namespace Service {
namespace FS {

using Kernel::ClientSession;
using Kernel::ServerSession;
using Kernel::SharedPtr;
using Service::FS::Directory;
using Service::FS::File;

void Module::Interface::Initialize(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x0801, 0, 2);
    rp.PopPID();

    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
    rb.Push(RESULT_SUCCESS);

    LOG_DEBUG(Service_FS, "called");
}

void Module::Interface::OpenFile(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x0802, 7, 2);
    rp.Skip(1, false); // Transaction.
    const ArchiveHandle archive_handle = rp.Pop<u64>();
    const auto filename_type = rp.PopEnum<FileSys::LowPathType>();
    const u32 filename_size = rp.Pop<u32>();
    const FileSys::Mode mode{rp.Pop<u32>()};
    const u32 attributes = rp.Pop<u32>(); // TODO(Link Mauve): do something with those attributes.
    const std::vector<u8> filename = rp.PopStaticBuffer();
    ASSERT(filename.size() == filename_size);
    const FileSys::Path file_path(filename_type, filename);
    const ResultVal<std::shared_ptr<File>> file_res =
        fs->archive_manager->OpenFileFromArchive(archive_handle, file_path, mode);

    IPC::RequestBuilder rb = rp.MakeBuilder(1, 2);
    rb.Push(file_res.Code());
    if (file_res.Succeeded()) {
        std::shared_ptr<File> file = *file_res;
        rb.PushMoveObjects(file->Connect());
    } else {
        rb.PushMoveObjects<Kernel::Object>(nullptr);
        LOG_ERROR(Service_FS, "failed to get a handle for file {}", file_path.DebugStr());
    }

    LOG_DEBUG(Service_FS, "path={}, mode={} attrs={}", file_path.DebugStr(), mode.hex, attributes);
}

void Module::Interface::OpenFileDirectly(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x803, 8, 4);
    rp.Skip(1, false); // Transaction
    const auto archive_id = rp.PopEnum<FS::ArchiveIdCode>();
    const auto archivename_type = rp.PopEnum<FileSys::LowPathType>();
    const u32 archivename_size = rp.Pop<u32>();
    const auto filename_type = rp.PopEnum<FileSys::LowPathType>();
    const u32 filename_size = rp.Pop<u32>();
    const FileSys::Mode mode{rp.Pop<u32>()};
    const u32 attributes = rp.Pop<u32>(); // TODO(Link Mauve): do something with those attributes.
    const std::vector<u8> archivename = rp.PopStaticBuffer();
    const std::vector<u8> filename = rp.PopStaticBuffer();
    ASSERT(archivename.size() == archivename_size);
    ASSERT(filename.size() == filename_size);
    const FileSys::Path archive_path(archivename_type, archivename);
    const FileSys::Path file_path(filename_type, filename);
    const ResultVal<ArchiveHandle> archive_handle =
        fs->archive_manager->OpenArchive(archive_id, archive_path);

    IPC::RequestBuilder rb = rp.MakeBuilder(1, 2);
    if (archive_handle.Failed()) {
        LOG_ERROR(Service_FS,
                  "Failed to get a handle for archive archive_id=0x{:08X} archive_path={}",
                  static_cast<u32>(archive_id), archive_path.DebugStr());
        rb.Push(archive_handle.Code());
        rb.PushMoveObjects<Kernel::Object>(nullptr);
        return;
    }
    SCOPE_EXIT({ fs->archive_manager->CloseArchive(*archive_handle); });

    const ResultVal<std::shared_ptr<File>> file_res =
        fs->archive_manager->OpenFileFromArchive(*archive_handle, file_path, mode);
    rb.Push(file_res.Code());
    if (file_res.Succeeded()) {
        const std::shared_ptr<File> file = *file_res;
        rb.PushMoveObjects(file->Connect());
    } else {
        rb.PushMoveObjects<Kernel::Object>(nullptr);
        LOG_ERROR(Service_FS, "failed to get a handle for file {} mode={} attributes={}",
                  file_path.DebugStr(), mode.hex, attributes);
    }

    LOG_DEBUG(Service_FS, "archive_id=0x{:08X} archive_path={} file_path={}, mode={} attributes={}",
              static_cast<u32>(archive_id), archive_path.DebugStr(), file_path.DebugStr(), mode.hex,
              attributes);
}

void Module::Interface::DeleteFile(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x804, 5, 2);
    rp.Skip(1, false); // TransactionId
    const ArchiveHandle archive_handle = rp.PopRaw<ArchiveHandle>();
    const auto filename_type = rp.PopEnum<FileSys::LowPathType>();
    const u32 filename_size = rp.Pop<u32>();
    const std::vector<u8> filename = rp.PopStaticBuffer();
    ASSERT(filename.size() == filename_size);
    const FileSys::Path file_path(filename_type, filename);

    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
    rb.Push(fs->archive_manager->DeleteFileFromArchive(archive_handle, file_path));

    LOG_DEBUG(Service_FS, "type={} size={} data={}", static_cast<u32>(filename_type), filename_size,
              file_path.DebugStr());
}

void Module::Interface::RenameFile(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x805, 9, 4);
    rp.Skip(1, false); // TransactionId
    const ArchiveHandle src_archive_handle = rp.PopRaw<ArchiveHandle>();
    const auto src_filename_type = rp.PopEnum<FileSys::LowPathType>();
    const u32 src_filename_size = rp.Pop<u32>();
    const ArchiveHandle dest_archive_handle = rp.PopRaw<ArchiveHandle>();
    const auto dest_filename_type = rp.PopEnum<FileSys::LowPathType>();
    const u32 dest_filename_size = rp.Pop<u32>();
    const std::vector<u8> src_filename = rp.PopStaticBuffer();
    const std::vector<u8> dest_filename = rp.PopStaticBuffer();
    ASSERT(src_filename.size() == src_filename_size);
    ASSERT(dest_filename.size() == dest_filename_size);
    const FileSys::Path src_file_path(src_filename_type, src_filename);
    const FileSys::Path dest_file_path(dest_filename_type, dest_filename);

    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
    rb.Push(fs->archive_manager->RenameFileBetweenArchives(src_archive_handle, src_file_path,
                                                           dest_archive_handle, dest_file_path));

    LOG_DEBUG(Service_FS,
              "src_type={} src_size={} src_data={} dest_type={} dest_size={} dest_data={}",
              static_cast<u32>(src_filename_type), src_filename_size, src_file_path.DebugStr(),
              static_cast<u32>(dest_filename_type), dest_filename_size, dest_file_path.DebugStr());
}

void Module::Interface::DeleteDirectory(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x806, 5, 2);
    rp.Skip(1, false); // TransactionId
    const ArchiveHandle archive_handle = rp.PopRaw<ArchiveHandle>();
    const auto dirname_type = rp.PopEnum<FileSys::LowPathType>();
    const u32 dirname_size = rp.Pop<u32>();
    const std::vector<u8> dirname = rp.PopStaticBuffer();
    ASSERT(dirname.size() == dirname_size);
    const FileSys::Path dir_path(dirname_type, dirname);

    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
    rb.Push(fs->archive_manager->DeleteDirectoryFromArchive(archive_handle, dir_path));

    LOG_DEBUG(Service_FS, "type={} size={} data={}", static_cast<u32>(dirname_type), dirname_size,
              dir_path.DebugStr());
}

void Module::Interface::DeleteDirectoryRecursively(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x807, 5, 2);
    rp.Skip(1, false); // TransactionId
    const ArchiveHandle archive_handle = rp.PopRaw<ArchiveHandle>();
    const auto dirname_type = rp.PopEnum<FileSys::LowPathType>();
    const u32 dirname_size = rp.Pop<u32>();
    const std::vector<u8> dirname = rp.PopStaticBuffer();
    ASSERT(dirname.size() == dirname_size);
    const FileSys::Path dir_path(dirname_type, dirname);

    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
    rb.Push(fs->archive_manager->DeleteDirectoryRecursivelyFromArchive(archive_handle, dir_path));

    LOG_DEBUG(Service_FS, "type={} size={} data={}", static_cast<u32>(dirname_type), dirname_size,
              dir_path.DebugStr());
}

void Module::Interface::CreateFile(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x808, 8, 2);

    rp.Skip(1, false); // TransactionId
    const ArchiveHandle archive_handle = rp.PopRaw<ArchiveHandle>();
    const auto filename_type = rp.PopEnum<FileSys::LowPathType>();
    const u32 filename_size = rp.Pop<u32>();
    const u32 attributes = rp.Pop<u32>();
    const u64 file_size = rp.Pop<u64>();
    const std::vector<u8> filename = rp.PopStaticBuffer();
    ASSERT(filename.size() == filename_size);
    const FileSys::Path file_path(filename_type, filename);

    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
    rb.Push(fs->archive_manager->CreateFileInArchive(archive_handle, file_path, file_size));

    LOG_DEBUG(Service_FS, "type={} attributes={} size={:x} data={}",
              static_cast<u32>(filename_type), attributes, file_size, file_path.DebugStr());
}

void Module::Interface::CreateDirectory(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x809, 6, 2);
    rp.Skip(1, false); // TransactionId
    const ArchiveHandle archive_handle = rp.PopRaw<ArchiveHandle>();
    const auto dirname_type = rp.PopEnum<FileSys::LowPathType>();
    const u32 dirname_size = rp.Pop<u32>();
    const u32 attributes = rp.Pop<u32>();
    const std::vector<u8> dirname = rp.PopStaticBuffer();
    ASSERT(dirname.size() == dirname_size);
    const FileSys::Path dir_path(dirname_type, dirname);

    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
    rb.Push(fs->archive_manager->CreateDirectoryFromArchive(archive_handle, dir_path));

    LOG_DEBUG(Service_FS, "type={} size={} data={}", static_cast<u32>(dirname_type), dirname_size,
              dir_path.DebugStr());
}

void Module::Interface::RenameDirectory(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x80A, 9, 4);
    rp.Skip(1, false); // TransactionId
    const ArchiveHandle src_archive_handle = rp.PopRaw<ArchiveHandle>();
    const auto src_dirname_type = rp.PopEnum<FileSys::LowPathType>();
    const u32 src_dirname_size = rp.Pop<u32>();
    const ArchiveHandle dest_archive_handle = rp.PopRaw<ArchiveHandle>();
    const auto dest_dirname_type = rp.PopEnum<FileSys::LowPathType>();
    const u32 dest_dirname_size = rp.Pop<u32>();
    const std::vector<u8> src_dirname = rp.PopStaticBuffer();
    const std::vector<u8> dest_dirname = rp.PopStaticBuffer();
    ASSERT(src_dirname.size() == src_dirname_size);
    ASSERT(dest_dirname.size() == dest_dirname_size);
    const FileSys::Path src_dir_path(src_dirname_type, src_dirname);
    const FileSys::Path dest_dir_path(dest_dirname_type, dest_dirname);

    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
    rb.Push(fs->archive_manager->RenameDirectoryBetweenArchives(
        src_archive_handle, src_dir_path, dest_archive_handle, dest_dir_path));

    LOG_DEBUG(Service_FS,
              "src_type={} src_size={} src_data={} dest_type={} dest_size={} dest_data={}",
              static_cast<u32>(src_dirname_type), src_dirname_size, src_dir_path.DebugStr(),
              static_cast<u32>(dest_dirname_type), dest_dirname_size, dest_dir_path.DebugStr());
}

void Module::Interface::OpenDirectory(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x80B, 4, 2);
    const auto archive_handle = rp.PopRaw<ArchiveHandle>();
    const auto dirname_type = rp.PopEnum<FileSys::LowPathType>();
    const u32 dirname_size = rp.Pop<u32>();
    const std::vector<u8> dirname = rp.PopStaticBuffer();
    ASSERT(dirname.size() == dirname_size);
    const FileSys::Path dir_path(dirname_type, dirname);
    const ResultVal<std::shared_ptr<Directory>> dir_res =
        fs->archive_manager->OpenDirectoryFromArchive(archive_handle, dir_path);

    IPC::RequestBuilder rb = rp.MakeBuilder(1, 2);
    rb.Push(dir_res.Code());
    if (dir_res.Succeeded()) {
        const std::shared_ptr<Directory> directory = *dir_res;
        const auto sessions = ServerSession::CreateSessionPair(directory->GetName());
        directory->ClientConnected(std::get<SharedPtr<ServerSession>>(sessions));
        rb.PushMoveObjects(std::get<SharedPtr<ClientSession>>(sessions));
    } else {
        LOG_ERROR(Service_FS, "failed to get a handle for directory type={} size={} data={}",
                  static_cast<u32>(dirname_type), dirname_size, dir_path.DebugStr());
        rb.PushMoveObjects<Kernel::Object>(nullptr);
    }

    LOG_DEBUG(Service_FS, "type={} size={} data={}", static_cast<u32>(dirname_type), dirname_size,
              dir_path.DebugStr());
}

void Module::Interface::OpenArchive(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x80C, 3, 2);
    const auto archive_id = rp.PopEnum<FS::ArchiveIdCode>();
    const auto archivename_type = rp.PopEnum<FileSys::LowPathType>();
    const u32 archivename_size = rp.Pop<u32>();
    const std::vector<u8> archivename = rp.PopStaticBuffer();
    ASSERT(archivename.size() == archivename_size);
    const FileSys::Path archive_path(archivename_type, archivename);
    const ResultVal<ArchiveHandle> handle =
        fs->archive_manager->OpenArchive(archive_id, archive_path);

    IPC::RequestBuilder rb = rp.MakeBuilder(3, 0);
    rb.Push(handle.Code());
    if (handle.Succeeded()) {
        rb.PushRaw(*handle);
    } else {
        rb.Push<u64>(0);
        LOG_ERROR(Service_FS,
                  "failed to get a handle for archive archive_id=0x{:08X} archive_path={}",
                  static_cast<u32>(archive_id), archive_path.DebugStr());
    }

    LOG_DEBUG(Service_FS, "archive_id=0x{:08X} archive_path={}", static_cast<u32>(archive_id),
              archive_path.DebugStr());
}

void Module::Interface::CloseArchive(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x80E, 2, 0);
    const auto archive_handle = rp.PopRaw<ArchiveHandle>();

    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
    rb.Push(fs->archive_manager->CloseArchive(archive_handle));

    LOG_DEBUG(Service_FS, "called");
}

void Module::Interface::IsSdmcDetected(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x817, 0, 0);

    IPC::RequestBuilder rb = rp.MakeBuilder(2, 0);
    rb.Push(RESULT_SUCCESS);
    rb.Push(Settings::values.use_virtual_sd);

    LOG_DEBUG(Service_FS, "called");
}

void Module::Interface::IsSdmcWriteable(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x818, 0, 0);

    IPC::RequestBuilder rb = rp.MakeBuilder(2, 0);
    rb.Push(RESULT_SUCCESS);
    rb.Push(Settings::values.use_virtual_sd);

    LOG_DEBUG(Service_FS, "called");
}

void Module::Interface::FormatSaveData(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x84C, 9, 2);
    const auto archive_id = rp.PopEnum<FS::ArchiveIdCode>();
    const auto archivename_type = rp.PopEnum<FileSys::LowPathType>();
    const u32 archivename_size = rp.Pop<u32>();
    const u32 block_size = rp.Pop<u32>();
    const u32 number_directories = rp.Pop<u32>();
    const u32 number_files = rp.Pop<u32>();
    const u32 directory_buckets = rp.Pop<u32>();
    const u32 file_buckets = rp.Pop<u32>();
    const bool duplicate_data = rp.Pop<bool>();
    const std::vector<u8> archivename = rp.PopStaticBuffer();
    ASSERT(archivename.size() == archivename_size);
    const FileSys::Path archive_path(archivename_type, archivename);

    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
    if (archive_id != FS::ArchiveIdCode::SaveData) {
        LOG_ERROR(Service_FS, "tried to format an archive different than SaveData, {}",
                  static_cast<u32>(archive_id));
        rb.Push(FileSys::ERROR_INVALID_PATH);
        return;
    }

    if (archive_path.GetType() != FileSys::LowPathType::Empty) {
        // TODO(Subv): Implement formatting the SaveData of other games
        LOG_ERROR(Service_FS, "archive LowPath type other than empty is currently unsupported");
        rb.Push(UnimplementedFunction(ErrorModule::FS));
        return;
    }

    FileSys::ArchiveFormatInfo format_info;
    format_info.duplicate_data = duplicate_data;
    format_info.number_directories = number_directories;
    format_info.number_files = number_files;
    format_info.total_size = block_size * 512;
    rb.Push(fs->archive_manager->FormatArchive(ArchiveIdCode::SaveData, format_info));

    LOG_WARNING(Service_FS, " (STUBBED), archive_path={}", archive_path.DebugStr());
}

void Module::Interface::LegacyFormatThisUserSaveData(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x80F, 6, 0);
    const u32 block_size = rp.Pop<u32>();
    const u32 number_directories = rp.Pop<u32>();
    const u32 number_files = rp.Pop<u32>();
    const u32 directory_buckets = rp.Pop<u32>();
    const u32 file_buckets = rp.Pop<u32>();
    const bool duplicate_data = rp.Pop<bool>();

    FileSys::ArchiveFormatInfo format_info;
    format_info.duplicate_data = duplicate_data;
    format_info.number_directories = number_directories;
    format_info.number_files = number_files;
    format_info.total_size = block_size * 512;

    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
    rb.Push(fs->archive_manager->FormatArchive(ArchiveIdCode::SaveData, format_info));

    LOG_DEBUG(Service_FS, "called");
}

void Module::Interface::GetFreeBytes(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x812, 2, 0);
    const ArchiveHandle archive_handle = rp.PopRaw<ArchiveHandle>();
    ResultVal<u64> bytes_res = fs->archive_manager->GetFreeBytesInArchive(archive_handle);

    IPC::RequestBuilder rb = rp.MakeBuilder(3, 0);
    rb.Push(bytes_res.Code());
    if (bytes_res.Succeeded())
        rb.Push<u64>(bytes_res.Unwrap());
    else
        rb.Push<u64>(0);

    LOG_DEBUG(Service_FS, "called");
}

void Module::Interface::CreateExtSaveData(Kernel::HLERequestContext& ctx) {
    // TODO(Subv): Figure out the other parameters.
    IPC::RequestParser rp(ctx, 0x0851, 9, 2);
    const MediaType media_type = static_cast<MediaType>(rp.Pop<u32>());
    const u32 save_low = rp.Pop<u32>();
    const u32 save_high = rp.Pop<u32>();
    const u32 unknown = rp.Pop<u32>();
    const u32 directories = rp.Pop<u32>();
    const u32 files = rp.Pop<u32>();
    const u64 size_limit = rp.Pop<u64>();
    const u32 icon_size = rp.Pop<u32>();
    auto icon_buffer = rp.PopMappedBuffer();

    std::vector<u8> icon(icon_size);
    icon_buffer.Read(icon.data(), 0, icon_size);

    FileSys::ArchiveFormatInfo format_info;
    format_info.number_directories = directories;
    format_info.number_files = files;
    format_info.duplicate_data = false;
    format_info.total_size = 0;

    IPC::RequestBuilder rb = rp.MakeBuilder(1, 2);
    rb.Push(
        fs->archive_manager->CreateExtSaveData(media_type, save_high, save_low, icon, format_info));
    rb.PushMappedBuffer(icon_buffer);

    LOG_WARNING(Service_FS,
                "(STUBBED) savedata_high={:08X} savedata_low={:08X} unknown={:08X} "
                "files={:08X} directories={:08X} size_limit={:016x} icon_size={:08X}",
                save_high, save_low, unknown, directories, files, size_limit, icon_size);
}

void Module::Interface::DeleteExtSaveData(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x852, 4, 0);
    const MediaType media_type = static_cast<MediaType>(rp.Pop<u32>());
    const u32 save_low = rp.Pop<u32>();
    const u32 save_high = rp.Pop<u32>();
    const u32 unknown = rp.Pop<u32>(); // TODO(Subv): Figure out what this is

    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
    rb.Push(fs->archive_manager->DeleteExtSaveData(media_type, save_high, save_low));

    LOG_WARNING(Service_FS,
                "(STUBBED) save_low={:08X} save_high={:08X} media_type={:08X} unknown={:08X}",
                save_low, save_high, static_cast<u32>(media_type), unknown);
}

void Module::Interface::CardSlotIsInserted(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x821, 0, 0);

    IPC::RequestBuilder rb = rp.MakeBuilder(2, 0);
    rb.Push(RESULT_SUCCESS);
    rb.Push(false);

    LOG_WARNING(Service_FS, "(STUBBED) called");
}

void Module::Interface::DeleteSystemSaveData(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x857, 2, 0);
    const u32 savedata_high = rp.Pop<u32>();
    const u32 savedata_low = rp.Pop<u32>();

    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
    rb.Push(fs->archive_manager->DeleteSystemSaveData(savedata_high, savedata_low));

    LOG_DEBUG(Service_FS, "called");
}

void Module::Interface::CreateSystemSaveData(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x856, 9, 0);
    const u32 savedata_high = rp.Pop<u32>();
    const u32 savedata_low = rp.Pop<u32>();
    const u32 total_size = rp.Pop<u32>();
    const u32 block_size = rp.Pop<u32>();
    const u32 directories = rp.Pop<u32>();
    const u32 files = rp.Pop<u32>();
    const u32 directory_buckets = rp.Pop<u32>();
    const u32 file_buckets = rp.Pop<u32>();
    const bool duplicate = rp.Pop<bool>();

    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
    rb.Push(fs->archive_manager->CreateSystemSaveData(savedata_high, savedata_low));

    LOG_WARNING(
        Service_FS,
        "(STUBBED) savedata_high={:08X} savedata_low={:08X} total_size={:08X}  block_size={:08X} "
        "directories={} files={} directory_buckets={} file_buckets={} duplicate={}",
        savedata_high, savedata_low, total_size, block_size, directories, files, directory_buckets,
        file_buckets, duplicate);
}

void Module::Interface::LegacyCreateSystemSaveData(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x810, 8, 0);
    const u32 savedata_id = rp.Pop<u32>();
    const u32 total_size = rp.Pop<u32>();
    const u32 block_size = rp.Pop<u32>();
    const u32 directories = rp.Pop<u32>();
    const u32 files = rp.Pop<u32>();
    const u32 directory_buckets = rp.Pop<u32>();
    const u32 file_buckets = rp.Pop<u32>();
    const bool duplicate = rp.Pop<bool>();

    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
    // With this command, the SystemSaveData always has save_high = 0 (Always created in the NAND)
    rb.Push(fs->archive_manager->CreateSystemSaveData(0, savedata_id));

    LOG_WARNING(Service_FS,
                "(STUBBED) savedata_id={:08X} total_size={:08X} block_size={:08X} directories={} "
                "files={} directory_buckets={} file_buckets={} duplicate={}",
                savedata_id, total_size, block_size, directories, files, directory_buckets,
                file_buckets, duplicate);
}

void Module::Interface::InitializeWithSdkVersion(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x861, 1, 2);
    const u32 version = rp.Pop<u32>();
    rp.PopPID();

    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
    rb.Push(RESULT_SUCCESS);

    LOG_WARNING(Service_FS, "(STUBBED) called, version: 0x{:08X}", version);
}

void Module::Interface::SetPriority(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x862, 1, 0);
    priority = rp.Pop<u32>();

    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
    rb.Push(RESULT_SUCCESS);

    LOG_DEBUG(Service_FS, "called priority=0x{:X}", priority);
}

void Module::Interface::GetPriority(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x863, 0, 0);

    if (priority == -1)
        LOG_INFO(Service_FS, "priority was not set, priority=0x{:X}", priority);

    IPC::RequestBuilder rb = rp.MakeBuilder(2, 0);
    rb.Push(RESULT_SUCCESS);
    rb.Push(priority);

    LOG_DEBUG(Service_FS, "called priority=0x{:X}", priority);
}

void Module::Interface::GetArchiveResource(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x849, 1, 0);
    const u32 system_media_type = rp.Pop<u32>();

    IPC::RequestBuilder rb = rp.MakeBuilder(5, 0);
    rb.Push(RESULT_SUCCESS);
    rb.Push<u32>(512);
    rb.Push<u32>(16384);
    rb.Push<u32>(0x80000); // 8GiB capacity
    rb.Push<u32>(0x80000); // 8GiB free

    LOG_WARNING(Service_FS, "(STUBBED) called Media type=0x{:08X}", system_media_type);
}

void Module::Interface::GetFormatInfo(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x845, 3, 2);
    const auto archive_id = rp.PopEnum<FS::ArchiveIdCode>();
    const auto archivename_type = rp.PopEnum<FileSys::LowPathType>();
    const u32 archivename_size = rp.Pop<u32>();
    const std::vector<u8> archivename = rp.PopStaticBuffer();
    ASSERT(archivename.size() == archivename_size);
    const FileSys::Path archive_path(archivename_type, archivename);
    const auto format_info = fs->archive_manager->GetArchiveFormatInfo(archive_id, archive_path);

    IPC::RequestBuilder rb = rp.MakeBuilder(5, 0);
    rb.Push(format_info.Code());
    if (format_info.Failed()) {
        LOG_ERROR(Service_FS, "Failed to retrieve the format info");
        rb.Skip(4, true);
        return;
    }

    rb.Push<u32>(format_info->total_size);
    rb.Push<u32>(format_info->number_directories);
    rb.Push<u32>(format_info->number_files);
    rb.Push<bool>(format_info->duplicate_data != 0);

    LOG_DEBUG(Service_FS, "archive_path={}", archive_path.DebugStr());
}

void Module::Interface::GetProgramLaunchInfo(Kernel::HLERequestContext& ctx) {
    // TODO(Subv): The real FS service manages its own process list and only checks the processes
    // that were registered with the 'fs:REG' service.
    IPC::RequestParser rp(ctx, 0x82F, 1, 0);
    const u32 process_id = rp.Pop<u32>();
    const auto process = Kernel::GetProcessById(process_id);

    IPC::RequestBuilder rb = rp.MakeBuilder(5, 0);
    if (process == nullptr) {
        // Note: In this case, the rest of the parameters are not changed but the command header
        // remains the same.
        rb.Push(ResultCode(FileSys::ErrCodes::ArchiveNotMounted, ErrorModule::FS,
                           ErrorSummary::NotFound, ErrorLevel::Status));
        rb.Skip(4, false);
        return;
    }

    const u64 program_id = process->codeset->program_id;
    const auto media_type = Service::AM::GetTitleMediaType(program_id);

    rb.Push(RESULT_SUCCESS);
    rb.Push(program_id);
    rb.Push(static_cast<u8>(media_type));
    rb.Push<u32>(0); // TODO(Subv): Find out what this value means.

    LOG_DEBUG(Service_FS, "process_id={}", process_id);
}

void Module::Interface::ObsoletedCreateExtSaveData(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x830, 6, 2);
    const MediaType media_type = static_cast<MediaType>(rp.Pop<u8>());
    const u32 save_low = rp.Pop<u32>();
    const u32 save_high = rp.Pop<u32>();
    const u32 icon_size = rp.Pop<u32>();
    const u32 directories = rp.Pop<u32>();
    const u32 files = rp.Pop<u32>();
    auto icon_buffer = rp.PopMappedBuffer();

    std::vector<u8> icon(icon_size);
    icon_buffer.Read(icon.data(), 0, icon_size);

    FileSys::ArchiveFormatInfo format_info;
    format_info.number_directories = directories;
    format_info.number_files = files;
    format_info.duplicate_data = false;
    format_info.total_size = 0;

    IPC::RequestBuilder rb = rp.MakeBuilder(1, 2);
    rb.Push(
        fs->archive_manager->CreateExtSaveData(media_type, save_high, save_low, icon, format_info));
    rb.PushMappedBuffer(icon_buffer);

    LOG_DEBUG(Service_FS,
              "called, savedata_high={:08X} savedata_low={:08X} "
              "icon_size={:08X} files={:08X} directories={:08X}",
              save_high, save_low, icon_size, directories, files);
}

void Module::Interface::ObsoletedDeleteExtSaveData(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x835, 2, 0);
    const MediaType media_type = static_cast<MediaType>(rp.Pop<u8>());
    const u32 save_low = rp.Pop<u32>();

    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
    rb.Push(fs->archive_manager->DeleteExtSaveData(media_type, 0, save_low));

    LOG_DEBUG(Service_FS, "called, save_low={:08X} media_type={:08X}", save_low,
              static_cast<u32>(media_type));
}

void Module::Interface::GetNumSeeds(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x87D, 0, 0);

    IPC::RequestBuilder rb = rp.MakeBuilder(2, 0);
    rb.Push(RESULT_SUCCESS);
    rb.Push<u32>(0);

    LOG_WARNING(Service_FS, "(STUBBED) called");
}

void Module::Interface::SetSaveDataSecureValue(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x865, 5, 0);
    const u64 value = rp.Pop<u64>();
    const u32 secure_value_slot = rp.Pop<u32>();
    const u32 unique_id = rp.Pop<u32>();
    const u8 title_variation = rp.Pop<u8>();

    // TODO: Generate and Save the Secure Value

    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
    rb.Push(RESULT_SUCCESS);

    LOG_WARNING(Service_FS,
                "(STUBBED) called, value=0x{:016x} secure_value_slot=0x{:08X} "
                "unqiue_id=0x{:08X} title_variation=0x{:02X}",
                value, secure_value_slot, unique_id, title_variation);
}

void Module::Interface::GetSaveDataSecureValue(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x866, 3, 0);
    const u32 secure_value_slot = rp.Pop<u32>();
    const u32 unique_id = rp.Pop<u32>();
    const u8 title_variation = rp.Pop<u8>();

    // TODO: Implement Secure Value Lookup & Generation
    IPC::RequestBuilder rb = rp.MakeBuilder(4, 0);
    rb.Push(RESULT_SUCCESS);
    rb.Push<bool>(false); // indicates that the secure value doesn't exist
    rb.Push<u64>(0);      // the secure value

    LOG_WARNING(
        Service_FS,
        "(STUBBED) called secure_value_slot=0x{:08X} unqiue_id=0x{:08X} title_variation=0x{:02X}",
        secure_value_slot, unique_id, title_variation);
}

Module::Interface::Interface(std::shared_ptr<Module> fs, const char* name, u32 max_session)
    : ServiceFramework(name, max_session), fs(std::move(fs)) {}

Module::Interface::~Interface() {}

Module::Module() {
    archive_manager = std::make_unique<ArchiveManager>();
}

Module::~Module() {}

ArchiveManager* Module::GetArchiveManager() {
    return archive_manager.get();
}

std::shared_ptr<Module> Module::current_fs;

std::shared_ptr<Module> Module::GetCurrent() {
    if (!current_fs)
        current_fs = std::make_shared<Module>();
    auto cur_fs(current_fs);
    return std::move(cur_fs);
}

void InstallInterfaces(SM::ServiceManager& service_manager) {
    auto fs = std::make_shared<Module>();
    std::make_shared<FS_LDR>(fs)->InstallAsService(service_manager);
    std::make_shared<FS_REG>(fs)->InstallAsService(service_manager);
    std::make_shared<FS_USER>(fs)->InstallAsService(service_manager);
    Module::current_fs = std::move(fs);
}

} // namespace FS
} // namespace Service