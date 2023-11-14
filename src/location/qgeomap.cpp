// Copyright (C) 2023 MapLibre contributors
// Copyright (C) 2017 The Qt Company Ltd.
// Copyright (C) 2017 Mapbox, Inc.

// SPDX-License-Identifier: LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgeomap.hpp"
#include "qgeomap_p.hpp"
#include "stylechange_p.h"
#include "texture_node.hpp"

#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtGui/QOpenGLContext>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QtOpenGL/QOpenGLFramebufferObject>
#else
#include <QtGui/QOpenGLFramebufferObject>
#endif
#include <QtLocation/private/qdeclarativecirclemapitem_p.h>
#include <QtLocation/private/qdeclarativegeomapitembase_p.h>
#include <QtLocation/private/qdeclarativepolygonmapitem_p.h>
#include <QtLocation/private/qdeclarativepolylinemapitem_p.h>
#include <QtLocation/private/qdeclarativerectanglemapitem_p.h>
#include <QtLocation/private/qgeoprojection_p.h>
#include <QtQuick/private/qsgcontext_p.h> // for debugging the context name
#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGImageNode>

#include <QMapLibre/Types>

#include <cmath>

// FIXME: Expose from MapLibre Native constants
#define MBGL_TILE_SIZE 512.0

namespace {

static const double invLog2 = 1.0 / std::log(2.0);

static double zoomLevelFrom256(double zoomLevelFor256, double tileSize) {
    return std::log(std::pow(2.0, zoomLevelFor256) * 256.0 / tileSize) * invLog2;
}

} // namespace

namespace QMapLibre {

QGeoMapMapLibrePrivate::QGeoMapMapLibrePrivate(QGeoMappingManagerEngine *engine)
    : QGeoMapPrivate(engine, new QGeoProjectionWebMercator) {}

QGeoMapMapLibrePrivate::~QGeoMapMapLibrePrivate() {}

QSGNode *QGeoMapMapLibrePrivate::updateSceneGraph(QSGNode *node, QQuickWindow *window) {
    Q_Q(QGeoMapMapLibre);

    if (m_viewportSize.isEmpty()) {
        delete node;
        return nullptr;
    }

    Map *map{};
    if (!node) {
        QOpenGLContext *currentCtx = QOpenGLContext::currentContext();
        if (!currentCtx) {
            qWarning("QOpenGLContext is NULL!");
            qWarning() << "You are running on QSG backend " << QSGContext::backend();
            qWarning("The MapLibre plugin works with both Desktop and ES 2.0+ OpenGL versions.");
            qWarning("Verify that your Qt is built with OpenGL, and what kind of OpenGL.");
            qWarning(
                "To force using a specific OpenGL version, check QSurfaceFormat::setRenderableType and "
                "QSurfaceFormat::setDefaultFormat");

            return node;
        }

        auto *mbglNode = new TextureNode(m_settings, m_viewportSize, window->devicePixelRatio(), q);
        QObject::connect(mbglNode->map(), &Map::mapChanged, q, &QGeoMapMapLibre::onMapChanged);
        m_syncState = MapTypeSync | CameraDataSync | ViewportSync | VisibleAreaSync;
        node = mbglNode;
    }
    map = static_cast<TextureNode *>(node)->map();

    if ((m_syncState & MapTypeSync) && m_activeMapType.metadata().contains(QStringLiteral("url"))) {
        map->setStyleUrl(m_activeMapType.metadata()[QStringLiteral("url")].toString());
    }

    if (m_syncState & VisibleAreaSync) {
        if (m_visibleArea.isEmpty()) {
            map->setMargins(QMargins());
        } else {
            QMargins margins(m_visibleArea.x(),                                                     // left
                             m_visibleArea.y(),                                                     // top
                             m_viewportSize.width() - m_visibleArea.width() - m_visibleArea.x(),    // right
                             m_viewportSize.height() - m_visibleArea.height() - m_visibleArea.y()); // bottom
            map->setMargins(margins);
        }
    }

    if (m_syncState & CameraDataSync || m_syncState & VisibleAreaSync) {
        map->setZoom(zoomLevelFrom256(m_cameraData.zoomLevel(), MBGL_TILE_SIZE));
        map->setBearing(m_cameraData.bearing());
        map->setPitch(m_cameraData.tilt());

        QGeoCoordinate coordinate = m_cameraData.center();
        map->setCoordinate(Coordinate(coordinate.latitude(), coordinate.longitude()));
    }

    if (m_syncState & ViewportSync) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        static_cast<TextureNode *>(node)->resize(m_viewportSize, window->devicePixelRatio(), window);
#else
        static_cast<TextureNode *>(node)->resize(m_viewportSize, window->devicePixelRatio());
#endif
    }

