// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <atomic>
#include <memory>
#include "core/core.h"
#include "core/frontend/emu_window.h"
#include "video_core/renderer_base.h"

class EmuWindow;

namespace Service {
namespace GSP {
class GSP_GPU;
enum class InterruptId : u8;
} // namespace GSP
} // namespace Service

namespace VideoCore {

struct VideoCoreSettings {
    std::atomic<bool> hw_renderer_enabled;
    std::atomic<bool> shader_jit_enabled;
    std::atomic<bool> hw_shader_enabled;
    std::atomic<bool> hw_shader_accurate_gs;
    std::atomic<bool> hw_shader_accurate_mul;
    std::atomic<bool> renderer_bg_color_update_requested;
    // Screenshot
    std::atomic<bool> renderer_screenshot_requested;
    void* screenshot_bits;
    std::function<void()> screenshot_complete_callback;
    Layout::FramebufferLayout screenshot_framebuffer_layout;
};

class VideoCore {
public:
    explicit VideoCore(Core::System& system);
    ~VideoCore();

    Core::System::ResultStatus Init(EmuWindow& emu_window);
    Core::System::ResultStatus Shutdown();

    void SignalInterrupt(Service::GSP::InterruptId interrupt_id);

    /// Request a screenshot of the next frame
    void RequestScreenshot(void* data, std::function<void()> callback,
                           const Layout::FramebufferLayout& layout);

    u16 GetResolutionScaleFactor();

    /// Gets a reference to the renderer
    RendererBase& Renderer();
    const RendererBase& Renderer() const;

    VideoCoreSettings& Settings();
    const VideoCoreSettings& Settings() const;

private:
    Core::System& system;
    VideoCoreSettings settings;
    std::unique_ptr<RendererBase> renderer;
};
} // namespace VideoCore
