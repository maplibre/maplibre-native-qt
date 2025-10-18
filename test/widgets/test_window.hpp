// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include <QMapLibreWidgets/MapWidget>

#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

#include <memory>

namespace QMapLibre::Test {

class MainWindow;

class Window : public QWidget {
    Q_OBJECT

public:
    explicit Window(MainWindow *mainWindow);

public slots:
    void dockUndock();

private:
    std::unique_ptr<QMapLibre::MapWidget> m_mapWidget;
    std::unique_ptr<QVBoxLayout> m_layout;
    MainWindow *m_mainWindowRef{};
};

} // namespace QMapLibre::Test
