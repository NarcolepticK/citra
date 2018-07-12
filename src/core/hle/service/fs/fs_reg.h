// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <unordered_map>
#include <boost/container/flat_map.hpp>
#include "core/hle/result.h"
#include "core/hle/service/fs/directory.h"
#include "core/hle/service/fs/file.h"
#include "core/hle/service/fs/fs.h"

namespace Loader {
class AppLoader;
}

namespace Service {
namespace FS {

using FileSys::ArchiveBackend;
using FileSys::ArchiveFactory;

class FS_REG final : public Module::Interface {
public:
    explicit FS_REG(std::shared_ptr<Module> fs);
    ~FS_REG();

    static ArchiveBackend* GetArchive(ArchiveHandle handle);
    static ArchiveFactory* GetRegisteredArchive(ArchiveIdCode id_code);

    static void RegisterSelfNCCH(Loader::AppLoader& app_loader);
    static void RegisterArchiveTypes();
    static void UnregisterArchiveTypes();

    /**
     * Opens an archive
     * @param id_code IdCode of the archive to open
     * @param archive_path Path to the archive, used with Binary paths
     * @return Handle to the opened archive
     */
    static ResultVal<ArchiveHandle> OpenArchive(ArchiveIdCode id_code,
                                                const FileSys::Path& archive_path);

    /**
     * Closes an archive
     * @param handle Handle to the archive to close
     */
    static ResultCode CloseArchive(ArchiveHandle handle);

    /**
     * Open a File from an Archive
     * @param archive_handle Handle to an open Archive object
     * @param path Path to the File inside of the Archive
     * @param mode Mode under which to open the File
     * @return The opened File object
     */
    static ResultVal<std::shared_ptr<File>> OpenFileFromArchive(ArchiveHandle archive_handle,
                                                                const FileSys::Path& path,
                                                                const FileSys::Mode mode);

    /**
     * Delete a File from an Archive
     * @param archive_handle Handle to an open Archive object
     * @param path Path to the File inside of the Archive
     * @return Whether deletion succeeded
     */
    static ResultCode DeleteFileFromArchive(ArchiveHandle archive_handle,
                                            const FileSys::Path& path);

    /**
     * Rename a File between two Archives
     * @param src_archive_handle Handle to the source Archive object
     * @param src_path Path to the File inside of the source Archive
     * @param dest_archive_handle Handle to the destination Archive object
     * @param dest_path Path to the File inside of the destination Archive
     * @return Whether rename succeeded
     */
    static ResultCode RenameFileBetweenArchives(ArchiveHandle src_archive_handle,
                                                const FileSys::Path& src_path,
                                                ArchiveHandle dest_archive_handle,
                                                const FileSys::Path& dest_path);

    /**
     * Delete a Directory from an Archive
     * @param archive_handle Handle to an open Archive object
     * @param path Path to the Directory inside of the Archive
     * @return Whether deletion succeeded
     */
    static ResultCode DeleteDirectoryFromArchive(ArchiveHandle archive_handle,
                                                 const FileSys::Path& path);

    /**
     * Delete a Directory and anything under it from an Archive
     * @param archive_handle Handle to an open Archive object
     * @param path Path to the Directory inside of the Archive
     * @return Whether deletion succeeded
     */
    static ResultCode DeleteDirectoryRecursivelyFromArchive(ArchiveHandle archive_handle,
                                                            const FileSys::Path& path);

    /**
     * Create a File in an Archive
     * @param archive_handle Handle to an open Archive object
     * @param path Path to the File inside of the Archive
     * @param file_size The size of the new file, filled with zeroes
     * @return File creation result code
     */
    static ResultCode CreateFileInArchive(ArchiveHandle archive_handle, const FileSys::Path& path,
                                          u64 file_size);

    /**
     * Create a Directory from an Archive
     * @param archive_handle Handle to an open Archive object
     * @param path Path to the Directory inside of the Archive
     * @return Whether creation of directory succeeded
     */
    static ResultCode CreateDirectoryFromArchive(ArchiveHandle archive_handle,
                                                 const FileSys::Path& path);

    /**
     * Rename a Directory between two Archives
     * @param src_archive_handle Handle to the source Archive object
     * @param src_path Path to the Directory inside of the source Archive
     * @param dest_archive_handle Handle to the destination Archive object
     * @param dest_path Path to the Directory inside of the destination Archive
     * @return Whether rename succeeded
     */
    static ResultCode RenameDirectoryBetweenArchives(ArchiveHandle src_archive_handle,
                                                     const FileSys::Path& src_path,
                                                     ArchiveHandle dest_archive_handle,
                                                     const FileSys::Path& dest_path);

    /**
     * Open a Directory from an Archive
     * @param archive_handle Handle to an open Archive object
     * @param path Path to the Directory inside of the Archive
     * @return The opened Directory object
     */
    static ResultVal<std::shared_ptr<Directory>> OpenDirectoryFromArchive(
        ArchiveHandle archive_handle, const FileSys::Path& path);

    /**
     * Get the free space in an Archive
     * @param archive_handle Handle to an open Archive object
     * @return The number of free bytes in the archive
     */
    static ResultVal<u64> GetFreeBytesInArchive(ArchiveHandle archive_handle);

