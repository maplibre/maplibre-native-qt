// Copyright (C) 2023 MapLibre contributors
// SPDX-License-Identifier: BSD-2-Clause

#include "qgeomap_opengl.hpp"
#include "qgeomap_p.hpp"
#include "texture_node_opengl.hpp"

#include <QtQuick/private/qsgcontext_p.h>
#include <QtGui/QOpenGLContext>
#include <QtPositioning/QGeoCoordinate>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGRendererInterface>

namespace QMapLibre {

QGeoMapMapLibreOpenGL::QGeoMapMapLibreOpenGL(QGeoMappingManagerEngine *engine, QObject *parent)
    : QGeoMapMapLibre(engine, parent) {}

QGeoMapMapLibreOpenGL::~QGeoMapMapLibreOpenGL() = default;

QSGNode *QGeoMapMapLibreOpenGL::updateSceneGraph(QSGNode *node, QQuickWindow *window) {
    Q_D(QGeoMapMapLibre);

    if (viewportSize().isEmpty()) {
        delete node;
        return nullptr;
    }

    Map *map{};
    if (node == nullptr) {
        // OpenGL requires a current context
        QOpenGLContext *currentCtx = QOpenGLContext::currentContext();
        if (currentCtx == nullptr) {
            qWarning("QOpenGLContext is NULL!");
            qWarning() << "You are running on QSG backend " << QSGContext::backend();
            qWarning("The MapLibre plugin works with both Desktop and ES 2.0+ OpenGL versions.");
            qWarning("Verify that your Qt is built with OpenGL, and what kind of OpenGL.");
            qWarning(
                "To force using a specific OpenGL version, check QSurfaceFormat::setRenderableType and "
                "QSurfaceFormat::setDefaultFormat");

            return node;
        }

        auto mbglNode = std::make_unique<TextureNodeOpenGL>(
            d->m_settings, viewportSize(), window->devicePixelRatio(), this);

        QObject::connect(mbglNode->map(), &Map::mapChanged, this, &QGeoMapMapLibreOpenGL::onMapChanged);
        d->m_syncState = QGeoMapMapLibrePrivate::MapTypeSync | QGeoMapMapLibrePrivate::CameraDataSync |
                         QGeoMapMapLibrePrivate::ViewportSync | QGeoMapMapLibrePrivate::VisibleAreaSync;
        node = mbglNode.release();
    }
    map = static_cast<TextureNodeOpenGL *>(node)->map();

    if ((d->m_syncState & QGeoMapMapLibrePrivate::MapTypeSync) != 0 &&
        activeMapType().metadata().contains(QStringLiteral("url"))) {
        map->setStyleUrl(activeMapType().metadata()[QStringLiteral("url")].toString());
    }

    if ((d->m_syncState & QGeoMapMapLibrePrivate::VisibleAreaSync) != 0) {
        QRectF visArea = visibleArea();
        if (visArea.isEmpty()) {
            map->setMargins(QMargins());
        } else {
            const QMargins margins(static_cast<int>(visArea.x()),
                                   static_cast<int>(visArea.y()),
                                   static_cast<int>(viewportSize().width() - visArea.width() - visArea.x()),
                                   static_cast<int>(viewportSize().height() - visArea.height() - visArea.y()));
            map->setMargins(margins);
        }
    }

    if ((d->m_syncState & QGeoMapMapLibrePrivate::CameraDataSync) != 0 ||
        (d->m_syncState & QGeoMapMapLibrePrivate::VisibleAreaSync) != 0) {
        constexpr double mapLibreTileSize = 512.0;
        const double invLog2 = 1.0 / std::log(2.0);
        auto zoomLevelFrom256 = [invLog2](double zoomLevelFor256, double tileSize) {
            return std::log(std::pow(2.0, zoomLevelFor256) * 256.0 / tileSize) * invLog2;
        };

        const QGeoCameraData cameraData = this->cameraData();
        map->setZoom(zoomLevelFrom256(cameraData.zoomLevel(), mapLibreTileSize));
        map->setBearing(cameraData.bearing());
        map->setPitch(cameraData.tilt());

        const QGeoCoordinate coordinate = cameraData.center();
        map->setCoordinate(Coordinate(coordinate.latitude(), coordinate.longitude()));
    }

    if ((d->m_syncState & QGeoMapMapLibrePrivate::ViewportSync) != 0) {
        static_cast<TextureNodeOpenGL *>(node)->resize(viewportSize(), window->devicePixelRatio(), window);
    }

    if (d->m_styleLoaded) {
        d->syncStyleChanges(map);
    }

    static_cast<TextureNodeOpenGL *>(node)->render(window);

    d->threadedRenderingHack(window, map);

    d->m_syncState = QGeoMapMapLibrePrivate::NoSync;

    return node;
}

} // namespace QMapLibre
