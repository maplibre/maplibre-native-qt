// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: MIT

#include "window.hpp"

#include "mainwindow.hpp"

#include <QApplication>
#include <QKeyEvent>
#include <QMessageBox>
#include <QVBoxLayout>

Window::Window(MainWindow *mainWindow)
    : QWidget(mainWindow),
      m_mainWindowRef(mainWindow) {
    QMapLibre::Styles styles;
    styles.emplace_back("https://tiles.openfreemap.org/styles/liberty", "Liberty Style");

    QMapLibre::Settings settings;
    settings.setStyles(styles);
    settings.setDefaultZoom(1);
    settings.setDefaultCoordinate(QMapLibre::Coordinate(10.0, 50.0));

    m_glWidget = std::make_unique<QMapLibre::GLWidget>(settings);

    m_layout = std::make_unique<QVBoxLayout>(this);
    m_layout->addWidget(m_glWidget.get());
    m_buttonDock = std::make_unique<QPushButton>(tr("Undock"), this);
    connect(m_buttonDock.get(), &QPushButton::clicked, this, &Window::dockUndock);
    m_layout->addWidget(m_buttonDock.get());

    setLayout(m_layout.get());

    setWindowTitle(tr("Hello QMapLibre"));
}

void Window::keyPressEvent(QKeyEvent *e) {
    if (e->key() == Qt::Key_Escape) {
        close();
    } else {
        QWidget::keyPressEvent(e);
    }
}

void Window::dockUndock() {
    if (parent() != nullptr) {
        setParent(nullptr);
        setAttribute(Qt::WA_DeleteOnClose);
        move(QGuiApplication::primaryScreen()->size().width() / 2 - width() / 2,
             QGuiApplication::primaryScreen()->size().height() / 2 - height() / 2);
        m_buttonDock->setText(tr("Dock"));
        show();
    } else {
        if (m_mainWindowRef->centralWidget() == nullptr) {
            if (m_mainWindowRef->isVisible()) {
                setAttribute(Qt::WA_DeleteOnClose, false);
                m_buttonDock->setText(tr("Undock"));
                m_mainWindowRef->setCentralWidget(this);
            } else {
                QMessageBox::information(nullptr, tr("Cannot dock"), tr("Main window already closed"));
            }
        } else {
            QMessageBox::information(nullptr, tr("Cannot dock"), tr("Main window already occupied"));
        }
    }
}