    /**
     * Erases the contents of the physical folder that contains the archive
     * identified by the specified id code and path
     * @param id_code The id of the archive to format
     * @param format_info Format information about the new archive
     * @param path The path to the archive, if relevant.
     * @return ResultCode 0 on success or the corresponding code on error
     */
    static ResultCode FormatArchive(ArchiveIdCode id_code,
                                    const FileSys::ArchiveFormatInfo& format_info,
                                    const FileSys::Path& path = FileSys::Path());

    /**
     * Retrieves the format info about the archive of the specified type and path.
     * The format info is supplied by the client code when creating archives.
     * @param id_code The id of the archive
     * @param archive_path The path of the archive, if relevant
     * @return The format info of the archive, or the corresponding error code if failed.
     */
    static ResultVal<FileSys::ArchiveFormatInfo> GetArchiveFormatInfo(
        ArchiveIdCode id_code, const FileSys::Path& archive_path);

    /**
     * Creates a blank SharedExtSaveData archive for the specified extdata ID
     * @param media_type The media type of the archive to create (NAND / SDMC)
     * @param high The high word of the extdata id to create
     * @param low The low word of the extdata id to create
     * @param smdh_icon the SMDH icon for this ExtSaveData
     * @param format_info Format information about the new archive
     * @return ResultCode 0 on success or the corresponding code on error
     */
    static ResultCode CreateExtSaveData(MediaType media_type, u32 high, u32 low,
                                        const std::vector<u8>& smdh_icon,
                                        const FileSys::ArchiveFormatInfo& format_info);

    /**
     * Deletes the SharedExtSaveData archive for the specified extdata ID
     * @param media_type The media type of the archive to delete (NAND / SDMC)
     * @param high The high word of the extdata id to delete
     * @param low The low word of the extdata id to delete
     * @return ResultCode 0 on success or the corresponding code on error
     */
    static ResultCode DeleteExtSaveData(MediaType media_type, u32 high, u32 low);

    /**
     * Deletes the SystemSaveData archive folder for the specified save data id
     * @param high The high word of the SystemSaveData archive to delete
     * @param low The low word of the SystemSaveData archive to delete
     * @return ResultCode 0 on success or the corresponding code on error
     */
    static ResultCode DeleteSystemSaveData(u32 high, u32 low);

    /**
     * Creates the SystemSaveData archive folder for the specified save data id
     * @param high The high word of the SystemSaveData archive to create
     * @param low The low word of the SystemSaveData archive to create
     * @return ResultCode 0 on success or the corresponding code on error
     */
    static ResultCode CreateSystemSaveData(u32 high, u32 low);

protected:
    /**
     * FS::Register service function
     *  Inputs:
     *      0 : Header code [0x040103C0]
     *      1 : Process ID to register
     *    2-3 : u64, Program Handle
     *    4-7 : Program Information
     *   8-15 : Storage Information
     *  Outputs:
     *      1 : Result of function, 0 on success, otherwise error code
     */
    void Register(Kernel::HLERequestContext& ctx);

    /**
     * FS::Unregister service function
     *  Inputs:
     *      0 : Header code [0x04020040]
     *      1 : Process ID of program to unregister
     *  Outputs:
     *      1 : Result of function, 0 on success, otherwise error code
     */
    void Unregister(Kernel::HLERequestContext& ctx);

    /**
     * FS::GetProgramInfo service function
     *  Inputs:
     *      0 : Header code [0x040300C0]
     *      1 : Entry Count
     *    2-3 : u64, Program Handle
     *  Outputs:
     *      1 : Result of function, 0 on success, otherwise error code
     */
    void GetProgramInfo(Kernel::HLERequestContext& ctx);

    /**
     * FS::LoadProgram service function
     *  Inputs:
     *      0 : Header code [0x04040100]
     *    1-4 : Program Information
     *  Outputs:
     *      1 : Result of function, 0 on success, otherwise error code
     *    2-3 : u64, Program Handle
     */
    void LoadProgram(Kernel::HLERequestContext& ctx);

    /**
     * FS::UnloadProgram service function
     *  Inputs:
     *      0 : Header code [0x04050080]
     *    1-2 : u64, Program Handle
     *  Outputs:
     *      1 : Result of function, 0 on success, otherwise error code
     */
    void UnloadProgram(Kernel::HLERequestContext& ctx);

    /**
     * FS::CheckHostLoadId service function
     *  Inputs:
     *      0 : Header code [0x04060080]
     *    1-2 : u64, Program Handle
     *  Outputs:
     *      1 : Result of function, 0 on success, otherwise error code
     */
    void CheckHostLoadId(Kernel::HLERequestContext& ctx);

private:
    /**
     * Registers an Archive type, instances of which can later be opened using its IdCode.
     * @param factory File system backend interface to the archive
     * @param id_code Id code used to access this type of archive
     */
    static ResultCode RegisterArchiveType(std::unique_ptr<FileSys::ArchiveFactory>&& factory,
                                          ArchiveIdCode id_code);
    /**
     * Map of registered archives, identified by id code. Once an archive is registered here, it is
     * never removed until UnregisterArchiveTypes is called.
     */
    static boost::container::flat_map<ArchiveIdCode, std::unique_ptr<ArchiveFactory>> id_code_map;

    /**
     * Map of active archive handles. Values are pointers to the archives in `idcode_map`.
     */
    static std::unordered_map<ArchiveHandle, std::unique_ptr<ArchiveBackend>> handle_map;
    static ArchiveHandle next_handle;
};

} // namespace FS
} // namespace Service
