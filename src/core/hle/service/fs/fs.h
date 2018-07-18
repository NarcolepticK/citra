// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/result.h"
#include "core/hle/service/fs/archive_manager.h"
#include "core/hle/service/service.h"

namespace Service {
namespace FS {

class Module final {
public:
    Module();
    ~Module();

    ArchiveManager* GetArchiveManager();
    static std::shared_ptr<Module> GetCurrent();

    class Interface : public ServiceFramework<Interface> {
    public:
        Interface(std::shared_ptr<Module> fs, const char* name, u32 max_session);
        ~Interface();

    protected:
        /**
         * FS::Dummy1 service function
         *  Inputs:
         *      0 : Header code [0x000100C6]
         *      1 : Buffer1 Size
         *      2 : Buffer2 Size
         *      3 : Buffer3 Size
         *      4 : (buffer1Size << 14) | 0x2
         *      5 : void* Buffer1
         *      6 : (buffer2Size << 14) | 0x402
         *      7 : void* Buffer2
         *      8 : (buffer3Size << 14) | 0x802
         *      9 : void* Buffer3
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void Dummy1(Kernel::HLERequestContext& ctx);

        /**
         * FS::Control service function
         *  Inputs:
         *      0 : Header code [0x040100C4]
         *      1 : Action
         *      2 : Input Size
         *      3 : Output Size
         *      4 : (inputSize << 4) | 0xA
         *      5 : void* Input
         *      6 : (outputSize << 4) | 0xC
         *      7 : void* Output
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void Control(Kernel::HLERequestContext& ctx);

        /**
         * FS::Initialize service function
         *  Inputs:
         *      0 : Header code [0x08010002]
         *    1-2 : PID Must be value 32 (ProcessId Header)
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void Initialize(Kernel::HLERequestContext& ctx);

        /**
         * FS::OpenFile service function
         *  Inputs:
         *      0 : Header code [0x080201C2]
         *      1 : Transaction
         *    2-3 : u64, Archive Handle
         *      4 : Path Type
         *      5 : Path Size
         *      6 : Open Flags
         *      7 : Attributes
         *      8 : (PathSize << 14) | 2
         *      9 : Path data pointer
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *      3 : File handle
         */
        void OpenFile(Kernel::HLERequestContext& ctx);

        /**
         * FS::OpenFileDirectly service function
         *  Inputs:
         *      0 : Header code [0x08030204]
         *      1 : Transaction
         *      2 : Archive ID
         *      3 : Archive Path Type
         *      4 : Archive Path Size
         *      5 : File Path Type
         *      6 : File Path Size
         *      7 : Open Flags
         *      8 : Attributes
         *      9 : (ArchivePathSize << 14) | 0x802
         *      10 : Archive Path Data Pointer
         *      11 : (FileLowPathSize << 14) | 2
         *      12 : File Path Data Pointer
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *      3 : File handle
         */
        void OpenFileDirectly(Kernel::HLERequestContext& ctx);

        /*
         * FS::DeleteFile service function
         *  Inputs:
         *      0 : Header code [0x08040142]
         *      1 : Transaction
         *    2-3 : u64, Archive Handle
         *      4 : Path Type
         *      5 : Path Size
         *      6 : (PathSize << 14) | 2
         *      7 : Path Data Pointer
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void DeleteFile(Kernel::HLERequestContext& ctx);

        /*
         * FS::RenameFile service function
         *  Inputs:
         *      0 : Header code [0x08050244]
         *      1 : Transaction
         *    2-3 : u64, Source Archive Handle
         *      4 : Source File Path Type
         *      5 : Source File Path Size
         *    6-7 : u64, Destination Archive Handle
         *      8 : Dest File Path Type
         *      9 : Dest File Path Size
         *     10 : (SourceFilePathSize << 14) | 0x402
         *     11 : Source File Path Data
         *     12 : (DestinationFilePathSize << 14) | 0x802
         *     13 : Destination File Path Data
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void RenameFile(Kernel::HLERequestContext& ctx);

        /*
         * FS::DeleteDirectory service function
         *  Inputs:
         *      0 : Header code [0x08060142]
         *      1 : Transaction
         *    2-3 : u64, Archive Handle
         *      4 : Path Type
         *      5 : Path Size
         *      6 : (PathSize << 14) | 0x2
         *      7 : Path Data Pointer
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void DeleteDirectory(Kernel::HLERequestContext& ctx);

        /*
         * FS::DeleteDirectoryRecursively service function
         *  Inputs:
         *      0 : Header code [0x08070142]
         *      1 : Transaction
         *    2-3 : u64, Archive Handle
         *      4 : Path Type
         *      5 : Path Size
         *      6 : (PathSize << 14) | 0x2
         *      7 : Path Data Pointer
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void DeleteDirectoryRecursively(Kernel::HLERequestContext& ctx);

        /*
         * FS::CreateFile service function
         *  Inputs:
         *      0 : Header code [0x08080202]
         *      1 : Transaction
         *    2-3 : u64, Archive Handle
         *      4 : Path Type
         *      5 : Path Size
         *      6 : Attributes
         *    7-8 : u64, Bytes to fill with zeroes in the file
         *      9 : (PathSize << 14) | 2
         *     10 : Path Data Pointer
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void CreateFile(Kernel::HLERequestContext& ctx);

        /*
         * FS::CreateDirectory service function
         *  Inputs:
         *      0 : Header code [0x08090182]
         *      1 : Transaction
         *    2-3 : u64, Archive Handle
         *      4 : Path Type
         *      5 : Path Size
         *      6 : Attributes
         *      7 : (PathSize << 14) | 2
         *      8 : Path Data Pointer
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void CreateDirectory(Kernel::HLERequestContext& ctx);

        /*
         * FS::RenameDirectory service function
         *  Inputs:
         *      0 : Header code [0x080A0244]
         *      1 : Transaction
         *    2-3 : u64, Source Archive Handle
         *      4 : Source Directory Path Type
         *      5 : Source Directory Path Size
         *    6-7 : u64, Destination Archive Handle
         *      8 : Destination Directory Path Type
         *      9 : Destination Directory Path Size
         *     10 : (SourceDirectoryPathSize << 14) | 0x402
         *     11 : Source Directory Path Data Pointer
         *     12 : (DestinationDirectoryPathSize << 14) | 0x802
         *     13 : Destination Directory Path Data Pointer
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void RenameDirectory(Kernel::HLERequestContext& ctx);

        /**
         * FS::OpenDirectory service function
         *  Inputs:
         *      0 : Header code [0x080B0102]
         *    1-2 : u64, Archive Handle
         *      3 : Path Type
         *      4 : Path Size
         *      7 : (PathSize << 14) | 2
         *      8 : Path Data Pointer
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *      3 : Directory handle
         */
        void OpenDirectory(Kernel::HLERequestContext& ctx);

