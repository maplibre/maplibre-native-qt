// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: MIT

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QQuickWindow>
#endif

int main(int argc, char* argv[]) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGLRhi);
#endif

    const QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/Example/main.qml")));

    return app.exec();
}
