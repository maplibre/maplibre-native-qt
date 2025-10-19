// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#include "map_window.hpp"

#include "main_window.hpp"

#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>

namespace QMapLibre::Test {

MapWindow::MapWindow(MainWindow *mainWindow)
    : QWidget(mainWindow),
      m_mainWindowRef(mainWindow) {
    Styles styles;
    styles.emplace_back("https://demotiles.maplibre.org/style.json", "Demo Tiles");

    Settings settings;
    settings.setStyles(styles);
    settings.setDefaultCoordinate(Coordinate(59.91, 10.75));
    settings.setDefaultZoom(4);

    m_mapWidget = std::make_unique<MapWidget>(settings).release();
    m_mapWidget->setParent(this);

    m_layout = std::make_unique<QVBoxLayout>(this);
    m_layout->setContentsMargins(0, 0, 0, 0); // Remove margins
    m_layout->setSpacing(0);                  // Remove spacing between widgets

    m_layout->addWidget(m_mapWidget, 1); // Give the map widget stretch factor

    setLayout(m_layout.get());
}

void MapWindow::dockUndock() {
    if (parent() != nullptr) {
        setParent(nullptr);
        move((QGuiApplication::primaryScreen()->size().width() / 2) - (width() / 2),
             (QGuiApplication::primaryScreen()->size().height() / 2) - (height() / 2));
        show();
    } else {
        if (m_mainWindowRef->centralWidget() == nullptr) {
            if (m_mainWindowRef->isVisible()) {
                m_mainWindowRef->setCentralWidget(this);
            }
        }
    }
}

} // namespace QMapLibre::Test
