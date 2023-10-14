// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: MIT

#ifndef WINDOW_H
#define WINDOW_H

#include <QPushButton>
#include <QWidget>

namespace QMapLibre {
class GLWidget;
}
class MainWindow;

class Window : public QWidget {
    Q_OBJECT

public:
    explicit Window(MainWindow *mainWindow);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void dockUndock();

private:
    QMapLibre::GLWidget *m_glWidget;
    QPushButton *m_buttonDock;
    MainWindow *m_mainWindow;
};

#endif
