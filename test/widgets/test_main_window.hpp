// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include "test_window.hpp"

#include <QtWidgets/QMainWindow>

namespace QMapLibre::Test {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow();

    void addNewWindow();
    [[nodiscard]] Window *currentCentralWidget() const { return static_cast<Window *>(centralWidget()); }
};

} // namespace QMapLibre::Test