    if (m_styleLoaded) {
        syncStyleChanges(map);
    }

    static_cast<TextureNode *>(node)->render(window);

    threadedRenderingHack(window, map);

    m_syncState = NoSync;

    return node;
}

QGeoMap::ItemTypes QGeoMapMapLibrePrivate::supportedMapItemTypes() const {
    return QGeoMap::MapRectangle | QGeoMap::MapCircle | QGeoMap::MapPolygon | QGeoMap::MapPolyline;
}

void QGeoMapMapLibrePrivate::addMapItem(QDeclarativeGeoMapItemBase *item) {
    Q_Q(QGeoMapMapLibre);

    switch (item->itemType()) {
        case QGeoMap::NoItem:
        case QGeoMap::MapQuickItem:
        case QGeoMap::CustomMapItem:
            return;
        case QGeoMap::MapRectangle: {
            QDeclarativeRectangleMapItem *mapItem = static_cast<QDeclarativeRectangleMapItem *>(item);
            QObject::connect(mapItem, &QQuickItem::visibleChanged, q, &QGeoMapMapLibre::onMapItemPropertyChanged);
            QObject::connect(mapItem,
                             &QDeclarativeGeoMapItemBase::mapItemOpacityChanged,
                             q,
                             &QGeoMapMapLibre::onMapItemPropertyChanged);
            QObject::connect(mapItem,
                             &QDeclarativeRectangleMapItem::bottomRightChanged,
                             q,
                             &QGeoMapMapLibre::onMapItemGeometryChanged);
            QObject::connect(
                mapItem, &QDeclarativeRectangleMapItem::topLeftChanged, q, &QGeoMapMapLibre::onMapItemGeometryChanged);
            QObject::connect(
                mapItem, &QDeclarativeRectangleMapItem::colorChanged, q, &QGeoMapMapLibre::onMapItemPropertyChanged);
            QObject::connect(mapItem->border(),
                             &QDeclarativeMapLineProperties::colorChanged,
                             q,
                             &QGeoMapMapLibre::onMapItemSubPropertyChanged);
            QObject::connect(mapItem->border(),
                             &QDeclarativeMapLineProperties::widthChanged,
                             q,
                             &QGeoMapMapLibre::onMapItemUnsupportedPropertyChanged);
        } break;
        case QGeoMap::MapCircle: {
            QDeclarativeCircleMapItem *mapItem = static_cast<QDeclarativeCircleMapItem *>(item);
            QObject::connect(mapItem, &QQuickItem::visibleChanged, q, &QGeoMapMapLibre::onMapItemPropertyChanged);
            QObject::connect(mapItem,
                             &QDeclarativeGeoMapItemBase::mapItemOpacityChanged,
                             q,
                             &QGeoMapMapLibre::onMapItemPropertyChanged);
            QObject::connect(
                mapItem, &QDeclarativeCircleMapItem::centerChanged, q, &QGeoMapMapLibre::onMapItemGeometryChanged);
            QObject::connect(
                mapItem, &QDeclarativeCircleMapItem::radiusChanged, q, &QGeoMapMapLibre::onMapItemGeometryChanged);
            QObject::connect(
                mapItem, &QDeclarativeCircleMapItem::colorChanged, q, &QGeoMapMapLibre::onMapItemPropertyChanged);
            QObject::connect(mapItem->border(),
                             &QDeclarativeMapLineProperties::colorChanged,
                             q,
                             &QGeoMapMapLibre::onMapItemSubPropertyChanged);
            QObject::connect(mapItem->border(),
                             &QDeclarativeMapLineProperties::widthChanged,
                             q,
                             &QGeoMapMapLibre::onMapItemUnsupportedPropertyChanged);
        } break;
        case QGeoMap::MapPolygon: {
            QDeclarativePolygonMapItem *mapItem = static_cast<QDeclarativePolygonMapItem *>(item);
            QObject::connect(mapItem, &QQuickItem::visibleChanged, q, &QGeoMapMapLibre::onMapItemPropertyChanged);
            QObject::connect(mapItem,
                             &QDeclarativeGeoMapItemBase::mapItemOpacityChanged,
                             q,
                             &QGeoMapMapLibre::onMapItemPropertyChanged);
            QObject::connect(
                mapItem, &QDeclarativePolygonMapItem::pathChanged, q, &QGeoMapMapLibre::onMapItemGeometryChanged);
            QObject::connect(
                mapItem, &QDeclarativePolygonMapItem::colorChanged, q, &QGeoMapMapLibre::onMapItemPropertyChanged);
            QObject::connect(mapItem->border(),
                             &QDeclarativeMapLineProperties::colorChanged,
                             q,
                             &QGeoMapMapLibre::onMapItemSubPropertyChanged);
            QObject::connect(mapItem->border(),
                             &QDeclarativeMapLineProperties::widthChanged,
                             q,
                             &QGeoMapMapLibre::onMapItemUnsupportedPropertyChanged);
        } break;
        case QGeoMap::MapPolyline: {
            QDeclarativePolylineMapItem *mapItem = static_cast<QDeclarativePolylineMapItem *>(item);
            QObject::connect(mapItem, &QQuickItem::visibleChanged, q, &QGeoMapMapLibre::onMapItemPropertyChanged);
            QObject::connect(mapItem,
                             &QDeclarativeGeoMapItemBase::mapItemOpacityChanged,
                             q,
                             &QGeoMapMapLibre::onMapItemPropertyChanged);
            QObject::connect(
                mapItem, &QDeclarativePolylineMapItem::pathChanged, q, &QGeoMapMapLibre::onMapItemGeometryChanged);
            QObject::connect(mapItem->line(),
                             &QDeclarativeMapLineProperties::colorChanged,
                             q,
                             &QGeoMapMapLibre::onMapItemSubPropertyChanged);
            QObject::connect(mapItem->line(),
                             &QDeclarativeMapLineProperties::widthChanged,
                             q,
                             &QGeoMapMapLibre::onMapItemSubPropertyChanged);
        } break;
    }

    QObject::connect(
        item, &QDeclarativeGeoMapItemBase::mapItemOpacityChanged, q, &QGeoMapMapLibre::onMapItemPropertyChanged);

    m_styleChanges << StyleChange::addMapItem(item, m_mapItemsBefore);

    emit q->sgNodeChanged();
}

