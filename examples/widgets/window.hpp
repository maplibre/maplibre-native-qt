// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: MIT

#ifndef WINDOW_H
#define WINDOW_H

#include <QMapLibreWidgets/GLWidget>

#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include <memory>

class MainWindow;

class Window : public QWidget {
    Q_OBJECT

public:
    explicit Window(MainWindow* mainWindow);

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void dockUndock();

private:
    std::unique_ptr<QMapLibre::GLWidget> m_glWidget{};
    std::unique_ptr<QVBoxLayout> m_layout{};
    std::unique_ptr<QPushButton> m_buttonDock{};
    MainWindow* m_mainWindowRef{};
};

#endif
