// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: MIT

#include <vulkan/vulkan.h>
#include <QDebug>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QVulkanInstance>
#include <QtQuick/QSGRendererInterface>

int main(int argc, char *argv[]) {
    // Set up graphics API and instance for each platform
#if defined(MLN_WITH_VULKAN)
    // Set Qt Quick to use Vulkan RHI
    QQuickWindow::setGraphicsApi(QSGRendererInterface::Vulkan);

    bool vulkanAvailable = true; // Assume available for now

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
        engine.load(QUrl(QStringLiteral("qrc:/Example/main.qml")));
    }
    if (engine.rootObjects().isEmpty()) return -1;

    return app.exec();
}
