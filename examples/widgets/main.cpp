// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: MIT

#include "mainwindow.hpp"

#include <QApplication>

int main(int argc, char **argv) {
    QApplication app(argc, argv);

    MainWindow window;

    window.resize(800, 600);
    window.show();

    return app.exec();
}
