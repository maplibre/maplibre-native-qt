// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: MIT

#include "mainwindow.hpp"

#include "window.hpp"

#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>

MainWindow::MainWindow() {
    auto *menuBar = new QMenuBar(this);
    QMenu *menuWindow = menuBar->addMenu(tr("&Window"));
    auto *actionAddNew = new QAction(menuWindow);
    actionAddNew->setText(tr("Add new"));
    menuWindow->addAction(actionAddNew);
    connect(actionAddNew, &QAction::triggered, this, &MainWindow::onAddNew);
    setMenuBar(menuBar);

    onAddNew();
}

void MainWindow::onAddNew() {
    if (!centralWidget())
        setCentralWidget(new Window(this));
    else
        QMessageBox::information(nullptr, tr("Cannot add new window"), tr("Already occupied. Undock first."));
}
