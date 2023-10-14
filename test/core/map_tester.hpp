// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include <QMapLibre/Map>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QOpenGLWidget>
#else
#include <QGLWidget>
#endif

class TestCore;

namespace QMapLibre {

class MapTester : public QObject {
    Q_OBJECT

public:
    MapTester();

    void runUntil(Map::MapChange);

private:
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QOpenGLWidget widget;
#else
    QGLWidget widget;
#endif
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
