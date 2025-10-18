// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: MIT

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenuBar>

#include <memory>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow();

private slots:
    void onAddNew();

private:
    std::unique_ptr<QMenuBar> m_menuBar;
    std::unique_ptr<QAction> m_actionAddNew;
};

#endif
