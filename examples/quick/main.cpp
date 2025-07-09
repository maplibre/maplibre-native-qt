// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: MIT

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QtQuick/QSGRendererInterface>

int main(int argc, char *argv[]) {
    // Use platform-appropriate graphics API
#if defined(__APPLE__)
    QQuickWindow::setGraphicsApi(QSGRendererInterface::MetalRhi);
#else
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGLRhi);
#endif

    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/Example/main.qml")));
    if (engine.rootObjects().isEmpty()) return -1;

    return app.exec();
}
