// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <atomic>
#include <memory>
#include "core/core.h"
#include "core/frontend/emu_window.h"

class EmuWindow;
class RendererBase;

namespace Service {
namespace GSP {
class GSP_GPU;
} // namespace GSP
} // namespace Service

////////////////////////////////////////////////////////////////////////////////////////////////////
// Video Core namespace

namespace VideoCore {

extern std::unique_ptr<RendererBase> g_renderer; ///< Renderer plugin
extern std::weak_ptr<Service::GSP::GSP_GPU> gsp_gpu;

// TODO: Wrap these in a user settings struct along with any other graphics settings (often set from
// qt ui)
extern std::atomic<bool> g_hw_renderer_enabled;
extern std::atomic<bool> g_shader_jit_enabled;
extern std::atomic<bool> g_hw_shader_enabled;
extern std::atomic<bool> g_hw_shader_accurate_gs;
extern std::atomic<bool> g_hw_shader_accurate_mul;
extern std::atomic<bool> g_renderer_bg_color_update_requested;
// Screenshot
extern std::atomic<bool> g_renderer_screenshot_requested;
extern void* g_screenshot_bits;
extern std::function<void()> g_screenshot_complete_callback;
extern Layout::FramebufferLayout g_screenshot_framebuffer_layout;

/// Initialize the video core
Core::System::ResultStatus Init(Core::System& system, EmuWindow& emu_window);

/// Shutdown the video core
void Shutdown();

/// Request a screenshot of the next frame
void RequestScreenshot(void* data, std::function<void()> callback,
                       const Layout::FramebufferLayout& layout);

u16 GetResolutionScaleFactor();
/// Sets the gsp class that we trigger interrupts for
void SetServiceToInterrupt(std::weak_ptr<Service::GSP::GSP_GPU> gsp);

} // namespace VideoCore