        /**
         * FS::OpenArchive service function
         *  Inputs:
         *      0 : Header code [0x080C00C2]
         *      1 : Archive ID
         *      2 : Path Type
         *      3 : Path Size
         *      4 : (LowPathSize << 14) | 2
         *      5 : Path Data Pointer
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *    2-3 : u64, Archive Handle
         */
        void OpenArchive(Kernel::HLERequestContext& ctx);

        /**
         * FS::ControlArchive service function
         *  Inputs:
         *      0 : Header code [0x080D0144]
         *    1-2 : u64, Archive Handle
         *      3 : Action
         *      4 : Input Size
         *      5 : Output Size
         *      6 : (inputSize << 4) | 0xA
         *      7 : void* Input
         *      8 : (outputSize << 4) | 0xC
         *      9 : void* Output
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void ControlArchive(Kernel::HLERequestContext& ctx);

        /**
         * FS::CloseArchive service function
         *  Inputs:
         *      0 : Header code [0x080E0080]
         *    1-2 : u64, Archive Handle
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void CloseArchive(Kernel::HLERequestContext& ctx);

        /**
         * FS::LegacyFormatThisUserSaveData service function
         * This has been obsoleted. Prefer FS:FormatSaveData.
         *  Inputs:
         *      0  : Header code [0x080F0180]
         *      1  : Size in Blocks (1 block = 512 bytes)
         *      2  : Number of Directories
         *      3  : Number of Files
         *      4  : Directory Bucket Count
         *      5  : File Bucket Count
         *      6  : u8, 0 = don't duplicate data, 1 = duplicate data
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void LegacyFormatThisUserSaveData(Kernel::HLERequestContext& ctx);

        /**
         * FS::LegacyCreateSystemSaveData service function.
         *  This has been obsoleted. It has been replaced with the new FS:CreateSystemSaveData.
         *  Inputs:
         *      0 : Header code [0x08100200]
         *      1 : Save ID
         *      2 : Total Size
         *      3 : Block Size
         *      4 : Number of Directories
         *      5 : Number of Files
         *      6 : Directory Bucket Count
         *      7 : File Bucket Count
         *      8 : u8, 0 = don't duplicate data, 1 = duplicate data
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void LegacyCreateSystemSaveData(Kernel::HLERequestContext& ctx);

        /**
         * FS::LegacyDeleteSystemSaveData service function
         * This has been obsoleted. It has been replaced with the new FS:DeleteSystemSaveData.
         *  Inputs:
         *      0 : Header code [0x08110040]
         *      1 : Save ID
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void LegacyDeleteSystemSaveData(Kernel::HLERequestContext& ctx);

        /**
         * FS::GetFreeBytes service function
         *  Inputs:
         *     0 : Header code [0x08120080]
         *     1 : u64, Archive Handle
         *  Outputs:
         *     1 : Result of function, 0 on success, otherwise error code
         *     2 : u64, Free Bytes
         */
        void GetFreeBytes(Kernel::HLERequestContext& ctx);

        /**
         * FS::GetCardType service function
         *  Inputs:
         *      0 : Header code [0x08130000]
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *      2 : u8, CTR card = 0, TWL card = 1
         */
        void GetCardType(Kernel::HLERequestContext& ctx);

        /**
         * FS::GetSdmcArchiveResource service function
         *  Inputs:
         *      0 : Header code [0x08140000]
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *    2-5 : Archive Resource
         */
        void GetSdmcArchiveResource(Kernel::HLERequestContext& ctx);