void QGeoMapMapLibrePrivate::removeMapItem(QDeclarativeGeoMapItemBase *item) {
    Q_Q(QGeoMapMapLibre);

    switch (item->itemType()) {
        case QGeoMap::NoItem:
        case QGeoMap::MapQuickItem:
        case QGeoMap::CustomMapItem:
            return;
        case QGeoMap::MapRectangle:
            q->disconnect(static_cast<QDeclarativeRectangleMapItem *>(item)->border());
            break;
        case QGeoMap::MapCircle:
            q->disconnect(static_cast<QDeclarativeCircleMapItem *>(item)->border());
            break;
        case QGeoMap::MapPolygon:
            q->disconnect(static_cast<QDeclarativePolygonMapItem *>(item)->border());
            break;
        case QGeoMap::MapPolyline:
            q->disconnect(static_cast<QDeclarativePolylineMapItem *>(item)->line());
            break;
    }

    q->disconnect(item);

    m_styleChanges << StyleChange::removeMapItem(item);

    emit q->sgNodeChanged();
}

void QGeoMapMapLibrePrivate::changeViewportSize(const QSize &) {
    Q_Q(QGeoMapMapLibre);

    m_syncState = m_syncState | ViewportSync;
    emit q->sgNodeChanged();
}

void QGeoMapMapLibrePrivate::changeCameraData(const QGeoCameraData &) {
    Q_Q(QGeoMapMapLibre);

    m_syncState = m_syncState | CameraDataSync;
    emit q->sgNodeChanged();
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
void QGeoMapMapLibrePrivate::changeActiveMapType(const QGeoMapType &)
#else
void QGeoMapMapLibrePrivate::changeActiveMapType(const QGeoMapType)
#endif
{
    Q_Q(QGeoMapMapLibre);

    m_syncState = m_syncState | MapTypeSync;
    emit q->sgNodeChanged();
}

void QGeoMapMapLibrePrivate::setVisibleArea(const QRectF &visibleArea) {
    Q_Q(QGeoMapMapLibre);
    const QRectF va = clampVisibleArea(visibleArea);
    if (va == m_visibleArea) return;

    m_visibleArea = va;
    m_geoProjection->setVisibleArea(va);

    m_syncState = m_syncState | VisibleAreaSync;
    emit q->sgNodeChanged();
}

QRectF QGeoMapMapLibrePrivate::visibleArea() const {
    return m_visibleArea;
}

void QGeoMapMapLibrePrivate::syncStyleChanges(Map *map) {
    for (const auto &change : m_styleChanges) {
        change->apply(map);
    }

    m_styleChanges.clear();
}

void QGeoMapMapLibrePrivate::threadedRenderingHack(QQuickWindow *window, Map *map) {
    // FIXME: Optimal support for threaded rendering needs core changes
    // in MapLibre Native. Meanwhile we need to set a timer to update
    // the map until all the resources are loaded, which is not exactly
    // battery friendly, because might trigger more paints than we need.
    if (!m_warned) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        m_threadedRendering = static_cast<QOpenGLContext *>(window->rendererInterface()->getResource(
                                                                window, QSGRendererInterface::OpenGLContextResource))
                                  ->thread() != QCoreApplication::instance()->thread();
#else
        m_threadedRendering = window->openglContext()->thread() != QCoreApplication::instance()->thread();
#endif

        if (m_threadedRendering) {
            qWarning() << "Threaded rendering is not optimal in the MapLibre Native plugin.";
        }

        m_warned = true;
    }

    if (m_threadedRendering) {
        if (!map->isFullyLoaded()) {
            QMetaObject::invokeMethod(&m_refresh, "start", Qt::QueuedConnection);
        } else {
            QMetaObject::invokeMethod(&m_refresh, "stop", Qt::QueuedConnection);
        }
    }
}

