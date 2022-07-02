// Copyright (C) 2022 MapLibre contributors
// Copyright (C) 2017 The Qt Company Ltd.
// Copyright (C) 2017 Mapbox, Inc.

// SPDX-License-Identifier: LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgeomapmaplibregl.h"
#include "qgeomapmaplibregl_p.h"
#include "qsgmaplibreglnode.h"
#include "qmaplibreglstylechange_p.h"

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
#include <QtLocation/private/qgeomapparameter_p.h>
#include <QtLocation/private/qgeoprojection_p.h>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGImageNode>
#include <QtQuick/private/qsgtexture_p.h>
#include <QtQuick/private/qsgcontext_p.h> // for debugging the context name

#include <QMapboxGL>

#include <cmath>

// FIXME: Expose from Mapbox GL constants
#define MBGL_TILE_SIZE 512.0

namespace {

static const double invLog2 = 1.0 / std::log(2.0);

static double zoomLevelFrom256(double zoomLevelFor256, double tileSize)
{
    return std::log(std::pow(2.0, zoomLevelFor256) * 256.0 / tileSize) * invLog2;
}

} // namespace

QGeoMapMapLibreGLPrivate::QGeoMapMapLibreGLPrivate(QGeoMappingManagerEngineMapLibreGL *engine)
    : QGeoMapPrivate(engine, new QGeoProjectionWebMercator)
{
}

QGeoMapMapLibreGLPrivate::~QGeoMapMapLibreGLPrivate()
{
}

QSGNode *QGeoMapMapLibreGLPrivate::updateSceneGraph(QSGNode *node, QQuickWindow *window)
{
    Q_Q(QGeoMapMapLibreGL);

    if (m_viewportSize.isEmpty()) {
        delete node;
        return 0;
    }

    QMapboxGL *map = 0;
    if (!node) {
        QOpenGLContext *currentCtx = QOpenGLContext::currentContext();
        if (!currentCtx) {
            qWarning("QOpenGLContext is NULL!");
            qWarning() << "You are running on QSG backend " << QSGContext::backend();
            qWarning("The MapLibreGL plugin works with both Desktop and ES 2.0+ OpenGL versions.");
            qWarning("Verify that your Qt is built with OpenGL, and what kind of OpenGL.");
            qWarning("To force using a specific OpenGL version, check QSurfaceFormat::setRenderableType and QSurfaceFormat::setDefaultFormat");

            return node;
        }
        if (m_useFBO) {
            auto *mbglNode = new QSGMapLibreGLTextureNode(m_settings, m_viewportSize, window->devicePixelRatio(), q);
            QObject::connect(mbglNode->map(), &QMapboxGL::mapChanged, q, &QGeoMapMapLibreGL::onMapChanged);
            m_syncState = MapTypeSync | CameraDataSync | ViewportSync | VisibleAreaSync;
            node = mbglNode;
        } else {
            auto *mbglNode = new QSGMapLibreGLRenderNode(m_settings, m_viewportSize, window->devicePixelRatio(), q);
            QObject::connect(mbglNode->map(), &QMapboxGL::mapChanged, q, &QGeoMapMapLibreGL::onMapChanged);
            m_syncState = MapTypeSync | CameraDataSync | ViewportSync | VisibleAreaSync;
            node = mbglNode;
        }
    }
    map = (m_useFBO) ? static_cast<QSGMapLibreGLTextureNode *>(node)->map()
                     : static_cast<QSGMapLibreGLRenderNode *>(node)->map();

    if (m_syncState & MapTypeSync) {
        map->setStyleUrl(m_activeMapType.name());
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
        map->setZoom(zoomLevelFrom256(m_cameraData.zoomLevel() , MBGL_TILE_SIZE));
        map->setBearing(m_cameraData.bearing());
        map->setPitch(m_cameraData.tilt());

        QGeoCoordinate coordinate = m_cameraData.center();
        map->setCoordinate(QMapbox::Coordinate(coordinate.latitude(), coordinate.longitude()));
    }

    if (m_syncState & ViewportSync) {
        if (m_useFBO) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            static_cast<QSGMapLibreGLTextureNode *>(node)->resize(m_viewportSize, window->devicePixelRatio(), window);
#else
            static_cast<QSGMapLibreGLTextureNode *>(node)->resize(m_viewportSize, window->devicePixelRatio());
#endif
        } else {
            map->resize(m_viewportSize);
        }
    }

    if (m_styleLoaded) {
        syncStyleChanges(map);
    }

    if (m_useFBO) {
        static_cast<QSGMapLibreGLTextureNode *>(node)->render(window);
    }

    threadedRenderingHack(window, map);

    m_syncState = NoSync;

    return node;
}

