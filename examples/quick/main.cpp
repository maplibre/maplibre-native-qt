// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: MIT

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QQuickWindow>
#endif
#include <QtCore/QObject>
#include <QtQuick/QSGRendererInterface>
#include <map.hpp>
#include <settings.hpp>
#include <QtCore/QVariant>
#include <platform/qt/src/utils/metal_renderer_backend.hpp>
#import <QuartzCore/CAMetalLayer.h>

#include <memory>

using namespace QMapLibre;

int main(int argc, char *argv[]) {
    // Use Metal RHI backend for macOS/iOS. This works because we removed the
    // Qt-Location MapView dependency that required an OpenGL context.
    QQuickWindow::setGraphicsApi(QSGRendererInterface::MetalRhi);

    const QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/Example/main.qml")));

    // Assume the root object is a Window
    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    auto *rootWindow = qobject_cast<QQuickWindow *>(engine.rootObjects().front());
    if (!rootWindow) {
        qCritical("Root object is not a QQuickWindow");
        return -1;
    }

    // Keep MapLibre objects alive for the lifetime of the window
    struct MapHolder : QObject {
        std::unique_ptr<Map> map;
        QSGRendererInterface* ri = nullptr;
    };

    auto holder = new MapHolder;
    holder->ri = rootWindow->rendererInterface();
    rootWindow->setProperty("_mapHolder", QVariant::fromValue(static_cast<QObject *>(holder)));

    // Initialize MapLibre once the render thread is active and the swap-chain
    // has a valid CAMetalLayer. We poll inside beforeRendering until the
    // layer becomes available.

    QObject::connect(rootWindow, &QQuickWindow::beforeRendering, rootWindow,
                     [rootWindow, holder]() mutable {
                         auto *ri = holder->ri; // cached from GUI thread

                         if (!holder->map) {
                             // Try to obtain the CAMetalLayer once.
                             void *layerPtr = ri->getResource(nullptr, "MetalLayer");
                             if (!layerPtr) {
                                 // Not ready yet – try again on next frame.
                                 return;
                             }

                             // Create MapLibre structures.
                             holder->map = std::make_unique<Map>(nullptr, Settings{},
                                                                  QSize(rootWindow->width(), rootWindow->height()),
                                                                  rootWindow->devicePixelRatio());
                             holder->map->createRenderer();
                             holder->map->setStyleUrl("https://demotiles.maplibre.org/style.json");
                             holder->map->setCoordinateZoom({40.7128, -74.0060}, 10);

                             // Inject the CAMetalLayer‐backed backend into the renderer
                             auto customBackend = std::make_unique<QMapLibre::MetalRendererBackend>((CA::MetalLayer*)layerPtr);
                             holder->map->renderer()->setBackend(std::move(customBackend));
                         }

                         // Render every frame.
                         holder->map->resize(QSize(rootWindow->width(), rootWindow->height()));
                         holder->map->render();
                     }, Qt::DirectConnection);

    return app.exec();
}