        /**
         * FS::GetNandArchiveResource service function
         *  Inputs:
         *      0 : Header code [0x08150000]
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *    2-5 : Archive Resource
         */
        void GetNandArchiveResource(Kernel::HLERequestContext& ctx);

        /**
         * FS::GetSdmcFatfsError service function
         *  Inputs:
         *      0 : Header code [0x08160000]
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *      2 : Fatfs Error
         */
        void GetSdmcFatfsError(Kernel::HLERequestContext& ctx);

        /*
         * FS::IsSdmcDetected service function
         *  Inputs:
         *      0 : Header code [0x08170000]
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *      2 : u8, Whether the Sdmc is detected
         */
        void IsSdmcDetected(Kernel::HLERequestContext& ctx);

        /**
         * FS::IsSdmcWriteable service function
         *  Inputs:
         *      0 : Header code [0x08180000]
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *      2 : u8, Wheter Sdmc is writable
         */
        void IsSdmcWriteable(Kernel::HLERequestContext& ctx);

        /**
         * FS::GetSdmcCid service function
         *  Inputs:
         *      0 : Header code [0x08190042]
         *      1 : Buffer Length (should be 0x10)
         *      2 : (bufferLength << 4) | 0xC
         *      3 : void* Buffer
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void GetSdmcCid(Kernel::HLERequestContext& ctx);

        /**
         * FS::GetNandCid service function
         *  Inputs:
         *      0 : Header code [0x081A0042]
         *      1 : Buffer Length (should be 0x10)
         *      2 : (bufferLength << 4) | 0xC
         *      3 : void* Buffer
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void GetNandCid(Kernel::HLERequestContext& ctx);

        /**
         * FS::GetSdmcSpeedInfo service function
         *  Inputs:
         *      0 : Header code [0x081B0000]
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *      2 : Speed Information
         */
        void GetSdmcSpeedInfo(Kernel::HLERequestContext& ctx);

        /**
         * FS::GetNandSpeedInfo service function
         *  Inputs:
         *      0 : Header code [0x081C0000]
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *      2 : Speed Information
         */
        void GetNandSpeedInfo(Kernel::HLERequestContext& ctx);

        /**
         * FS::GetSdmcLog service function
         *  Inputs:
         *      0 : Header code [0x081D0042]
         *      1 : Buffer Length
         *      2 : (bufferLength << 4) | 0xC
         *      3 : void* Buffer
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void GetSdmcLog(Kernel::HLERequestContext& ctx);

        /**
         * FS::GetNandLog service function
         *  Inputs:
         *      0 : Header code [0x081E0042]
         *      1 : Buffer Length
         *      2 : (bufferLength << 4) | 0xC
         *      3 : void* Buffer
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void GetNandLog(Kernel::HLERequestContext& ctx);

        /**
         * FS::ClearSdmcLog service function
         *  Inputs:
         *      0 : Header code [0x081F0000]
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void ClearSdmcLog(Kernel::HLERequestContext& ctx);

        /**
         * FS::ClearNandLog service function
         *  Inputs:
         *      0 : Header code [0x08200000]
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void ClearNandLog(Kernel::HLERequestContext& ctx);

        /**
         * FS::CardSlotIsInserted service function.
         *  Inputs:
         *      0 : Header code [0x08210000]
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *      2 : u8, 0 = No card inserted, 1 = card inserted
         */
        void CardSlotIsInserted(Kernel::HLERequestContext& ctx);

        /**
         * FS::CardSlotPowerOn service function
         *  Inputs:
         *      0 : Header code [0x08220000]
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *      2 : u8, Status, 0 = off, 1 = on
         */
        void CardSlotPowerOn(Kernel::HLERequestContext& ctx);

        /**
         * FS::CardSlotPowerOff service function
         *  Inputs:
         *      0 : Header code [0x08230000]
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *      2 : u8, Status, 0 = off, 1 = on
         */
        void CardSlotPowerOff(Kernel::HLERequestContext& ctx);

        /**
         * FS::CardSlotGetCardIFPowerStatus service function
         *  Inputs:
         *      0 : Header code [0x08240000]
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *      2 : u8, Status, 0 = off, 1 = on
         */
        void CardSlotGetCardIFPowerStatus(Kernel::HLERequestContext& ctx);