void QGeoMapMapLibreGLPrivate::addParameter(QGeoMapParameter *param)
{
    Q_Q(QGeoMapMapLibreGL);

    QObject::connect(param, &QGeoMapParameter::propertyUpdated, q,
        &QGeoMapMapLibreGL::onParameterPropertyUpdated);

    if (m_styleLoaded) {
        m_styleChanges << QMapLibreGLStyleChange::addMapParameter(param);
        emit q->sgNodeChanged();
    }
}

void QGeoMapMapLibreGLPrivate::removeParameter(QGeoMapParameter *param)
{
    Q_Q(QGeoMapMapLibreGL);

    q->disconnect(param);

    if (m_styleLoaded) {
        m_styleChanges << QMapLibreGLStyleChange::removeMapParameter(param);
        emit q->sgNodeChanged();
    }
}

QGeoMap::ItemTypes QGeoMapMapLibreGLPrivate::supportedMapItemTypes() const
{
    return QGeoMap::MapRectangle | QGeoMap::MapCircle | QGeoMap::MapPolygon | QGeoMap::MapPolyline;
}

void QGeoMapMapLibreGLPrivate::addMapItem(QDeclarativeGeoMapItemBase *item)
{
    Q_Q(QGeoMapMapLibreGL);

    switch (item->itemType()) {
    case QGeoMap::NoItem:
    case QGeoMap::MapQuickItem:
    case QGeoMap::CustomMapItem:
        return;
    case QGeoMap::MapRectangle: {
        QDeclarativeRectangleMapItem *mapItem = static_cast<QDeclarativeRectangleMapItem *>(item);
        QObject::connect(mapItem, &QQuickItem::visibleChanged, q, &QGeoMapMapLibreGL::onMapItemPropertyChanged);
        QObject::connect(mapItem, &QDeclarativeGeoMapItemBase::mapItemOpacityChanged, q, &QGeoMapMapLibreGL::onMapItemPropertyChanged);
        QObject::connect(mapItem, &QDeclarativeRectangleMapItem::bottomRightChanged, q, &QGeoMapMapLibreGL::onMapItemGeometryChanged);
        QObject::connect(mapItem, &QDeclarativeRectangleMapItem::topLeftChanged, q, &QGeoMapMapLibreGL::onMapItemGeometryChanged);
        QObject::connect(mapItem, &QDeclarativeRectangleMapItem::colorChanged, q, &QGeoMapMapLibreGL::onMapItemPropertyChanged);
        QObject::connect(mapItem->border(), &QDeclarativeMapLineProperties::colorChanged, q, &QGeoMapMapLibreGL::onMapItemSubPropertyChanged);
        QObject::connect(mapItem->border(), &QDeclarativeMapLineProperties::widthChanged, q, &QGeoMapMapLibreGL::onMapItemUnsupportedPropertyChanged);
    } break;
    case QGeoMap::MapCircle: {
        QDeclarativeCircleMapItem *mapItem = static_cast<QDeclarativeCircleMapItem *>(item);
        QObject::connect(mapItem, &QQuickItem::visibleChanged, q, &QGeoMapMapLibreGL::onMapItemPropertyChanged);
        QObject::connect(mapItem, &QDeclarativeGeoMapItemBase::mapItemOpacityChanged, q, &QGeoMapMapLibreGL::onMapItemPropertyChanged);
        QObject::connect(mapItem, &QDeclarativeCircleMapItem::centerChanged, q, &QGeoMapMapLibreGL::onMapItemGeometryChanged);
        QObject::connect(mapItem, &QDeclarativeCircleMapItem::radiusChanged, q, &QGeoMapMapLibreGL::onMapItemGeometryChanged);
        QObject::connect(mapItem, &QDeclarativeCircleMapItem::colorChanged, q, &QGeoMapMapLibreGL::onMapItemPropertyChanged);
        QObject::connect(mapItem->border(), &QDeclarativeMapLineProperties::colorChanged, q, &QGeoMapMapLibreGL::onMapItemSubPropertyChanged);
        QObject::connect(mapItem->border(), &QDeclarativeMapLineProperties::widthChanged, q, &QGeoMapMapLibreGL::onMapItemUnsupportedPropertyChanged);
    } break;
    case QGeoMap::MapPolygon: {
        QDeclarativePolygonMapItem *mapItem = static_cast<QDeclarativePolygonMapItem *>(item);
        QObject::connect(mapItem, &QQuickItem::visibleChanged, q, &QGeoMapMapLibreGL::onMapItemPropertyChanged);
        QObject::connect(mapItem, &QDeclarativeGeoMapItemBase::mapItemOpacityChanged, q, &QGeoMapMapLibreGL::onMapItemPropertyChanged);
        QObject::connect(mapItem, &QDeclarativePolygonMapItem::pathChanged, q, &QGeoMapMapLibreGL::onMapItemGeometryChanged);
        QObject::connect(mapItem, &QDeclarativePolygonMapItem::colorChanged, q, &QGeoMapMapLibreGL::onMapItemPropertyChanged);
        QObject::connect(mapItem->border(), &QDeclarativeMapLineProperties::colorChanged, q, &QGeoMapMapLibreGL::onMapItemSubPropertyChanged);
        QObject::connect(mapItem->border(), &QDeclarativeMapLineProperties::widthChanged, q, &QGeoMapMapLibreGL::onMapItemUnsupportedPropertyChanged);
    } break;
    case QGeoMap::MapPolyline: {
        QDeclarativePolylineMapItem *mapItem = static_cast<QDeclarativePolylineMapItem *>(item);
        QObject::connect(mapItem, &QQuickItem::visibleChanged, q, &QGeoMapMapLibreGL::onMapItemPropertyChanged);
        QObject::connect(mapItem, &QDeclarativeGeoMapItemBase::mapItemOpacityChanged, q, &QGeoMapMapLibreGL::onMapItemPropertyChanged);
        QObject::connect(mapItem, &QDeclarativePolylineMapItem::pathChanged, q, &QGeoMapMapLibreGL::onMapItemGeometryChanged);
        QObject::connect(mapItem->line(), &QDeclarativeMapLineProperties::colorChanged, q, &QGeoMapMapLibreGL::onMapItemSubPropertyChanged);
        QObject::connect(mapItem->line(), &QDeclarativeMapLineProperties::widthChanged, q, &QGeoMapMapLibreGL::onMapItemSubPropertyChanged);
    } break;
    }

    QObject::connect(item, &QDeclarativeGeoMapItemBase::mapItemOpacityChanged, q, &QGeoMapMapLibreGL::onMapItemPropertyChanged);

    m_styleChanges << QMapLibreGLStyleChange::addMapItem(item, m_mapItemsBefore);

    emit q->sgNodeChanged();
}

