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
    styles.emplace_back("https://demotiles.maplibre.org/style.json", "Demo Tiles");

    QMapLibre::Settings settings;
    settings.setStyles(styles);
    settings.setDefaultZoom(5);
    settings.setDefaultCoordinate(QMapLibre::Coordinate(43, 21));

    m_mapWidget = new QMapLibre::MapWidget(settings);
    m_mapWidget->setParent(this);

    m_layout = std::make_unique<QVBoxLayout>(this);
    m_layout->setContentsMargins(0, 0, 0, 0); // Remove margins
    m_layout->setSpacing(0);                  // Remove spacing between widgets

    m_layout->addWidget(m_mapWidget, 1); // Give the map widget stretch factor

    m_buttonDock = std::make_unique<QPushButton>(tr("Undock"), this);
    m_buttonDock->setMinimumHeight(40); // Make button more visible
    m_buttonDock->setMaximumHeight(40); // Fixed height for button
    connect(m_buttonDock.get(), &QPushButton::clicked, this, &Window::dockUndock);
    m_layout->addWidget(m_buttonDock.get(), 0); // No stretch for button

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
