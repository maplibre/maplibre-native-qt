// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: MIT

#include <QDebug>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QtQuick/QSGRendererInterface>
#if QT_CONFIG(vulkan)
#include <QVulkanInstance>
#endif

int main(int argc, char *argv[]) {
    // Set up graphics API and instance for each platform
#if defined(MLN_WITH_VULKAN)

    // Let Qt handle Vulkan initialization automatically
    QQuickWindow::setGraphicsApi(QSGRendererInterface::Vulkan);

    // Enable Vulkan debug output
    qputenv("QT_VULKAN_DEBUG_OUTPUT", "1");

#elif defined(__APPLE__)
    // Use Metal on Apple platforms
    QQuickWindow::setGraphicsApi(QSGRendererInterface::Metal);

#else
    // Use OpenGL on other platforms
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

#endif

    QGuiApplication app(argc, argv);

    qDebug() << "Platform:" << QGuiApplication::platformName();

    QQmlApplicationEngine engine;

    // Try to load test file if provided as argument
    if (argc > 1) {
        engine.load(QUrl::fromLocalFile(argv[1]));
    } else {
        engine.load(QUrl(QStringLiteral("qrc:/minimal/main.qml")));
    }
    if (engine.rootObjects().isEmpty()) return -1;

    return app.exec();
}