void QGeoMapMapLibreGLPrivate::removeMapItem(QDeclarativeGeoMapItemBase *item)
{
    Q_Q(QGeoMapMapLibreGL);

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

    m_styleChanges << QMapLibreGLStyleChange::removeMapItem(item);

    emit q->sgNodeChanged();
}

void QGeoMapMapLibreGLPrivate::changeViewportSize(const QSize &)
{
    Q_Q(QGeoMapMapLibreGL);

    m_syncState = m_syncState | ViewportSync;
    emit q->sgNodeChanged();
}

void QGeoMapMapLibreGLPrivate::changeCameraData(const QGeoCameraData &)
{
    Q_Q(QGeoMapMapLibreGL);

    m_syncState = m_syncState | CameraDataSync;
    emit q->sgNodeChanged();
}

void QGeoMapMapLibreGLPrivate::changeActiveMapType(const QGeoMapType)
{
    Q_Q(QGeoMapMapLibreGL);

    m_syncState = m_syncState | MapTypeSync;
    emit q->sgNodeChanged();
}

void QGeoMapMapLibreGLPrivate::setVisibleArea(const QRectF &visibleArea)
{
    Q_Q(QGeoMapMapLibreGL);
    const QRectF va = clampVisibleArea(visibleArea);
    if (va == m_visibleArea)
        return;

    m_visibleArea = va;
    m_geoProjection->setVisibleArea(va);

    m_syncState = m_syncState | VisibleAreaSync;
    emit q->sgNodeChanged();
}

QRectF QGeoMapMapLibreGLPrivate::visibleArea() const
{
    return m_visibleArea;
}

void QGeoMapMapLibreGLPrivate::syncStyleChanges(QMapboxGL *map)
{
    for (const auto& change : m_styleChanges) {
        change->apply(map);
    }

    m_styleChanges.clear();
}

