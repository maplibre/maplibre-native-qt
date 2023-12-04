// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: MIT

#include "window.hpp"

#include "mainwindow.hpp"

#include <QMapLibreWidgets/GLWidget>

#include <QApplication>
#include <QKeyEvent>
#include <QMessageBox>
#include <QVBoxLayout>

Window::Window(MainWindow *mainWindow)
    : QWidget(mainWindow),
      m_mainWindow(mainWindow) {
    QMapLibre::Settings settings;
    settings.setProviderTemplate(QMapLibre::Settings::MapLibreProvider);
    m_glWidget = new QMapLibre::GLWidget(settings);

    auto *layout = new QVBoxLayout;
    layout->addWidget(m_glWidget);
    m_buttonDock = new QPushButton(tr("Undock"), this);
    connect(m_buttonDock, &QPushButton::clicked, this, &Window::dockUndock);
    layout->addWidget(m_buttonDock);

    setLayout(layout);

    setWindowTitle(tr("Hello QMapLibre"));
}

void Window::keyPressEvent(QKeyEvent *e) {
    if (e->key() == Qt::Key_Escape)
        close();
    else
        QWidget::keyPressEvent(e);
}

void Window::dockUndock() {
    if (parent()) {
        setParent(nullptr);
        setAttribute(Qt::WA_DeleteOnClose);
        move(QGuiApplication::primaryScreen()->size().width() / 2 - width() / 2,
             QGuiApplication::primaryScreen()->size().height() / 2 - height() / 2);
        m_buttonDock->setText(tr("Dock"));
        show();
    } else {
        if (!m_mainWindow->centralWidget()) {
            if (m_mainWindow->isVisible()) {
                setAttribute(Qt::WA_DeleteOnClose, false);
                m_buttonDock->setText(tr("Undock"));
                m_mainWindow->setCentralWidget(this);
            } else {
                QMessageBox::information(nullptr, tr("Cannot dock"), tr("Main window already closed"));
            }
        } else {
            QMessageBox::information(nullptr, tr("Cannot dock"), tr("Main window already occupied"));
        }
    }
}
