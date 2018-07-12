// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/file_sys/archive_backend.h"
#include "core/file_sys/errors.h"
#include "core/file_sys/file_backend.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/client_port.h"
#include "core/hle/kernel/client_session.h"
#include "core/hle/kernel/event.h"
#include "core/hle/kernel/server_session.h"
#include "core/hle/service/fs/file.h"

using Kernel::ClientSession;
using Kernel::ServerSession;
using Kernel::SharedPtr;

namespace FileSys {
class FileBackend;
} // namespace FileSys

namespace Service {
namespace FS {

File::File(std::unique_ptr<FileSys::FileBackend>&& backend, const FileSys::Path& path)
    : ServiceFramework("", 1), path(path), backend(std::move(backend)) {
    static const FunctionInfo functions[] = {
        // clang-format off
        {0x08010100, &File::OpenSubFile, "OpenSubFile"},
        {0x080200C2, &File::Read, "Read"},
        {0x08030102, &File::Write, "Write"},
        {0x08040000, &File::GetSize, "GetSize"},
        {0x08050080, &File::SetSize, "SetSize"},
        {0x08080000, &File::Close, "Close"},
        {0x08090000, &File::Flush, "Flush"},
        {0x080A0040, &File::SetPriority, "SetPriority"},
        {0x080B0000, &File::GetPriority, "GetPriority"},
        {0x080C0000, &File::OpenLinkFile, "OpenLinkFile"},
        // clang-format on
    };

    RegisterHandlers(functions);
}

void File::Read(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x0802, 3, 2);
    u64 offset = rp.Pop<u64>();
    u32 length = rp.Pop<u32>();
    auto& buffer = rp.PopMappedBuffer();
    const FileSessionSlot* file = GetSessionData(ctx.Session());

    if (file->subfile && length > file->size) {
        LOG_WARNING(Service_FS, "Trying to read beyond the subfile size, truncating");
        length = file->size;
    }

    // This file session might have a specific offset from where to start reading, apply it.
    offset += file->offset;

    if (offset + length > backend->GetSize()) {
        LOG_ERROR(Service_FS,
                  "Reading from out of bounds offset=0x{:x} length=0x{:08X} file_size=0x{:x}",
                  offset, length, backend->GetSize());
    }

    IPC::RequestBuilder rb = rp.MakeBuilder(2, 2);

    std::vector<u8> data(length);
    ResultVal<size_t> read = backend->Read(offset, data.size(), data.data());
    if (read.Failed()) {
        rb.Push(read.Code());
        rb.Push<u32>(0);
    } else {
        buffer.Write(data.data(), 0, *read);
        rb.Push(RESULT_SUCCESS);
        rb.Push<u32>(*read);
    }
    rb.PushMappedBuffer(buffer);

    LOG_TRACE(Service_FS, "Read {}: offset=0x{:x} length=0x{:08X}", GetName(), offset, length);

    std::chrono::nanoseconds read_timeout_ns{backend->GetReadDelayNs(length)};
    ctx.SleepClientThread(Kernel::GetCurrentThread(), "file::read", read_timeout_ns,
                          [](Kernel::SharedPtr<Kernel::Thread> thread,
                             Kernel::HLERequestContext& ctx, ThreadWakeupReason reason) {
                              // Nothing to do here
                          });
}

void File::Write(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x0803, 4, 2);
    u64 offset = rp.Pop<u64>();
    u32 length = rp.Pop<u32>();
    u32 flush = rp.Pop<u32>();
    auto& buffer = rp.PopMappedBuffer();
    const FileSessionSlot* file = GetSessionData(ctx.Session());

    // Subfiles can not be written to
    IPC::RequestBuilder rb = rp.MakeBuilder(2, 2);
    if (file->subfile) {
        rb.Push(FileSys::ERROR_UNSUPPORTED_OPEN_FLAGS);
        rb.Push<u32>(0);
        rb.PushMappedBuffer(buffer);
        return;
    }

    std::vector<u8> data(length);
    buffer.Read(data.data(), 0, data.size());
    const ResultVal<size_t> written = backend->Write(offset, data.size(), flush != 0, data.data());
    if (written.Failed()) {
        rb.Push(written.Code());
        rb.Push<u32>(0);
    } else {
        rb.Push(RESULT_SUCCESS);
        rb.Push<u32>(*written);
    }
    rb.PushMappedBuffer(buffer);

    LOG_TRACE(Service_FS, "Write {}: offset=0x{:x} length={}, flush=0x{:x}", GetName(), offset,
              length, flush);
}

void File::GetSize(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x0804, 0, 0);

    const FileSessionSlot* file = GetSessionData(ctx.Session());

    IPC::RequestBuilder rb = rp.MakeBuilder(3, 0);
    rb.Push(RESULT_SUCCESS);
    rb.Push<u64>(file->size);
}