/*
 * QGeoMapMapLibre implementation
 */

QGeoMapMapLibre::QGeoMapMapLibre(QGeoMappingManagerEngine *engine, QObject *parent)
    : QGeoMap(*new QGeoMapMapLibrePrivate(engine), parent) {
    Q_D(QGeoMapMapLibre);

    connect(&d->m_refresh, &QTimer::timeout, this, &QGeoMap::sgNodeChanged);
    d->m_refresh.setInterval(250);
}

QGeoMapMapLibre::~QGeoMapMapLibre() {}

void QGeoMapMapLibre::setSettings(const Settings &settings) {
    Q_D(QGeoMapMapLibre);

    d->m_settings = settings;
}

void QGeoMapMapLibre::setMapItemsBefore(const QString &before) {
    Q_D(QGeoMapMapLibre);
    d->m_mapItemsBefore = before;
}

QGeoMap::Capabilities QGeoMapMapLibre::capabilities() const {
    return Capabilities(SupportsVisibleRegion | SupportsSetBearing | SupportsAnchoringCoordinate | SupportsVisibleArea);
}

QSGNode *QGeoMapMapLibre::updateSceneGraph(QSGNode *oldNode, QQuickWindow *window) {
    Q_D(QGeoMapMapLibre);
    return d->updateSceneGraph(oldNode, window);
}

void QGeoMapMapLibre::onMapChanged(Map::MapChange change) {
    Q_D(QGeoMapMapLibre);

    if (change == Map::MapChangeDidFinishLoadingStyle || change == Map::MapChangeDidFailLoadingMap) {
        d->m_styleLoaded = true;
    } else if (change == Map::MapChangeWillStartLoadingMap) {
        d->m_styleLoaded = false;
        d->m_styleChanges.clear();

        for (QDeclarativeGeoMapItemBase *item : d->m_mapItems)
            d->m_styleChanges << StyleChange::addMapItem(item, d->m_mapItemsBefore);
    }
}

void QGeoMapMapLibre::onMapItemPropertyChanged() {
    Q_D(QGeoMapMapLibre);

    QDeclarativeGeoMapItemBase *item = static_cast<QDeclarativeGeoMapItemBase *>(sender());
    d->m_styleChanges << StyleSetPaintProperty::fromMapItem(item);
    d->m_styleChanges << StyleSetLayoutProperty::fromMapItem(item);

    emit sgNodeChanged();
}

void QGeoMapMapLibre::onMapItemSubPropertyChanged() {
    Q_D(QGeoMapMapLibre);

    QDeclarativeGeoMapItemBase *item = static_cast<QDeclarativeGeoMapItemBase *>(sender()->parent());
    d->m_styleChanges << StyleSetPaintProperty::fromMapItem(item);

    emit sgNodeChanged();
}

void QGeoMapMapLibre::onMapItemUnsupportedPropertyChanged() {
    // TODO https://bugreports.qt.io/browse/QTBUG-58872
    qWarning() << "Unsupported property for managed Map item";
}

void QGeoMapMapLibre::onMapItemGeometryChanged() {
    Q_D(QGeoMapMapLibre);

    QDeclarativeGeoMapItemBase *item = static_cast<QDeclarativeGeoMapItemBase *>(sender());
    d->m_styleChanges << StyleAddSource::fromMapItem(item);

    emit sgNodeChanged();
}

} // namespace QMapLibre
