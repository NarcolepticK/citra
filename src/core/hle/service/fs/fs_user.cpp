// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/service/fs/fs_user.h"

namespace Service {
namespace FS {

FS_USER::FS_USER(std::shared_ptr<Module> fs) : Module::Interface(std::move(fs), "fs:USER", 32) {
    static const FunctionInfo functions[] = {
        // clang-format off
        // fs: common commands
        {0x000100C6, nullptr, "Dummy1"},
        {0x040100C4, nullptr, "Control"},
        // fs:USER & fs:LDR shared commands
        {0x08010002, &FS_USER::Initialize, "Initialize"},
        {0x080201C2, &FS_USER::OpenFile, "OpenFile"},
        {0x08030204, &FS_USER::OpenFileDirectly, "OpenFileDirectly"},
        {0x08040142, &FS_USER::DeleteFile, "DeleteFile"},
        {0x08050244, &FS_USER::RenameFile, "RenameFile"},
        {0x08060142, &FS_USER::DeleteDirectory, "DeleteDirectory"},
        {0x08070142, &FS_USER::DeleteDirectoryRecursively, "DeleteDirectoryRecursively"},
        {0x08080202, &FS_USER::CreateFile, "CreateFile"},
        {0x08090182, &FS_USER::CreateDirectory, "CreateDirectory"},
        {0x080A0244, &FS_USER::RenameDirectory, "RenameDirectory"},
        {0x080B0102, &FS_USER::OpenDirectory, "OpenDirectory"},
        {0x080C00C2, &FS_USER::OpenArchive, "OpenArchive"},
        {0x080D0144, nullptr, "ControlArchive"},
        {0x080E0080, &FS_USER::CloseArchive, "CloseArchive"},
        {0x080F0180, &FS_USER::LegacyFormatThisUserSaveData, "LegacyFormatThisUserSaveData"},
        {0x08100200, &FS_USER::LegacyCreateSystemSaveData, "LegacyCreateSystemSaveData"},
        {0x08110040, nullptr, "LegacyDeleteSystemSaveData"},
        {0x08120080, &FS_USER::GetFreeBytes, "GetFreeBytes"},
        {0x08130000, nullptr, "GetCardType"},
        {0x08140000, nullptr, "GetSdmcArchiveResource"},
        {0x08150000, nullptr, "GetNandArchiveResource"},
        {0x08160000, nullptr, "GetSdmcFatfsError"},
        {0x08170000, &FS_USER::IsSdmcDetected, "IsSdmcDetected"},
        {0x08180000, &FS_USER::IsSdmcWriteable, "IsSdmcWritable"},
        {0x08190042, nullptr, "GetSdmcCid"},
        {0x081A0042, nullptr, "GetNandCid"},
        {0x081B0000, nullptr, "GetSdmcSpeedInfo"},
        {0x081C0000, nullptr, "GetNandSpeedInfo"},
        {0x081D0042, nullptr, "GetSdmcLog"},
        {0x081E0042, nullptr, "GetNandLog"},
        {0x081F0000, nullptr, "ClearSdmcLog"},
        {0x08200000, nullptr, "ClearNandLog"},
        {0x08210000, &FS_USER::CardSlotIsInserted, "CardSlotIsInserted"},
        {0x08220000, nullptr, "CardSlotPowerOn"},
        {0x08230000, nullptr, "CardSlotPowerOff"},
        {0x08240000, nullptr, "CardSlotGetCardIFPowerStatus"},
        {0x08250040, nullptr, "CardNorDirectCommand"},
        {0x08260080, nullptr, "CardNorDirectCommandWithAddress"},
        {0x08270082, nullptr, "CardNorDirectRead"},
        {0x082800C2, nullptr, "CardNorDirectReadWithAddress"},
        {0x08290082, nullptr, "CardNorDirectWrite"},
        {0x082A00C2, nullptr, "CardNorDirectWriteWithAddress"},
        {0x082B00C2, nullptr, "CardNorDirectRead_4xIO"},
        {0x082C0082, nullptr, "CardNorDirectCpuWriteWithoutVerify"},
        {0x082D0040, nullptr, "CardNorDirectSectorEraseWithoutVerify"},
        {0x082E0040, nullptr, "GetProductInfo"},
        {0x082F0040, &FS_USER::GetProgramLaunchInfo, "GetProgramLaunchInfo"},
        {0x08300182, &FS_USER::ObsoletedCreateExtSaveData, "Obsoleted_3_0_CreateExtSaveData"},
        {0x08310180, nullptr, "LegacyCreateSharedExtSaveData"},
        {0x08320102, nullptr, "LegacyReadExtSaveDataIcon"},
        {0x08330082, nullptr, "LegacyEnumerateExtSaveData"},
        {0x08340082, nullptr, "LegacyEnumerateSharedExtSaveData"},
        {0x08350080, &FS_USER::ObsoletedDeleteExtSaveData, "Obsoleted_3_0_DeleteExtSaveData"},
        {0x08360080, nullptr, "LegacyDeleteSharedExtSaveData"},
        {0x08370040, nullptr, "SetCardSpiBaudRate"},
        {0x08380040, nullptr, "SetCardSpiBusMode"},
        {0x08390000, nullptr, "SendInitializeInfoTo9"},
        {0x083A0100, nullptr, "GetSpecialContentIndex"},
        {0x083B00C2, nullptr, "GetLegacyRomHeader"},
        {0x083C00C2, nullptr, "GetLegacyBannerData"},
        {0x083D0100, nullptr, "CheckAuthorityToAccessExtSaveData"},
        {0x083E00C2, nullptr, "QueryTotalQuotaSize"},
        {0x083F00C0, nullptr, "LegacyGetExtDataBlockSize"},
        {0x08400040, nullptr, "AbnegateAccessRight"},
        {0x08410000, nullptr, "DeleteSdmcRoot"},
        {0x08420040, nullptr, "DeleteAllExtSaveDataOnNand"},
        {0x08430000, nullptr, "InitializeCtrFileSystem"},
        {0x08440000, nullptr, "CreateSeed"},
        {0x084500C2, &FS_USER::GetFormatInfo, "GetFormatInfo"},
        {0x08460102, nullptr, "GetLegacyRomHeader2"},
        {0x08470180, nullptr, "LegacyFormatCtrCardUserSaveData"},
        {0x08480042, nullptr, "GetSdmcCtrRootPath"},
        {0x08490040, &FS_USER::GetArchiveResource, "GetArchiveResource"},
        {0x084A0002, nullptr, "ExportIntegrityVerificationSeed"},
        {0x084B0002, nullptr, "ImportIntegrityVerificationSeed"},
        {0x084C0242, &FS_USER::FormatSaveData, "FormatSaveData"},
        {0x084D0102, nullptr, "GetLegacySubBannerData"},
        {0x084E0342, nullptr, "UpdateSha256Context"},
        {0x084F0102, nullptr, "ReadSpecialFile"},
        {0x08500040, nullptr, "GetSpecialFileSize"},
        {0x08510242, &FS_USER::CreateExtSaveData, "CreateExtSaveData"},
        {0x08520100, &FS_USER::DeleteExtSaveData, "DeleteExtSaveData"},
        {0x08530142, nullptr, "ReadExtSaveDataIcon"},
        {0x085400C0, nullptr, "GetExtDataBlockSize"},
        {0x08550102, nullptr, "EnumerateExtSaveData"},
        {0x08560240, &FS_USER::CreateSystemSaveData, "CreateSystemSaveData"},
        {0x08570080, &FS_USER::DeleteSystemSaveData, "DeleteSystemSaveData"},
        {0x08580000, nullptr, "StartDeviceMoveAsSource"},
        {0x08590200, nullptr, "StartDeviceMoveAsDestination"},
        {0x085A00C0, nullptr, "SetArchivePriority"},
        {0x085B0080, nullptr, "GetArchivePriority"},
        {0x085C00C0, nullptr, "SetCtrCardLatencyParameter"},
        {0x085D01C0, nullptr, "SetFsCompatibilityInfo"},
        {0x085E0040, nullptr, "ResetCardCompatibilityParameter"},
        {0x085F0040, nullptr, "SwitchCleanupInvalidSaveData"},
        {0x08600042, nullptr, "EnumerateSystemSaveData"},
        {0x08610042, &FS_USER::InitializeWithSdkVersion, "InitializeWithSdkVersion"},
        {0x08620040, &FS_USER::SetPriority, "SetPriority"},
        {0x08630000, &FS_USER::GetPriority, "GetPriority"},
        {0x08640000, nullptr, "LegacyGetNandInfo"},
        {0x08650140, &FS_USER::SetSaveDataSecureValue, "SetSaveDataSecureValue"},
        {0x086600C0, &FS_USER::GetSaveDataSecureValue, "GetSaveDataSecureValue"},
        {0x086700C4, nullptr, "ControlSecureSave"},
        {0x08680000, nullptr, "GetMediaType"},
        {0x08690000, nullptr, "GetNandEraseCount"},
        {0x086A0082, nullptr, "ReadNandReport"},
        {0x087A0180, nullptr, "AddSeed"},
        {0x087D0000, &FS_USER::GetNumSeeds, "GetNumSeeds"},
        {0x088600C0, nullptr, "CheckUpdatedDat"},
        // clang-format on
    };

    RegisterHandlers(functions);
}

} // namespace FS
} // namespace Service