        /**
         * FS::CardNorDirectCommand service function
         *  Inputs:
         *      0 : Header code [0x08250040]
         *      1 : u8, CommandID
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void CardNorDirectCommand(Kernel::HLERequestContext& ctx);

        /**
         * FS::CardNorDirectCommandWithAddress service function
         *  Inputs:
         *      0 : Header code [0x08260080]
         *      1 : u8, CommandID
         *      2 : u32, Address
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void CardNorDirectCommandWithAddress(Kernel::HLERequestContext& ctx);

        /**
         * FS::CardNorDirectRead service function
         *  Inputs:
         *      0 : Header code [0x08270082]
         *      1 : u8, CommandID
         *      2 : u32, Size
         *      3 : (Size<<6) | 0xC
         *      4 : void* Output Buffer
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void CardNorDirectRead(Kernel::HLERequestContext& ctx);

        /**
         * FS::CardNorDirectReadWithAddress service function
         *  Inputs:
         *      0 : Header code [0x082800C2]
         *      1 : u8, CommandID
         *      2 : u32, Address
         *      3 : u32, Size
         *      4 : (Size<<6) | 0xC
         *      5 : void* Output Buffer
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void CardNorDirectReadWithAddress(Kernel::HLERequestContext& ctx);

        /**
         * FS::CardNorDirectWrite service function
         *  Inputs:
         *      0 : Header code [0x08290082]
         *      1 : u8, CommandID
         *      2 : u32, Size
         *      3 : (Size<<6) | 0xC
         *      4 : void* Input Buffer
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void CardNorDirectWrite(Kernel::HLERequestContext& ctx);

        /**
         * FS::CardNorDirectWriteWithAddress service function
         *  Inputs:
         *      0 : Header code [0x082A00C2]
         *      1 : u8, CommandID
         *      2 : u32, Address
         *      3 : u32, Size
         *      4 : (Size<<6) | 0xC
         *      5 : void* Input Buffer
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void CardNorDirectWriteWithAddress(Kernel::HLERequestContext& ctx);

        /**
         * FS::CardNorDirectRead_4xIO service function
         *  Inputs:
         *      0 : Header code [0x082B00C2]
         *      1 : u8, CommandID
         *      2 : u32, Address
         *      3 : u32, Size
         *      4 : (Size<<6) | 0xC
         *      5 : void* Output Buffer
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void CardNorDirectRead_4xIO(Kernel::HLERequestContext& ctx);

        /**
         * FS::CardNorDirectCpuWriteWithoutVerify service function
         *  Inputs:
         *      0 : Header code [0x082C0082]
         *      1 : u32, Address
         *      2 : u32, Size
         *      3 : (Size<<6) | 0xA
         *      4 : void* Input Buffer
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void CardNorDirectCpuWriteWithoutVerify(Kernel::HLERequestContext& ctx);

        /**
         * FS::CardNorDirectSectorEraseWithoutVerify service function
         *  Inputs:
         *      0 : Header code [0x082D0040]
         *      1 : u32, Address
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void CardNorDirectSectorEraseWithoutVerify(Kernel::HLERequestContext& ctx);

        /**
         * FS::GetProductInfo service function
         *  Inputs:
         *      0 : Header code [0x082E0040]
         *      1 : u32, ProcessID
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *    2-6 : Product Information
         */
        void GetProductInfo(Kernel::HLERequestContext& ctx);

        /**
         * FS::GetProgramLaunchInfo service function.
         *  Inputs:
         *      0 : Header code [0x082F0040]
         *      1 : ProcessID
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *    2-5 : Program Information
         */
        void GetProgramLaunchInfo(Kernel::HLERequestContext& ctx);

        /**
         * FS::ObsoletedCreateExtSaveData service function
         * This has been obsoleted. It now wraps around the new FS:CreateExtSaveData.
         *  Inputs:
         *      0 : Header code [0x08300182]
         *      1 : Media Type
         *    2-3 : u64, Save ID
         *      4 : SMDH Size
         *      5 : Number of directories
         *      6 : Number of files
         *      7 : (SMDH Size << 4) | 0xA
         *      8 : SMDH Data Pointer
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void ObsoletedCreateExtSaveData(Kernel::HLERequestContext& ctx);

        /**
         * FS::LegacyCreateSharedExtData service function
         * This has been obsoleted. It now wraps around the new FS:CreateExtSaveData.
         *  Inputs:
         *      0 : Header code [0x08310180]
         *      1 : Media Type
         *    2-3 : u64, Save ID
         *      4 : Number of Directories
         *      5 : Number of Files
         *      6 : Size Limit
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void LegacyCreateSharedExtData(Kernel::HLERequestContext& ctx);

        /**
         * FS::LegacyReadExtSaveDataIcon service function
         *  Inputs:
         *      0 : Header code [0x08320102]
         *      1 : Media Type
         *    2-3 : u64, Save ID
         *      4 : SMDH Size
         *      5 : (SMDH size<<4) | 12
         *      6 : SMDH Data Pointer
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *      2 : Total read data
         */
        void LegacyReadExtSaveDataIcon(Kernel::HLERequestContext& ctx);

        /**
         * FS::LegacyEnumerateExtSaveData service function
         * This has been obsoleted. It now wraps around the new FS:EnumerateExtSaveData.
         *  Inputs:
         *      0 : Header code [0x08330082]
         *      1 : Output ID Count (size / 8)
         *      2 : Media Type
         *      3 : (outputBytes << 4) | 0xC
         *      4 : Output Pointer
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *      2 : Written ID Count
         */
        void LegacyEnumerateExtSaveData(Kernel::HLERequestContext& ctx);

        /**
         * FS::LegacyEnumerateSharedExtSaveData service function
         * This has been obsoleted. It now wraps around the new FS:EnumerateExtSaveData.
         *  Inputs:
         *      0 : Header code [0x08340082]
         *      1 : Output ID Count (size / 4)
         *      2 : Media Type
         *      3 : (outputBytes << 4) | 0xC
         *      4 : Output Pointer
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *      2 : Written ID Count
         */
        void LegacyEnumerateSharedExtSaveData(Kernel::HLERequestContext& ctx);

