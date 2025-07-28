// Copyright (C) 2024 MapLibre contributors
// SPDX-License-Identifier: BSD-2-Clause

#include "qgeomap_vulkan.hpp"
#include "qgeomap_p.hpp"
#include "texture_node_vulkan.hpp"

#include <QtPositioning/QGeoCoordinate>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGRendererInterface>

namespace QMapLibre {

QGeoMapMapLibreVulkan::QGeoMapMapLibreVulkan(QGeoMappingManagerEngine *engine, QObject *parent)
    : QGeoMapMapLibre(engine, parent) {}

QGeoMapMapLibreVulkan::~QGeoMapMapLibreVulkan() = default;

QSGNode *QGeoMapMapLibreVulkan::updateSceneGraph(QSGNode *node, QQuickWindow *window) {
    Q_D(QGeoMapMapLibre);

    if (viewportSize().isEmpty()) {
        delete node;
        return nullptr;
    }

    Map *map{};
    if (node == nullptr) {
        // Vulkan doesn't need context checks like OpenGL
        auto mbglNode = std::make_unique<TextureNodeVulkan>(
            d->m_settings, viewportSize(), window->devicePixelRatio(), this);

        QObject::connect(mbglNode->map(), &Map::mapChanged, this, &QGeoMapMapLibreVulkan::onMapChanged);
        d->m_syncState = QGeoMapMapLibrePrivate::MapTypeSync | QGeoMapMapLibrePrivate::CameraDataSync |
                         QGeoMapMapLibrePrivate::ViewportSync | QGeoMapMapLibrePrivate::VisibleAreaSync;
        node = mbglNode.release();
    }
    map = static_cast<TextureNodeVulkan *>(node)->map();

    if ((d->m_syncState & QGeoMapMapLibrePrivate::MapTypeSync) != 0 &&
        activeMapType().metadata().contains(QStringLiteral("url"))) {
        QString styleUrl = activeMapType().metadata()[QStringLiteral("url")].toString();
        qDebug() << "QGeoMapMapLibreVulkan: Setting style URL:" << styleUrl;
        map->setStyleUrl(styleUrl);
        // Force a render after setting style URL to trigger tile loading
        try {
            qDebug() << "QGeoMapMapLibreVulkan: Forcing render after style URL set";
            static_cast<TextureNodeVulkan *>(node)->render(window);
        } catch (const std::exception &e) {
            qWarning() << "QGeoMapMapLibreVulkan: Exception during forced render:" << e.what();
        }
    } else {
        qDebug() << "QGeoMapMapLibreVulkan: No style URL available. m_syncState:" << d->m_syncState
                 << "has url:" << activeMapType().metadata().contains(QStringLiteral("url"))
                 << "metadata:" << activeMapType().metadata();
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
        double zoom = zoomLevelFrom256(cameraData.zoomLevel(), mapLibreTileSize);
        map->setZoom(zoom);
        map->setBearing(cameraData.bearing());
        map->setPitch(cameraData.tilt());

        const QGeoCoordinate coordinate = cameraData.center();
        map->setCoordinate(Coordinate(coordinate.latitude(), coordinate.longitude()));
        qDebug() << "QGeoMapMapLibreVulkan: Setting camera - zoom:" << zoom << "center:" << coordinate.latitude() << ","
                 << coordinate.longitude();
    }

    if ((d->m_syncState & QGeoMapMapLibrePrivate::ViewportSync) != 0) {
        static_cast<TextureNodeVulkan *>(node)->resize(viewportSize(), window->devicePixelRatio(), window);
    }

    if (d->m_styleLoaded) {
        d->syncStyleChanges(map);
    }

    static_cast<TextureNodeVulkan *>(node)->render(window);

    // No threaded rendering hack needed for Vulkan

    d->m_syncState = QGeoMapMapLibrePrivate::NoSync;

    return node;
}

} // namespace QMapLibre
