// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <algorithm>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <vector>
#include "core/hle/service/gsp/gsp.h"

namespace Debugger {

class GraphicsDebugger {
public:
    // Base class for all objects which need to be notified about GPU events
    class DebuggerObserver {
    public:
        DebuggerObserver(std::shared_ptr<GraphicsDebugger> debugger) : debugger_weak(debugger) {}

        virtual ~DebuggerObserver() {
            if (auto debugger = debugger_weak.lock())
                debugger->UnregisterObserver(this);
        }

        /**
         * Called when a GX command has been processed and is ready for being
         * read via GraphicsDebugger::ReadGXCommandHistory.
         * @param total_command_count Total number of commands in the GX history
         * @note All methods in this class are called from the GSP thread
         */
        virtual void GXCommandProcessed(int total_command_count) {
            if (auto debugger = debugger_weak.lock()) {
                const Service::GSP::Command& cmd =
                    debugger->ReadGXCommandHistory(total_command_count - 1);
                LOG_TRACE(Debug_GPU, "Received command: id={:x}", static_cast<int>(cmd.id.Value()));
            }
        }

    protected:
        std::weak_ptr<GraphicsDebugger> debugger_weak;
    };

    void GXCommandProcessed(u8* command_data) {
        std::lock_guard<std::mutex> lock(debugger_mutex);
        if (observers.empty())
            return;

        gx_command_history.emplace_back();
        Service::GSP::Command& cmd = gx_command_history.back();

        memcpy(&cmd, command_data, sizeof(Service::GSP::Command));

        ForEachObserver([this](DebuggerObserver* observer) {
            observer->GXCommandProcessed(static_cast<int>(this->gx_command_history.size()));
        });
    }

    Service::GSP::Command& ReadGXCommandHistory(int index) {
        std::lock_guard<std::mutex> lock(debugger_mutex);
        return gx_command_history[index];
    }

    void RegisterObserver(DebuggerObserver* observer) {
        // TODO: Check for duplicates
        std::lock_guard<std::mutex> lock(debugger_mutex);
        observers.push_back(observer);
    }

    void UnregisterObserver(DebuggerObserver* observer) {
        std::lock_guard<std::mutex> lock(debugger_mutex);
        observers.erase(std::remove(observers.begin(), observers.end(), observer), observers.end());
    }

private:
    void ForEachObserver(std::function<void(DebuggerObserver*)> func) {
        std::for_each(observers.begin(), observers.end(), func);
    }

    std::mutex debugger_mutex;
    std::vector<DebuggerObserver*> observers;
    std::vector<Service::GSP::Command> gx_command_history;
};
} // namespace Debugger