        /**
         * FS::ObsoletedDeleteExtSaveData service function
         * This has been obsoleted. It now wraps around the new FS:DeleteExtSaveData.
         * The Save ID High value is fixed to 0x00000000 (normal).
         *  Inputs:
         *      0 : Header code [0x08350080]
         *      1 : Media Type
         *      2 : Save ID Low
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void ObsoletedDeleteExtSaveData(Kernel::HLERequestContext& ctx);

        /**
         * FS::LegacyDeleteSharedExtSaveData service function
         * This has been obsoleted. It now wraps around the new FS:DeleteExtSaveData.
         * The Save ID High value is fixed to 0x00048000 (shared).
         *  Inputs:
         *      0 : Header code [0x08360080]
         *      1 : Media Type
         *      2 : Save ID Low
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void LegacyDeleteSharedExtSaveData(Kernel::HLERequestContext& ctx);

        /**
         * FS::SetCardSpiBaudRate service function
         *  Inputs:
         *      0 : Header code [0x08370040]
         *      1 : Baud Rate
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void SetCardSpiBaudRate(Kernel::HLERequestContext& ctx);

        /**
         * FS::SetCardSpiBusMode service function
         *  Inputs:
         *      0 : Header code [0x08380040]
         *      1 : Bus Mode
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void SetCardSpiBusMode(Kernel::HLERequestContext& ctx);

        /**
         * FS::SendInitializeInfoTo9 service function
         *  Inputs:
         *      0 : Header code [0x08390000]
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void SendInitializeInfoTo9(Kernel::HLERequestContext& ctx);

        /**
         * FS::GetSpecialContentIndex service function
         *  Inputs:
         *      0 : Header code [0x083A0100]
         *      1 : Media Type
         *    2-3 : u64, Program ID
         *      4 : Special Content Type
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *      2 : u16, Special Content Index
         */
        void GetSpecialContentIndex(Kernel::HLERequestContext& ctx);

        /**
         * FS::GetLegacyRomHeader service function
         *  Inputs:
         *      0 : Header code [0x083B00C2]
         *      1 : Media Type
         *    2-3 : u64, Program ID
         *      4 : (0x3B4 << 4) | 0xC
         *      5 : Output Pointer
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void GetLegacyRomHeader(Kernel::HLERequestContext& ctx);

        /**
         * FS::GetLegacyBannerData service function
         *  Inputs:
         *      0 : Header code [0x083C00C2]
         *      1 : Media Type
         *    2-3 : u64, Program ID
         *      4 : (0x23C0 << 4) | 0xC
         *      5 : Output Pointer
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void GetLegacyBannerData(Kernel::HLERequestContext& ctx);

        /**
         * FS::CheckAuthorityToAccessExtSaveData service function
         *  Inputs:
         *      0 : Header code [0x083D0100]
         *      1 : Media Type
         *    2-3 : u64, Save ID
         *      4 : Process ID
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *      2 : u8, 0 = no access, 1 = has access
         */
        void CheckAuthorityToAccessExtSaveData(Kernel::HLERequestContext& ctx);

        /**
         * FS::QueryTotalQuotaSize service function
         *  Inputs:
         *      0 : Header code [0x083E00C2]
         *      1 : Number of Directories
         *      2 : Number of Files
         *      3 : Files Size Count
         *      4 : ((FileSizeCount * 8) << 4) | 0xA
         *      5 : File Size Buffer Pointer
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *    2-3 : u64, Total Quota Size
         */
        void QueryTotalQuotaSize(Kernel::HLERequestContext& ctx);

        /**
         * FS::LegacyGetExtDataBlockSize service function
         * This has been obsoleted. It now wraps around the new FS:GetExtDataBlockSize.
         *  Inputs:
         *      0 : Header code [0x083F00C0]
         *      1 : Media Type
         *    2-3 : u64, Save ID
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *    2-3 : u64, Total Blocks
         *    4-5 : u64, Remaining Blocks
         *      6 : Block Size
         */
        void LegacyGetExtDataBlockSize(Kernel::HLERequestContext& ctx);

        /**
         * FS::AbnegateAccessRight service function
         *  Inputs:
         *      0 : Header code [0x08400040]
         *      1 : Access Right (should be < 0x38)
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void AbnegateAccessRight(Kernel::HLERequestContext& ctx);

        /**
         * FS::DeleteSdmcRoot service function
         * This deletes the "/Nintendo 3DS/<SomeID>/" directory from SD card.
         *  Inputs:
         *      0 : Header code [0x08410000]
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void DeleteSdmcRoot(Kernel::HLERequestContext& ctx);

        /**
         * FS::DeleteAllExtSaveDataOnNand service function
         *  Inputs:
         *      0 : Header code [0x08420040]
         *      1 : Transaction
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void DeleteAllExtSaveDataOnNand(Kernel::HLERequestContext& ctx);

        /**
         * FS::InitializeCtrFileSystem service function
         *  Inputs:
         *      0 : Header code [0x08430000]
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void InitializeCtrFileSystem(Kernel::HLERequestContext& ctx);

        /**
         * FS::CreateSeed service function
         *  Inputs:
         *      0 : Header code [0x08440000]
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void CreateSeed(Kernel::HLERequestContext& ctx);

        /**
         * FS::GetFormatInfo service function.
         *  Inputs:
         *      0 : Header code [0x084500C2]
         *      1 : Archive ID
         *      2 : Path Type
         *      3 : Path Size
         *      4 : (PathSize << 14) | 2
         *      5 : Path Data Pointer
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *      2 : Total Size
         *      3 : Number of Directories
         *      4 : Number of Files
         *      5 : u8, 0 = don't duplicate data, 1 = duplicate data
         */
        void GetFormatInfo(Kernel::HLERequestContext& ctx);

