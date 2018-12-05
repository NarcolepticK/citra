// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <array>
#include <iterator>
#include <memory>
#include <QBoxLayout>
#include <QComboBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QPushButton>
#include <boost/range/algorithm/copy.hpp>
#include "citra_qt/debugger/graphics/graphics_tracing.h"
#include "common/common_types.h"
#include "core/core.h"
#include "core/hw/hw.h"
#include "core/hw/gpu.h"
#include "core/hw/lcd.h"
#include "core/tracer/recorder.h"
#include "nihstro/float24.h"
#include "video_core/pica.h"
#include "video_core/pica/pica_state.h"
#include "video_core/video_core.h"

GraphicsTracingWidget::GraphicsTracingWidget(std::shared_ptr<Pica::DebugContext> debug_context,
                                             QWidget* parent)
    : BreakPointObserverDock(debug_context, tr("CiTrace Recorder"), parent) {

    setObjectName("CiTracing");

    QPushButton* start_recording = new QPushButton(tr("Start Recording"));
    QPushButton* stop_recording =
        new QPushButton(QIcon::fromTheme("document-save"), tr("Stop and Save"));
    QPushButton* abort_recording = new QPushButton(tr("Abort Recording"));

    connect(this, &GraphicsTracingWidget::SetStartTracingButtonEnabled, start_recording,
            &QPushButton::setVisible);
    connect(this, &GraphicsTracingWidget::SetStopTracingButtonEnabled, stop_recording,
            &QPushButton::setVisible);
    connect(this, &GraphicsTracingWidget::SetAbortTracingButtonEnabled, abort_recording,
            &QPushButton::setVisible);
    connect(start_recording, &QPushButton::clicked, this, &GraphicsTracingWidget::StartRecording);
    connect(stop_recording, &QPushButton::clicked, this, &GraphicsTracingWidget::StopRecording);
    connect(abort_recording, &QPushButton::clicked, this, &GraphicsTracingWidget::AbortRecording);

    stop_recording->setVisible(false);
    abort_recording->setVisible(false);

    auto main_widget = new QWidget;
    auto main_layout = new QVBoxLayout;
    {
        auto sub_layout = new QHBoxLayout;
        sub_layout->addWidget(start_recording);
        sub_layout->addWidget(stop_recording);
        sub_layout->addWidget(abort_recording);
        main_layout->addLayout(sub_layout);
    }
    main_widget->setLayout(main_layout);
    setWidget(main_widget);
}

void GraphicsTracingWidget::StartRecording() {
    auto context = context_weak.lock();
    if (!context)
        return;

    const auto& hw_manager = Core::System::GetInstance().HardwareManager();
    const auto& pica_state = Core::System::GetInstance().VideoCore().Pica().State();
    const auto shader_binary = pica_state.vs.program_code;
    const auto swizzle_data = pica_state.vs.swizzle_data;

    // Encode floating point numbers to 24-bit values
    // TODO: Drop this explicit conversion once we store float24 values bit-correctly internally.
    std::array<u32, 4 * 16> default_attributes;
    for (unsigned i = 0; i < 16; ++i) {
        for (unsigned comp = 0; comp < 3; ++comp) {
            default_attributes[4 * i + comp] = nihstro::to_float24(
                pica_state.input_default_attributes.attr[i][comp].ToFloat32());
        }
    }

    std::array<u32, 4 * 96> vs_float_uniforms;
    for (unsigned i = 0; i < 96; ++i)
        for (unsigned comp = 0; comp < 3; ++comp)
            vs_float_uniforms[4 * i + comp] =
                nihstro::to_float24(pica_state.vs.uniforms.f[i][comp].ToFloat32());

    CiTrace::Recorder::InitialState state;
    std::copy_n((u32*)&hw_manager.Gpu().Regs(), sizeof(HW::GPU::Regs) / sizeof(u32),
                std::back_inserter(state.gpu_registers));
    std::copy_n((u32*)&hw_manager.Lcd().Regs(), sizeof(HW::LCD::Regs) / sizeof(u32),
                std::back_inserter(state.lcd_registers));
    std::copy_n((u32*)&pica_state.regs, sizeof(pica_state.regs) / sizeof(u32),
                std::back_inserter(state.pica_registers));
    boost::copy(default_attributes, std::back_inserter(state.default_attributes));
    boost::copy(shader_binary, std::back_inserter(state.vs_program_binary));
    boost::copy(swizzle_data, std::back_inserter(state.vs_swizzle_data));
    boost::copy(vs_float_uniforms, std::back_inserter(state.vs_float_uniforms));
    // boost::copy(TODO: Not implemented, std::back_inserter(state.gs_program_binary));
    // boost::copy(TODO: Not implemented, std::back_inserter(state.gs_swizzle_data));
    // boost::copy(TODO: Not implemented, std::back_inserter(state.gs_float_uniforms));

    auto recorder = new CiTrace::Recorder(state);
    context->recorder = std::shared_ptr<CiTrace::Recorder>(recorder);

    emit SetStartTracingButtonEnabled(false);
    emit SetStopTracingButtonEnabled(true);
    emit SetAbortTracingButtonEnabled(true);
}

void GraphicsTracingWidget::StopRecording() {
    auto context = context_weak.lock();
    if (!context)
        return;

    QString filename = QFileDialog::getSaveFileName(this, tr("Save CiTrace"), "citrace.ctf",
                                                    tr("CiTrace File (*.ctf)"));

    if (filename.isEmpty()) {
        // If the user canceled the dialog, keep recording
        return;
    }

    context->recorder->Finish(filename.toStdString());
    context->recorder = nullptr;

    emit SetStopTracingButtonEnabled(false);
    emit SetAbortTracingButtonEnabled(false);
    emit SetStartTracingButtonEnabled(true);
}

void GraphicsTracingWidget::AbortRecording() {
    auto context = context_weak.lock();
    if (!context)
        return;

    context->recorder = nullptr;

    emit SetStopTracingButtonEnabled(false);
    emit SetAbortTracingButtonEnabled(false);
    emit SetStartTracingButtonEnabled(true);
}

void GraphicsTracingWidget::OnBreakPointHit(Pica::DebugContext::Event event, void* data) {
    widget()->setEnabled(true);
}

void GraphicsTracingWidget::OnResumed() {
    widget()->setEnabled(false);
}

void GraphicsTracingWidget::OnEmulationStarting(EmuThread* emu_thread) {
    // Disable tracing starting/stopping until a GPU breakpoint is reached
    widget()->setEnabled(false);
}

void GraphicsTracingWidget::OnEmulationStopping() {
    // TODO: Is it safe to access the context here?

    auto context = context_weak.lock();
    if (!context)
        return;

    if (context->recorder) {
        auto reply =
            QMessageBox::question(this, tr("CiTracing still active"),
                                  tr("A CiTrace is still being recorded. Do you want to save it? "
                                     "If not, all recorded data will be discarded."),
                                  QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

        if (reply == QMessageBox::Yes) {
            StopRecording();
        } else {
            AbortRecording();
        }
    }

    // If the widget was disabled before, enable it now to allow starting
    // tracing before starting the next emulation session
    widget()->setEnabled(true);
}
