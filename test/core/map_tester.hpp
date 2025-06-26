// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include <QMapLibre/Map>

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