        /**
         * FS::GetLegacyRomHeader2 service function
         *  Inputs:
         *      0 : Header code [0x08460102]
         *      1 : Output Size
         *      2 : Media Type
         *    3-4 : u64, Program ID
         *      5 : (OutputSize << 4) | 0xC
         *      6 : Output Pointer
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void GetLegacyRomHeader2(Kernel::HLERequestContext& ctx);

        /**
         * FS::LegacyFormatCtrCardUserSaveData service function
         * This has been obsoleted. Prefer FS:FormatSaveData.
         *  Inputs:
         *      0 : Header code [0x08470180]
         *      1 : Size in Blocks (1 Block = 512 bytes)
         *      2 : Number of Directories
         *      3 : Number of Files
         *      4 : Directory Bucket Count
         *      5 : File Bucket Count
         *      6 : u8, 0 = don't duplicate data, 1 = duplicate data
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void LegacyFormatCtrCardUserSaveData(Kernel::HLERequestContext& ctx);

        /**
         * FS::GetSdmcCtrRootPath service function
         * This retrieves the /Nintendo 3DS/<ID0>/<ID1> path.
         *  Inputs:
         *      0 : Header code [0x08480042]
         *      1 : Size of output buffer in wide-characters
         *      2 : (bytesize << 4) | 0xC
         *      3 : Output wchar buffer pointer
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void GetSdmcCtrRootPath(Kernel::HLERequestContext& ctx);

        /**
         * FS::GetArchiveResource service function.
         *  Inputs:
         *      0 : Header code [0x08490040]
         *      1 : System Media type
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *    2-5 : Archive Resource
         */
        void GetArchiveResource(Kernel::HLERequestContext& ctx);

        /**
         * FS::ExportIntegrityVerificationSeed service function
         *  Inputs:
         *      0 : Header code [0x084A0002]
         *      1 : (0x130 << 4) | 0xC
         *      2 : Output Integrity Verification Seed Pointer
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void ExportIntegrityVerificationSeed(Kernel::HLERequestContext& ctx);

        /**
         * FS::ImportIntegrityVerificationSeed service function
         *  Inputs:
         *      0 : Header code [0x084B0002]
         *      1 : (0x130 << 4) | 0xA
         *      2 : Input Integrity Verification Seed Pointer
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void ImportIntegrityVerificationSeed(Kernel::HLERequestContext& ctx);

        /**
         * FS::FormatSaveData service function,
         * Formats the SaveData specified by the input path.
         *  Inputs:
         *      0  : Header code [0x084C0242]
         *      1  : Archive ID
         *      2  : Path Type
         *      3  : Path Size
         *      4  : Size in Blocks (1 block = 512 bytes)
         *      5  : Number of Directories
         *      6  : Max Number of Files
         *      7  : Directory Bucket Count
         *      8  : File Bucket Count
         *      9  : u8, 0 = don't duplicate data, 1 = duplicate data
         *      10 : (PathSize << 14) | 2
         *      11 : Path Data Pointer
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void FormatSaveData(Kernel::HLERequestContext& ctx);

        /**
         * FS::GetLegacySubBannerData service function
         *  Inputs:
         *      0 : Header code [0x084D0102]
         *      1 : Output Size
         *      2 : Media Type
         *    3-4 : u64, Program ID
         *      5 : (OutputSize << 4) | 0xC
         *      6 : Output Pointer
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void GetLegacySubBannerData(Kernel::HLERequestContext& ctx);

        /**
         * FS::UpdateSha256Context service function
         *  Inputs:
         *      0 : Header code [0x084E0342]
         *    1-8 : Input Hash. Not used at all.
         *      9 : Input Data Size
         *     10 : u32, Flag, must be zero.
         *     11 : u32, Flag, must be zero.
         *     12 : s8, Flag, must be zero.
         *     13 : s8, Flag, must be zero.
         *     14 : (Size << 4) | 0xA
         *     15 : Input Data Pointer
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *    2-9 : Output SHA-256 Hash
         */
        void UpdateSha256Context(Kernel::HLERequestContext& ctx);

        /**
         * FS::ReadSpecialFile service function
         *  Inputs:
         *      0 : Header code [0x084F0102]
         *      1 : Must be 0x0.
         *    2-3 : u64, File Offset
         *      4 : Size
         *      5 : (Size << 4) | 0xC
         *      6 : Data Pointer
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *      2 : Total Read Data
         */
        void ReadSpecialFile(Kernel::HLERequestContext& ctx);

        /**
         * FS::GetSpecialFileSize service function
         *  Inputs:
         *      0 : Header code [0x08500040]
         *      1 : Must be 0x0.
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *    2-3 : u64, File Size
         */
        void GetSpecialFileSize(Kernel::HLERequestContext& ctx);

