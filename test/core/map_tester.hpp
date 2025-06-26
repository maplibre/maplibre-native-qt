// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include <QMapLibre/Map>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#error "Qt versions older than 6 are no longer supported."
#endif

#include <QOpenGLWidget>

class TestCore;

namespace QMapLibre {

class MapTester : public QObject {
    Q_OBJECT

public:
    MapTester();

    void runUntil(Map::MapChange);

private:
    QOpenGLWidget widget;
    const QSize size;

protected:
    Settings settings;
    Map map;

    std::function<void(Map::MapChange)> changeCallback;

private slots:
    void onMapChanged(Map::MapChange);
    void onNeedsRendering();

    friend class ::TestCore;
};

} // namespace QMapLibre