void QGeoMapMapLibreGLPrivate::threadedRenderingHack(QQuickWindow *window, QMapboxGL *map)
{
    // FIXME: Optimal support for threaded rendering needs core changes
    // in MapLibre GL Native. Meanwhile we need to set a timer to update
    // the map until all the resources are loaded, which is not exactly
    // battery friendly, because might trigger more paints than we need.
    if (!m_warned) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        m_threadedRendering = static_cast<QOpenGLContext*>(window->rendererInterface()->getResource(window, QSGRendererInterface::OpenGLContextResource))->thread() != QCoreApplication::instance()->thread();
#else
        m_threadedRendering = window->openglContext()->thread() != QCoreApplication::instance()->thread();
#endif

        if (m_threadedRendering) {
            qWarning() << "Threaded rendering is not optimal in the MapLibre GL plugin.";
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
 * QGeoMapMapLibreGL implementation
 */

QGeoMapMapLibreGL::QGeoMapMapLibreGL(QGeoMappingManagerEngineMapLibreGL *engine, QObject *parent)
    :   QGeoMap(*new QGeoMapMapLibreGLPrivate(engine), parent), m_engine(engine)
{
    Q_D(QGeoMapMapLibreGL);

    connect(&d->m_refresh, &QTimer::timeout, this, &QGeoMap::sgNodeChanged);
    d->m_refresh.setInterval(250);
}

QGeoMapMapLibreGL::~QGeoMapMapLibreGL()
{
}

QString QGeoMapMapLibreGL::copyrightsStyleSheet() const
{
    return QStringLiteral("* { vertical-align: middle; font-weight: normal }");
}

void QGeoMapMapLibreGL::setMapLibreGLSettings(const QMapboxGLSettings& settings)
{
    Q_D(QGeoMapMapLibreGL);

    d->m_settings = settings;
}

void QGeoMapMapLibreGL::setUseFBO(bool useFBO)
{
    Q_D(QGeoMapMapLibreGL);
    d->m_useFBO = useFBO;
}

void QGeoMapMapLibreGL::setMapItemsBefore(const QString &before)
{
    Q_D(QGeoMapMapLibreGL);
    d->m_mapItemsBefore = before;
}

QGeoMap::Capabilities QGeoMapMapLibreGL::capabilities() const
{
    return Capabilities(SupportsVisibleRegion
                        | SupportsSetBearing
                        | SupportsAnchoringCoordinate
                        | SupportsVisibleArea );
}

QSGNode *QGeoMapMapLibreGL::updateSceneGraph(QSGNode *oldNode, QQuickWindow *window)
{
    Q_D(QGeoMapMapLibreGL);
    return d->updateSceneGraph(oldNode, window);
}

void QGeoMapMapLibreGL::onMapChanged(QMapboxGL::MapChange change)
{
    Q_D(QGeoMapMapLibreGL);

    if (change == QMapboxGL::MapChangeDidFinishLoadingStyle || change == QMapboxGL::MapChangeDidFailLoadingMap) {
        d->m_styleLoaded = true;
    } else if (change == QMapboxGL::MapChangeWillStartLoadingMap) {
        d->m_styleLoaded = false;
        d->m_styleChanges.clear();

        for (QDeclarativeGeoMapItemBase *item : d->m_mapItems)
            d->m_styleChanges << QMapLibreGLStyleChange::addMapItem(item, d->m_mapItemsBefore);

        for (QGeoMapParameter *param : d->m_mapParameters)
            d->m_styleChanges << QMapLibreGLStyleChange::addMapParameter(param);
    }
}

void QGeoMapMapLibreGL::onMapItemPropertyChanged()
{
    Q_D(QGeoMapMapLibreGL);

    QDeclarativeGeoMapItemBase *item = static_cast<QDeclarativeGeoMapItemBase *>(sender());
    d->m_styleChanges << QMapLibreGLStyleSetPaintProperty::fromMapItem(item);
    d->m_styleChanges << QMapLibreGLStyleSetLayoutProperty::fromMapItem(item);

    emit sgNodeChanged();
}

void QGeoMapMapLibreGL::onMapItemSubPropertyChanged()
{
    Q_D(QGeoMapMapLibreGL);

    QDeclarativeGeoMapItemBase *item = static_cast<QDeclarativeGeoMapItemBase *>(sender()->parent());
    d->m_styleChanges << QMapLibreGLStyleSetPaintProperty::fromMapItem(item);

    emit sgNodeChanged();
}

void QGeoMapMapLibreGL::onMapItemUnsupportedPropertyChanged()
{
    // TODO https://bugreports.qt.io/browse/QTBUG-58872
    qWarning() << "Unsupported property for managed Map item";
}

void QGeoMapMapLibreGL::onMapItemGeometryChanged()
{
    Q_D(QGeoMapMapLibreGL);

    QDeclarativeGeoMapItemBase *item = static_cast<QDeclarativeGeoMapItemBase *>(sender());
    d->m_styleChanges << QMapLibreGLStyleAddSource::fromMapItem(item);

    emit sgNodeChanged();
}

void QGeoMapMapLibreGL::onParameterPropertyUpdated(QGeoMapParameter *param, const char *)
{
    Q_D(QGeoMapMapLibreGL);

    d->m_styleChanges.append(QMapLibreGLStyleChange::addMapParameter(param));

    emit sgNodeChanged();
}

void QGeoMapMapLibreGL::copyrightsChanged(const QString &copyrightsHtml)
{
    Q_D(QGeoMapMapLibreGL);

    QString copyrightsHtmlFinal = copyrightsHtml;

    if (d->m_activeMapType.name().startsWith("mapbox://")) {
        copyrightsHtmlFinal = "<table><tr><th><img src='qrc:/maplibregl/mapbox_logo.png'/></th><th>"
            + copyrightsHtmlFinal + "</th></tr></table>";
    }

    QGeoMap::copyrightsChanged(copyrightsHtmlFinal);
}
