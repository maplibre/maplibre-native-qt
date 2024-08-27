// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#include "map_tester.hpp"

#include <QOpenGLContext>
#include <QOpenGLFunctions>

#include <QtCore/QCoreApplication>

namespace QMapLibre {

MapTester::MapTester()
    : size(512, 512),
      settings(Settings::MapLibreProvider),
      map(nullptr, settings, size) {
    connect(&map, &Map::mapChanged, this, &MapTester::onMapChanged);
    connect(&map, &Map::needsRendering, this, &MapTester::onNeedsRendering);
    map.resize(size);
    map.setOpenGLFramebufferObject(widget.defaultFramebufferObject(), size);
    map.setCoordinateZoom(Coordinate(60.170448, 24.942046), 14);
    widget.show();
}

void MapTester::runUntil(Map::MapChange status) {
    changeCallback = [&](Map::MapChange change) {
        if (change == status) {
            qApp->exit();
            changeCallback = nullptr;
        }
    };

    qApp->exec();
}

void MapTester::onMapChanged(Map::MapChange change) {
    if (changeCallback) {
        changeCallback(change);
    }
}

void MapTester::onNeedsRendering() {
    widget.makeCurrent();
    QOpenGLContext::currentContext()->functions()->glViewport(0, 0, widget.width(), widget.height());
    map.render();
}

} // namespace QMapLibre