        /**
         * FS::CreateExtSaveData service function
         *  Inputs:
         *      0 : Header code [0x08510242]
         *    1-4 : ExtSaveDataInfo
         *      5 : Number of Directories
         *      6 : Number of Files
         *    7-8 : u64, Size Limit (default = -1)
         *      9 : SMDH Size
         *     10 : (SMDH Size << 4) | 0xA
         *     11 : SMDH Data Pointer
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void CreateExtSaveData(Kernel::HLERequestContext& ctx);

        /**
         * FS::DeleteExtSaveData service function
         *  Inputs:
         *      0 : Header code [0x08520100]
         *    1-4 : ExtSaveDataInfo
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void DeleteExtSaveData(Kernel::HLERequestContext& ctx);

        /**
         * FS::ReadExtSaveDataIcon service function
         *  Inputs:
         *      0 : Header code [0x08530142]
         *    1-4 : ExtSaveDataInfo
         *      5 : SMDH Size
         *      6 : (SMDH Size << 4) | 0xC
         *      7 : SMDH Data Pointer
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *      2 : Total Read Data
         */
        void ReadExtSaveDataIcon(Kernel::HLERequestContext& ctx);

        /**
         * FS:: GetExtDataBlockSize service function
         *  Inputs:
         *      0 : Header code [0x08540100]
         *    1-4 : ExtSaveDataInfo
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *    2-3 : u64, Total Blocks
         *    4-5 : u64, Remaining Blocks
         *      6 : Block Size
         */
        void GetExtDataBlockSize(Kernel::HLERequestContext& ctx);

        /**
         * FS::EnumerateExtSaveData service function
         *  Inputs:
         *      0 : Header code [0x8550102]
         *      1 : Output ID Buffer Size
         *      2 : Media Type
         *      3 : ID Entry Size (usually 4 or 8)
         *      4 : u8, 0 = Non-Shared, 1 = Shared
         *      5 : (outputBufferSize << 4) | 0xC
         *      6 : Output Pointer
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *      2 : Written ID Count
         */
        void EnumerateExtSaveData(Kernel::HLERequestContext& ctx);

        /**
         * FS::CreateSystemSaveData service function.
         *  Inputs:
         *      0 : Header code [0x08560240]
         *    1-2 : SystemSaveDataInfo
         *      3 : Total Size
         *      4 : Block Size
         *      5 : Number of Directories
         *      6 : Number of Files
         *      7 : Directory Bucket Count
         *      8 : File Bucket Count
         *      9 : u8, 0 = don't duplicate data, 1 = duplicate data
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void CreateSystemSaveData(Kernel::HLERequestContext& ctx);

        /**
         * FS::DeleteSystemSaveData service function.
         *  Inputs:
         *      0 : Header code [0x08570080]
         *    1-2 : SystemSaveDataInfo
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void DeleteSystemSaveData(Kernel::HLERequestContext& ctx);

        /**
         * FS::StartDeviceMoveAsSource service function
         *  Inputs:
         *      0 : Header code [0x08580000]
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *    2-9 : Device Move Context
         */
        void StartDeviceMoveAsSource(Kernel::HLERequestContext& ctx);

        /**
         * FS::StartDeviceMoveAsDestination service function
         *  Inputs:
         *      0 : Header code [0x08590240]
         *    1-8 : Device Move Context
         *      9 : u8, 0 = don't clear, 1 = clear
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void StartDeviceMoveAsDestination(Kernel::HLERequestContext& ctx);

        /**
         * FS::SetArchivePriority service function
         *  Inputs:
         *      0 : Header code [0x085A00C0]
         *    1-2 : u64, Archive Handle
         *      3 : Priority
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void SetArchivePriority(Kernel::HLERequestContext& ctx);

        /**
         * FS::GetArchivePriority service function
         *  Inputs:
         *      0 : Header code [0x085B0080]
         *    1-2 : u64, Archive Handle
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *      2 : Priority
         */
        void GetArchivePriority(Kernel::HLERequestContext& ctx);

        /**
         * FS::SetCtrCardLatencyParameter service function
         *  Inputs:
         *      0 : Header code [0x085C00C0]
         *    1-2 : u64, Latency (milliseconds)
         *      3 : u8, 0 = don't emulate endurance, 1 = emulate endurance
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void SetCtrCardLatencyParameter(Kernel::HLERequestContext& ctx);

        /**
         * FS::SetFsCompatibilityInfo service function
         *  Inputs:
         *      0 : Header code [0x085D01C0]
         *    1-2 : u64, Title ID
         *      3 : Media Type
         *      4 : Unknown
         *      5 : Unknown
         *      6 : Unknown
         *      7 : Unknown
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void SetFsCompatibilityInfo(Kernel::HLERequestContext& ctx);

        /**
         * FS::ResetCardCompatibilityParameter service function
         *  Inputs:
         *      0 : Header code [0x085E0040]
         *      1 : Unknown
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void ResetCardCompatibilityParameter(Kernel::HLERequestContext& ctx);

        /**
         * FS::SwitchCleanupInvalidSaveData service function
         *  Inputs:
         *      0 : Header code [0x085F0040]
         *      1 : u8, 0 = disable, 1 = enable
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void SwitchCleanupInvalidSaveData(Kernel::HLERequestContext& ctx);

        /**
         * FS::EnumerateSystemSaveData service function
         *  Inputs:
         *      0 : Header code [0x08600042]
         *      1 : Output Buffer Size
         *      2 : (Size << 4) | 0xC
         *      3 : Output Buffer
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *      2 : Total Save IDs written to the output buffer
         */
        void EnumerateSystemSaveData(Kernel::HLERequestContext& ctx);