void File::SetSize(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x0805, 2, 0);
    const u64 size = rp.Pop<u64>();

    FileSessionSlot* file = GetSessionData(ctx.Session());

    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
    // SetSize can not be called on subfiles.
    if (file->subfile) {
        rb.Push(FileSys::ERROR_UNSUPPORTED_OPEN_FLAGS);
        return;
    }

    file->size = size;
    backend->SetSize(size);
    rb.Push(RESULT_SUCCESS);
}

void File::Close(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x0808, 0, 0);

    // TODO(Subv): Only close the backend if this client is the only one left.
    if (connected_sessions.size() > 1)
        LOG_WARNING(Service_FS, "Closing File backend but {} clients still connected",
                    connected_sessions.size());

    backend->Close();
    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
    rb.Push(RESULT_SUCCESS);
}

void File::Flush(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x0809, 0, 0);

    const FileSessionSlot* file = GetSessionData(ctx.Session());

    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
    // Subfiles can not be flushed.
    if (file->subfile) {
        rb.Push(FileSys::ERROR_UNSUPPORTED_OPEN_FLAGS);
        return;
    }

    backend->Flush();
    rb.Push(RESULT_SUCCESS);
}

void File::SetPriority(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x080A, 1, 0);

    FileSessionSlot* file = GetSessionData(ctx.Session());
    file->priority = rp.Pop<u32>();

    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
    rb.Push(RESULT_SUCCESS);
}

void File::GetPriority(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x080B, 0, 0);

    const FileSessionSlot* file = GetSessionData(ctx.Session());

    IPC::RequestBuilder rb = rp.MakeBuilder(2, 0);
    rb.Push(RESULT_SUCCESS);
    rb.Push(file->priority);
}

void File::OpenLinkFile(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x080C, 0, 0);

    const auto sessions = ServerSession::CreateSessionPair(GetName());
    const auto server = std::get<SharedPtr<ServerSession>>(sessions);
    ClientConnected(server);

    FileSessionSlot* slot = GetSessionData(server);
    const FileSessionSlot* original_file = GetSessionData(ctx.Session());

    slot->priority = original_file->priority;
    slot->offset = 0;
    slot->size = backend->GetSize();
    slot->subfile = false;

    IPC::RequestBuilder rb = rp.MakeBuilder(1, 2);
    rb.Push(RESULT_SUCCESS);
    rb.PushMoveObjects(std::get<SharedPtr<ClientSession>>(sessions));

    LOG_WARNING(Service_FS, "(STUBBED) File command OpenLinkFile {}", GetName());
}

void File::OpenSubFile(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x0801, 4, 0);
    const s64 offset = rp.PopRaw<s64>();
    const s64 size = rp.PopRaw<s64>();
    const FileSessionSlot* original_file = GetSessionData(ctx.Session());

    IPC::RequestBuilder rb = rp.MakeBuilder(1, 2);
    if (original_file->subfile) {
        // OpenSubFile can not be called on a file which is already as subfile
        rb.Push(FileSys::ERROR_UNSUPPORTED_OPEN_FLAGS);
        return;
    }

    if (offset < 0 || size < 0) {
        rb.Push(FileSys::ERR_WRITE_BEYOND_END);
        return;
    }

    size_t end = offset + size;

    // TODO(Subv): Check for overflow and return ERR_WRITE_BEYOND_END

    if (end > original_file->size) {
        rb.Push(FileSys::ERR_WRITE_BEYOND_END);
        return;
    }

    const auto sessions = ServerSession::CreateSessionPair(GetName());
    const auto server = std::get<SharedPtr<ServerSession>>(sessions);
    ClientConnected(server);

    FileSessionSlot* slot = GetSessionData(server);
    slot->priority = original_file->priority;
    slot->offset = offset;
    slot->size = size;
    slot->subfile = true;

    rb.Push(RESULT_SUCCESS);
    rb.PushMoveObjects(std::get<SharedPtr<ClientSession>>(sessions));
}

Kernel::SharedPtr<Kernel::ClientSession> File::Connect() {
    const auto sessions = Kernel::ServerSession::CreateSessionPair(GetName());
    const auto server = std::get<Kernel::SharedPtr<Kernel::ServerSession>>(sessions);
    ClientConnected(server);

    FileSessionSlot* slot = GetSessionData(server);
    slot->priority = 0;
    slot->offset = 0;
    slot->size = backend->GetSize();
    slot->subfile = false;

    return std::get<Kernel::SharedPtr<Kernel::ClientSession>>(sessions);
}

} // namespace FS
} // namespace Service