// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: MIT

#include <QtQuick/qquickitem.h>
#include <QDebug>
#include <QGuiApplication>
#include <QLoggingCategory>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QtQuick/QSGRendererInterface>
#if QT_CONFIG(vulkan)
#include <QVulkanInstance>
#endif

#include <QMapLibre/Utils>

int main(int argc, char *argv[]) {
    // Enable verbose logging for debugging
    QLoggingCategory::setFilterRules(
        "qt.location.*.debug=true\n"
        "qt.positioning.*.debug=true\n"
        "maplibre.*.debug=true");
    // Set up graphics API and instance for each platform
    const QMapLibre::RendererType rendererType = QMapLibre::supportedRendererType();
    auto graphicsApi = static_cast<QSGRendererInterface::GraphicsApi>(rendererType);
    QQuickWindow::setGraphicsApi(graphicsApi);

    if (rendererType == QMapLibre::Vulkan) {
        // Enable Vulkan debug output
        qputenv("QT_VULKAN_DEBUG_OUTPUT", "1");
    }

    const QGuiApplication app(argc, argv);

    qDebug() << "Platform:" << QGuiApplication::platformName();
    qDebug() << "Graphics API:" << QQuickWindow::graphicsApi();
    qDebug() << "Renderer Type:" << rendererType;

    // Check if location services are available
    QQmlApplicationEngine engine;

    qDebug() << "Starting QML engine...";

    // Try to load test file if provided as argument
    if (argc > 1) {
        qDebug() << "Loading file:" << argv[1];
        engine.load(QUrl::fromLocalFile(argv[1]));
    } else {
        qDebug() << "Loading QML from resources...";
        engine.load(QUrl(QStringLiteral("qrc:/Example/main.qml")));
    }

    qDebug() << "Checking root objects...";
    if (engine.rootObjects().isEmpty()) {
        qDebug() << "ERROR: No root objects found!";

        // Check for errors
        const auto errors = engine.outputWarningsToStandardError();
        qDebug() << "QML Loading failed. Check above for errors.";
        return -1;
    }
    qDebug() << "Root objects loaded successfully";

    return app.exec();
}
