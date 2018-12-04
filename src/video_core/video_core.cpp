// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <memory>
#include "common/logging/log.h"
#include "core/settings.h"
#include "core/hle/service/gsp/gsp_gpu.h"
#include "video_core/pica.h"
#include "video_core/renderer_base.h"
#include "video_core/renderer_opengl/renderer_opengl.h"
#include "video_core/gpu_debugger.h"
#include "video_core/video_core.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// Video Core namespace

namespace VideoCore {

VideoCore::VideoCore(Core::System& system) : system(system) {}
VideoCore::~VideoCore() {}

Core::System::ResultStatus VideoCore::Init(EmuWindow& emu_window) {
    pica = std::make_unique<Pica::Pica>();
    pica->Init();

    renderer = std::make_unique<OpenGL::RendererOpenGL>(system, emu_window);
    Core::System::ResultStatus result = renderer->Init();

    if (result != Core::System::ResultStatus::Success) {
        LOG_ERROR(Render, "initialization failed !");
    } else {
        LOG_DEBUG(Render, "initialized OK");
    }

    return result;
}

Core::System::ResultStatus VideoCore::Shutdown() {
    pica->Shutdown();

    renderer.reset();

    LOG_DEBUG(Render, "shutdown OK");
    return Core::System::ResultStatus::Success;
}

void VideoCore::SignalInterrupt(Service::GSP::InterruptId interrupt_id) {
    auto gsp = system.ServiceManager().GetService<Service::GSP::GSP_GPU>("gsp::Gpu");
    if (gsp)
        gsp->SignalInterrupt(interrupt_id);
};

void VideoCore::RequestScreenshot(void* data, std::function<void()> callback,
                                  const Layout::FramebufferLayout& layout) {
    if (settings.renderer_screenshot_requested) {
        LOG_ERROR(Render, "A screenshot is already requested or in progress, ignoring the request");
        return;
    }
    settings.screenshot_bits = data;
    settings.screenshot_complete_callback = std::move(callback);
    settings.screenshot_framebuffer_layout = layout;
    settings.renderer_screenshot_requested = true;
}

u16 VideoCore::GetResolutionScaleFactor() {
    if (settings.hw_renderer_enabled) {
        return !Settings::values.resolution_factor
                   ? renderer->GetRenderWindow().GetFramebufferLayout().GetScalingRatio()
                   : Settings::values.resolution_factor;
    } else {
        // Software renderer always render at native resolution
        return 1;
    }
}

void VideoCore::SetServiceToInterrupt(std::weak_ptr<Service::GSP::GSP_GPU> gsp) {
    gsp_gpu = std::move(gsp);
};

RendererBase& VideoCore::Renderer() {
    return *renderer;
};

const RendererBase& VideoCore::Renderer() const {
    return *renderer;
};

VideoCoreSettings& VideoCore::Settings() {
    return settings;
};

const VideoCoreSettings& VideoCore::Settings() const {
    return settings;
};

Pica::Pica& VideoCore::Pica() {
    return *pica;
};

const Pica::Pica& VideoCore::Pica() const {
    return *pica;
};

} // namespace VideoCore
