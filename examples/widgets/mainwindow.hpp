// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: MIT

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow();

private slots:
    void onAddNew();
};

#endif
