// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: MIT

#include "mainwindow.hpp"

#include "window.hpp"

#include <QMenu>
#include <QMessageBox>

MainWindow::MainWindow() {
    m_menuBar = std::make_unique<QMenuBar>(this);
    QMenu* menuWindow = m_menuBar->addMenu(tr("&Window"));
    m_actionAddNew = std::make_unique<QAction>(menuWindow);
    m_actionAddNew->setText(tr("Add new"));
    menuWindow->addAction(m_actionAddNew.get());
    connect(m_actionAddNew.get(), &QAction::triggered, this, &MainWindow::onAddNew);
    setMenuBar(m_menuBar.get());

    onAddNew();
}

void MainWindow::onAddNew() {
    if (centralWidget() == nullptr) {
        auto window = std::make_unique<Window>(this);
        setCentralWidget(window.release()); // takes ownership
    } else {
        QMessageBox::information(nullptr, tr("Cannot add new window"), tr("Already occupied. Undock first."));
    }
}