        /**
         * FS::InitializeWithSdkVersion service function.
         *  Inputs:
         *      0 : Header code [0x08610042]
         *      1 : SDK Version
         *      2 : Must be value 32 (ProcessId Header)
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void InitializeWithSdkVersion(Kernel::HLERequestContext& ctx);

        /**
         * FS::SetPriority service function.
         *  Inputs:
         *      0 : Header code [0x08620040]
         *      1 : Priority
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void SetPriority(Kernel::HLERequestContext& ctx);

        /**
         * FS::GetPriority service function.
         *  Inputs:
         *      0 : Header code [0x08630000]
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *      2 : Priority
         */
        void GetPriority(Kernel::HLERequestContext& ctx);

        /**
         * FS::LegacyGetNandInfo service function
         * This command is stubbed and returns error 0xE0C046F8.
         * It is unknown what its response values meant.
         *  Inputs:
         *      0 : Header code [0x08640000]
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *      2 : ?
         *      3 : ?
         *      4 : ?
         *      5 : ?
         *      6 : ?
         *      7 : ?
         *      8 : ?
         */
        void LegacyGetNandInfo(Kernel::HLERequestContext& ctx);

        /**
         * FS::SetSaveDataSecureValue service function.
         *  Inputs:
         *      0 : Header code [0x08650140]
         *    1-2 : u64, Secure Value
         *      3 : Secure Value Slot
         *      4 : Title Unique Id (0 = current)
         *      5 : u8, Title Variation (0 = current)
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void SetSaveDataSecureValue(Kernel::HLERequestContext& ctx);

        /**
         * FS::GetSaveDataSecureValue service function.
         *  Inputs:
         *      0 : Header code [0x086600C0]
         *      1 : Secure Value Slot
         *      2 : Title Unique Id (0 = current)
         *      3 : u8, Title Variation (0 = current)
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *      2 : u8, 0 = doesn't exist, 1 = exists
         *    3-4 : u64, Secure Value
         */
        void GetSaveDataSecureValue(Kernel::HLERequestContext& ctx);

        /**
         * FS::ControlSecureSave service function
         *  Inputs:
         *      0 : Header code [0x086700C4]
         *      1 : Action
         *      2 : Input Size
         *      3 : Output Size
         *      4 : (inputSize << 4) | 0xA
         *      5 : void* Input
         *      6 : (outputSize << 4) | 0xC
         *      7 : void* Output
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void ControlSecureSave(Kernel::HLERequestContext& ctx);

        /**
         * FS::GetMediaType service function
         *  Inputs:
         *      0 : Header code [0x08680000]
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *      2 : Media Type
         */
        void GetMediaType(Kernel::HLERequestContext& ctx);

        /**
         * FS::LegacyGetNandEraseCount service function
         * This command is stubbed and returns error 0xE0C046F8.
         * It is unknown what its response values meant.
         *  Inputs:
         *      0 : Header code [0x08690000]
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *      2 : ?
         *      3 : ?
         *      4 : ?
         *      5 : ?
         */
        void LegacyGetNandEraseCount(Kernel::HLERequestContext& ctx);

        /**
         * FS::ReadNandReport service function
         *  Inputs:
         *      0 : Header code [0x086A0082]
         *      1 : Buffer Length
         *      2 : Unknown
         *      3 : (bufferLength << 4) | 0xC
         *      4 : Buffer Pointer
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void ReadNandReport(Kernel::HLERequestContext& ctx);

        /**
         * FS::AddSeed service function
         * Adds an entry to the seed DB for the specified title.
         *  Inputs:
         *      0 : Header code [0x087A0180]
         *    1-2 : u64, Title ID
         *    3-6 : Seed
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void AddSeed(Kernel::HLERequestContext& ctx);

        /**
         * FS::GetNumSeeds service function.
         *  Inputs:
         *      0 : Header code [0x087D0000]
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *      2 : Number of seeds in the SEEDDB
         */
        void GetNumSeeds(Kernel::HLERequestContext& ctx);

        /**
         * FS::CheckUpdatedDat service function
         *  Inputs:
         *      0 : Header code [0x088600C0]
         *      1 : u8, Media Type
         *    2-3 : u64, Title ID
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *      2 : u8, Output bool
         */
        void CheckUpdatedDat(Kernel::HLERequestContext& ctx);

        u32 priority = -1; ///< For SetPriority and GetPriority service functions

    private:
        std::shared_ptr<Module> fs;
    };

    std::unique_ptr<ArchiveManager> archive_manager;
    static std::shared_ptr<Module> current_fs;
};

/// Initialize the FS services
void InstallInterfaces(SM::ServiceManager& service_manager);

} // namespace FS
} // namespace Service