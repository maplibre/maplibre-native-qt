// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#include "test_main_window.hpp"

#include "test_window.hpp"

#include <QtWidgets/QMenu>

namespace {
constexpr int MinimumWidth = 640;
constexpr int MinimumHeight = 480;
constexpr int DefaultPosition = 100;
} // namespace

namespace QMapLibre::Test {

MainWindow::MainWindow() {
    setMinimumSize(MinimumWidth, MinimumHeight);

    addNewWindow();

    move(DefaultPosition, DefaultPosition);
}

void MainWindow::addNewWindow() {
    if (centralWidget() != nullptr) {
        return;
    }

    auto window = std::make_unique<Window>(this);
    setCentralWidget(window.release()); // takes ownership
}

} // namespace QMapLibre::Test
