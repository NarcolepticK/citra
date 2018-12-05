// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <QAbstractListModel>
#include <QDockWidget>
#include "video_core/debugger/debugger.h"

class QPushButton;
class QTreeView;

class GPUCommandListModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum {
        CommandIdRole = Qt::UserRole,
    };

    explicit GPUCommandListModel(QObject* parent);

    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

public slots:
    void OnPicaTraceFinished(const Pica::PicaTrace& trace);

private:
    Pica::PicaTrace pica_trace;
};

class GPUCommandListWidget : public QDockWidget {
    Q_OBJECT

public:
    explicit GPUCommandListWidget(std::shared_ptr<Pica::PicaTracer> pica_tracer,
                                  QWidget* parent = nullptr);

public slots:
    void OnToggleTracing();
    void OnCommandDoubleClicked(const QModelIndex&);

    void SetCommandInfo(const QModelIndex&);

    void CopyAllToClipboard();

signals:
    void TracingFinished(const Pica::PicaTrace&);

private:
    std::shared_ptr<Pica::PicaTracer> pica_tracer;
    std::unique_ptr<Pica::PicaTrace> pica_trace;

    QTreeView* list_widget;
    QWidget* command_info_widget;
    QPushButton* toggle_tracing;
};
