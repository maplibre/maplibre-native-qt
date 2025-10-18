// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include <QMapLibreWidgets/Export>

#include <QMapLibre/Map>
#include <QMapLibre/Settings>
#include <QMapLibre/Types>

#include <QtWidgets/QRhiWidget>

#include <memory>

QT_BEGIN_NAMESPACE
class QKeyEvent;
class QMouseEvent;
class QWheelEvent;
QT_END_NAMESPACE

namespace QMapLibre {

class MapWidgetPrivate;

class Q_MAPLIBRE_WIDGETS_EXPORT MapWidget : public QRhiWidget {
    Q_OBJECT

public:
    explicit MapWidget(const Settings &settings);
    ~MapWidget() override;

    [[nodiscard]] Map *map();

signals:
    void onMouseDoubleClickEvent(QMapLibre::Coordinate coordinate);
    void onMouseMoveEvent(QMapLibre::Coordinate coordinate);
    void onMousePressEvent(QMapLibre::Coordinate coordinate);
    void onMouseReleaseEvent(QMapLibre::Coordinate coordinate);

public slots:
    void handleMapChange(QMapLibre::Map::MapChange change);

protected:
    // Event handlers
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    bool event(QEvent *e) override;

    // QRhiWidget implementation
    void initialize(QRhiCommandBuffer *cb) override;
    void render(QRhiCommandBuffer *cb) override;
    void releaseResources() override;

private:
    Q_DISABLE_COPY(MapWidget);

    std::unique_ptr<MapWidgetPrivate> d_ptr;
};

} // namespace QMapLibre
