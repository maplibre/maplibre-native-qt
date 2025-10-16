// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include <QMapLibre/Settings>
#include <QMapLibre/Types>
#include "export_widgets.hpp"

#include <QRhiWidget>
#include <memory>

QT_BEGIN_NAMESPACE
class QKeyEvent;
class QMouseEvent;
class QWheelEvent;
QT_END_NAMESPACE

namespace QMapLibre {

class Map;
class RhiWidgetPrivate;

class Q_MAPLIBRE_WIDGETS_EXPORT RhiWidget : public QRhiWidget {
    Q_OBJECT

public:
    explicit RhiWidget(const Settings &settings);
    ~RhiWidget() override;

    [[nodiscard]] Map *map();

signals:
    void mapChanged(Map *map);

protected:
    // QRhiWidget implementation
    void initialize(QRhiCommandBuffer *cb) override;
    void render(QRhiCommandBuffer *cb) override;
    void releaseResources() override;

    // Event handlers
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    bool event(QEvent *e) override;

    [[nodiscard]] Map *mapInstance();

    std::unique_ptr<RhiWidgetPrivate> d_ptr;

private:
    Q_DISABLE_COPY(RhiWidget);
};

} // namespace QMapLibre
