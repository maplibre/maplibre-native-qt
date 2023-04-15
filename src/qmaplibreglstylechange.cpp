// Copyright (C) 2023 MapLibre contributors
// Copyright (C) 2017 Mapbox, Inc.

// SPDX-License-Identifier: LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qmaplibreglstylechange_p.h"

#include <QtCore/QDebug>
#include <QtCore/QMetaProperty>
#include <QtCore/QRegularExpression>
#include <QtCore/QStringList>
#include <QtPositioning/QGeoPath>
#include <QtPositioning/QGeoPolygon>
#include <QtQml/QJSValue>
#include <QtLocation/private/qdeclarativecirclemapitem_p_p.h>

#include <QMapLibreGL/Map>

namespace {

QString getId(QDeclarativeGeoMapItemBase *mapItem)
{
    return QStringLiteral("QtLocation-") +
            ((mapItem->objectName().isEmpty()) ? QString::number(quint64(mapItem)) : mapItem->objectName());
}

// MapLibre GL supports geometry segments that spans above 180 degrees in
// longitude. To keep visual expectations in parity with Qt, we need to adapt
// the coordinates to always use the shortest path when in ambiguity.
static bool geoRectangleCrossesDateLine(const QGeoRectangle &rect) {
    return rect.topLeft().longitude() > rect.bottomRight().longitude();
}

QMapLibreGL::Feature featureFromMapRectangle(QDeclarativeRectangleMapItem *mapItem)
{
    const QGeoRectangle *rect = static_cast<const QGeoRectangle *>(&mapItem->geoShape());
    QMapLibreGL::Coordinate bottomLeft { rect->bottomLeft().latitude(), rect->bottomLeft().longitude() };
    QMapLibreGL::Coordinate topLeft { rect->topLeft().latitude(), rect->topLeft().longitude() };
    QMapLibreGL::Coordinate bottomRight { rect->bottomRight().latitude(), rect->bottomRight().longitude() };
    QMapLibreGL::Coordinate topRight { rect->topRight().latitude(), rect->topRight().longitude() };
    if (geoRectangleCrossesDateLine(*rect)) {
        bottomRight.second += 360.0;
        topRight.second += 360.0;
    }
    QMapLibreGL::CoordinatesCollections geometry { { { bottomLeft, bottomRight, topRight, topLeft, bottomLeft } } };

    return QMapLibreGL::Feature(QMapLibreGL::Feature::PolygonType, geometry, {}, getId(mapItem));
}

QMapLibreGL::Feature featureFromMapCircle(QDeclarativeCircleMapItem *mapItem)
{
    static const int circleSamples = 128;
    const QGeoProjectionWebMercator &p = static_cast<const QGeoProjectionWebMercator&>(mapItem->map()->geoProjection());
#if QT_VERSION < QT_VERSION_CHECK(6, 5, 0)
    QList<QGeoCoordinate> path;
    QGeoCoordinate leftBound;
    QDeclarativeCircleMapItemPrivate::calculatePeripheralPoints(path, mapItem->center(), mapItem->radius(), circleSamples, leftBound);
#endif
    QList<QDoubleVector2D> pathProjected;
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    QDeclarativeCircleMapItemPrivate::calculatePeripheralPoints(pathProjected, mapItem->center(), mapItem->radius(), p, circleSamples);
    // TODO: Removed for now. Update when the 6.6 API for this is fixed
    // if (QDeclarativeCircleMapItemPrivateCPU::crossEarthPole(mapItem->center(), mapItem->radius()))
    //     QDeclarativeCircleMapItemPrivateCPU::preserveCircleGeometry(pathProjected, mapItem->center(), mapItem->radius(), p);
    QList<QGeoCoordinate> path;
    for (const QDoubleVector2D &c : std::as_const(pathProjected))
        path << p.mapProjectionToGeo(c);
#else
    for (const QGeoCoordinate &c : qAsConst(path))
        pathProjected << p.geoToMapProjection(c);
    if (QDeclarativeCircleMapItemPrivateCPU::crossEarthPole(mapItem->center(), mapItem->radius()))
        QDeclarativeCircleMapItemPrivateCPU::preserveCircleGeometry(pathProjected, mapItem->center(), mapItem->radius(), p);
    path.clear();
    for (const QDoubleVector2D &c : qAsConst(pathProjected))
        path << p.mapProjectionToGeo(c);
#endif


    QMapLibreGL::Coordinates coordinates;
    for (const QGeoCoordinate &coordinate : path) {
        coordinates << QMapLibreGL::Coordinate { coordinate.latitude(), coordinate.longitude() };
    }
    coordinates.append(coordinates.first());  // closing the path
    QMapLibreGL::CoordinatesCollections geometry { { coordinates } };
    return QMapLibreGL::Feature(QMapLibreGL::Feature::PolygonType, geometry, {}, getId(mapItem));
}

static QMapLibreGL::Coordinates qgeocoordinate2mapboxcoordinate(const QList<QGeoCoordinate> &crds, const bool crossesDateline, bool closed = false)
{
    QMapLibreGL::Coordinates coordinates;
    for (const QGeoCoordinate &coordinate : crds) {
        if (!coordinates.empty() && crossesDateline && qAbs(coordinate.longitude() - coordinates.last().second) > 180.0) {
            coordinates << QMapLibreGL::Coordinate { coordinate.latitude(), coordinate.longitude() + (coordinate.longitude() >= 0 ? -360.0 : 360.0) };
        } else {
            coordinates << QMapLibreGL::Coordinate { coordinate.latitude(), coordinate.longitude() };
        }
    }
    if (closed && !coordinates.empty() && coordinates.last() != coordinates.first())
        coordinates.append(coordinates.first());  // closing the path
    return coordinates;
}

QMapLibreGL::Feature featureFromMapPolygon(QDeclarativePolygonMapItem *mapItem)
{
    const QGeoPolygon *polygon = static_cast<const QGeoPolygon *>(&mapItem->geoShape());
    const bool crossesDateline = geoRectangleCrossesDateLine(polygon->boundingGeoRectangle());
    QMapLibreGL::CoordinatesCollections geometry;
    QMapLibreGL::CoordinatesCollection poly;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QMapLibreGL::Coordinates coordinates = qgeocoordinate2mapboxcoordinate(polygon->perimeter(), crossesDateline, true);
#else
    QMapLibreGL::Coordinates coordinates = qgeocoordinate2mapboxcoordinate(polygon->path(), crossesDateline, true);
#endif
    poly.push_back(coordinates);
    for (int i = 0; i < polygon->holesCount(); ++i) {
        coordinates = qgeocoordinate2mapboxcoordinate(polygon->holePath(i), crossesDateline, true);
        poly.push_back(coordinates);
    }

    geometry.push_back(poly);
    return QMapLibreGL::Feature(QMapLibreGL::Feature::PolygonType, geometry, {}, getId(mapItem));
}

QMapLibreGL::Feature featureFromMapPolyline(QDeclarativePolylineMapItem *mapItem)
{
    const QGeoPath *path = static_cast<const QGeoPath *>(&mapItem->geoShape());
    QMapLibreGL::Coordinates coordinates;
    const bool crossesDateline = geoRectangleCrossesDateLine(path->boundingGeoRectangle());
    for (const QGeoCoordinate &coordinate : path->path()) {
        if (!coordinates.empty() && crossesDateline && qAbs(coordinate.longitude() - coordinates.last().second) > 180.0) {
            coordinates << QMapLibreGL::Coordinate { coordinate.latitude(), coordinate.longitude() + (coordinate.longitude() >= 0 ? -360.0 : 360.0) };
        } else {
            coordinates << QMapLibreGL::Coordinate { coordinate.latitude(), coordinate.longitude() };
        }
    }
    QMapLibreGL::CoordinatesCollections geometry { { coordinates } };

    return QMapLibreGL::Feature(QMapLibreGL::Feature::LineStringType, geometry, {}, getId(mapItem));
}

QMapLibreGL::Feature featureFromMapItem(QDeclarativeGeoMapItemBase *item)
{
    switch (item->itemType()) {
    case QGeoMap::MapRectangle:
        return featureFromMapRectangle(static_cast<QDeclarativeRectangleMapItem *>(item));
    case QGeoMap::MapCircle:
        return featureFromMapCircle(static_cast<QDeclarativeCircleMapItem *>(item));
    case QGeoMap::MapPolygon:
        return featureFromMapPolygon(static_cast<QDeclarativePolygonMapItem *>(item));
    case QGeoMap::MapPolyline:
        return featureFromMapPolyline(static_cast<QDeclarativePolylineMapItem *>(item));
    default:
        qWarning() << "Unsupported QGeoMap item type: " << item->itemType();
        return QMapLibreGL::Feature();
    }
}

} // namespace


// QMapLibreGLStyleChange
QList<QSharedPointer<QMapLibreGLStyleChange>> QMapLibreGLStyleChange::addMapItem(QDeclarativeGeoMapItemBase *item, const QString &before)
{
    QList<QSharedPointer<QMapLibreGLStyleChange>> changes;

    switch (item->itemType()) {
    case QGeoMap::MapRectangle:
    case QGeoMap::MapCircle:
    case QGeoMap::MapPolygon:
    case QGeoMap::MapPolyline:
        break;
    default:
        qWarning() << "Unsupported QGeoMap item type: " << item->itemType();
        return changes;
    }

    QMapLibreGL::Feature feature = featureFromMapItem(item);

    changes << QMapLibreGLStyleAddLayer::fromFeature(feature, before);
    changes << QMapLibreGLStyleAddSource::fromFeature(feature);
    changes << QMapLibreGLStyleSetPaintProperty::fromMapItem(item);
    changes << QMapLibreGLStyleSetLayoutProperty::fromMapItem(item);

    return changes;
}

QList<QSharedPointer<QMapLibreGLStyleChange>> QMapLibreGLStyleChange::removeMapItem(QDeclarativeGeoMapItemBase *item)
{
    QList<QSharedPointer<QMapLibreGLStyleChange>> changes;

    const QString id = getId(item);

    changes << QSharedPointer<QMapLibreGLStyleChange>(new QMapLibreGLStyleRemoveLayer(id));
    changes << QSharedPointer<QMapLibreGLStyleChange>(new QMapLibreGLStyleRemoveSource(id));

    return changes;
}

// QMapLibreGLStyleSetLayoutProperty

void QMapLibreGLStyleSetLayoutProperty::apply(QMapLibreGL::Map *map)
{
    map->setLayoutProperty(m_layer, m_property, m_value);
}

QList<QSharedPointer<QMapLibreGLStyleChange>> QMapLibreGLStyleSetLayoutProperty::fromMapItem(QDeclarativeGeoMapItemBase *item)
{
    QList<QSharedPointer<QMapLibreGLStyleChange>> changes;

    switch (item->itemType()) {
    case QGeoMap::MapPolyline:
        changes = fromMapItem(static_cast<QDeclarativePolylineMapItem *>(item));
    default:
        break;
    }

    changes << QSharedPointer<QMapLibreGLStyleChange>(
        new QMapLibreGLStyleSetLayoutProperty(getId(item), QStringLiteral("visibility"),
            item->isVisible() ? QStringLiteral("visible") : QStringLiteral("none")));

    return changes;
}

QList<QSharedPointer<QMapLibreGLStyleChange>> QMapLibreGLStyleSetLayoutProperty::fromMapItem(QDeclarativePolylineMapItem *item)
{
    QList<QSharedPointer<QMapLibreGLStyleChange>> changes;
    changes.reserve(2);

    const QString id = getId(item);

    changes << QSharedPointer<QMapLibreGLStyleChange>(
        new QMapLibreGLStyleSetLayoutProperty(id, QStringLiteral("line-cap"), QStringLiteral("square")));
    changes << QSharedPointer<QMapLibreGLStyleChange>(
        new QMapLibreGLStyleSetLayoutProperty(id, QStringLiteral("line-join"), QStringLiteral("bevel")));

    return changes;
}

QMapLibreGLStyleSetLayoutProperty::QMapLibreGLStyleSetLayoutProperty(const QString& layer, const QString& property, const QVariant &value)
    : m_layer(layer), m_property(property), m_value(value)
{
}

// QMapLibreGLStyleSetPaintProperty

QMapLibreGLStyleSetPaintProperty::QMapLibreGLStyleSetPaintProperty(const QString& layer, const QString& property, const QVariant &value)
    : m_layer(layer), m_property(property), m_value(value)
{
}

void QMapLibreGLStyleSetPaintProperty::apply(QMapLibreGL::Map *map)
{
    map->setPaintProperty(m_layer, m_property, m_value);
}

QList<QSharedPointer<QMapLibreGLStyleChange>> QMapLibreGLStyleSetPaintProperty::fromMapItem(QDeclarativeGeoMapItemBase *item)
{
    switch (item->itemType()) {
    case QGeoMap::MapRectangle:
        return fromMapItem(static_cast<QDeclarativeRectangleMapItem *>(item));
    case QGeoMap::MapCircle:
        return fromMapItem(static_cast<QDeclarativeCircleMapItem *>(item));
    case QGeoMap::MapPolygon:
        return fromMapItem(static_cast<QDeclarativePolygonMapItem *>(item));
    case QGeoMap::MapPolyline:
        return fromMapItem(static_cast<QDeclarativePolylineMapItem *>(item));
    default:
        qWarning() << "Unsupported QGeoMap item type: " << item->itemType();
        return QList<QSharedPointer<QMapLibreGLStyleChange>>();
    }
}

QList<QSharedPointer<QMapLibreGLStyleChange>> QMapLibreGLStyleSetPaintProperty::fromMapItem(QDeclarativeRectangleMapItem *item)
{
    QList<QSharedPointer<QMapLibreGLStyleChange>> changes;
    changes.reserve(3);

    const QString id = getId(item);

    changes << QSharedPointer<QMapLibreGLStyleChange>(
        new QMapLibreGLStyleSetPaintProperty(id, QStringLiteral("fill-opacity"), item->color().alphaF() * item->mapItemOpacity()));
    changes << QSharedPointer<QMapLibreGLStyleChange>(
        new QMapLibreGLStyleSetPaintProperty(id, QStringLiteral("fill-color"), item->color()));
    changes << QSharedPointer<QMapLibreGLStyleChange>(
        new QMapLibreGLStyleSetPaintProperty(id, QStringLiteral("fill-outline-color"), item->border()->color()));

    return changes;
}

QList<QSharedPointer<QMapLibreGLStyleChange>> QMapLibreGLStyleSetPaintProperty::fromMapItem(QDeclarativeCircleMapItem *item)
{
    QList<QSharedPointer<QMapLibreGLStyleChange>> changes;
    changes.reserve(3);

    const QString id = getId(item);

    changes << QSharedPointer<QMapLibreGLStyleChange>(
        new QMapLibreGLStyleSetPaintProperty(id, QStringLiteral("fill-opacity"), item->color().alphaF() * item->mapItemOpacity()));
    changes << QSharedPointer<QMapLibreGLStyleChange>(
        new QMapLibreGLStyleSetPaintProperty(id, QStringLiteral("fill-color"), item->color()));
    changes << QSharedPointer<QMapLibreGLStyleChange>(
        new QMapLibreGLStyleSetPaintProperty(id, QStringLiteral("fill-outline-color"), item->border()->color()));

    return changes;
}

QList<QSharedPointer<QMapLibreGLStyleChange>> QMapLibreGLStyleSetPaintProperty::fromMapItem(QDeclarativePolygonMapItem *item)
{
    QList<QSharedPointer<QMapLibreGLStyleChange>> changes;
    changes.reserve(3);

    const QString id = getId(item);

    changes << QSharedPointer<QMapLibreGLStyleChange>(
        new QMapLibreGLStyleSetPaintProperty(id, QStringLiteral("fill-opacity"), item->color().alphaF() * item->mapItemOpacity()));
    changes << QSharedPointer<QMapLibreGLStyleChange>(
        new QMapLibreGLStyleSetPaintProperty(id, QStringLiteral("fill-color"), item->color()));
    changes << QSharedPointer<QMapLibreGLStyleChange>(
        new QMapLibreGLStyleSetPaintProperty(id, QStringLiteral("fill-outline-color"), item->border()->color()));

    return changes;
}

QList<QSharedPointer<QMapLibreGLStyleChange>> QMapLibreGLStyleSetPaintProperty::fromMapItem(QDeclarativePolylineMapItem *item)
{
    QList<QSharedPointer<QMapLibreGLStyleChange>> changes;
    changes.reserve(3);

    const QString id = getId(item);

    changes << QSharedPointer<QMapLibreGLStyleChange>(
        new QMapLibreGLStyleSetPaintProperty(id, QStringLiteral("line-opacity"), item->line()->color().alphaF() * item->mapItemOpacity()));
    changes << QSharedPointer<QMapLibreGLStyleChange>(
        new QMapLibreGLStyleSetPaintProperty(id, QStringLiteral("line-color"), item->line()->color()));
    changes << QSharedPointer<QMapLibreGLStyleChange>(
        new QMapLibreGLStyleSetPaintProperty(id, QStringLiteral("line-width"), item->line()->width()));

    return changes;
}

// QMapLibreGLStyleAddLayer

void QMapLibreGLStyleAddLayer::apply(QMapLibreGL::Map *map)
{
    map->addLayer(m_params, m_before);
}

QSharedPointer<QMapLibreGLStyleChange> QMapLibreGLStyleAddLayer::fromFeature(const QMapLibreGL::Feature &feature, const QString &before)
{
    auto layer = new QMapLibreGLStyleAddLayer();
    layer->m_params[QStringLiteral("id")] = feature.id;
    layer->m_params[QStringLiteral("source")] = feature.id;

    switch (feature.type) {
    case QMapLibreGL::Feature::PointType:
        layer->m_params[QStringLiteral("type")] = QStringLiteral("circle");
        break;
    case QMapLibreGL::Feature::LineStringType:
        layer->m_params[QStringLiteral("type")] = QStringLiteral("line");
        break;
    case QMapLibreGL::Feature::PolygonType:
        layer->m_params[QStringLiteral("type")] = QStringLiteral("fill");
        break;
    }

    layer->m_before = before;

    return QSharedPointer<QMapLibreGLStyleChange>(layer);
}


// QMapLibreGLStyleRemoveLayer

void QMapLibreGLStyleRemoveLayer::apply(QMapLibreGL::Map *map)
{
    map->removeLayer(m_id);
}

QMapLibreGLStyleRemoveLayer::QMapLibreGLStyleRemoveLayer(const QString &id) : m_id(id)
{
}


// QMapLibreGLStyleAddSource

void QMapLibreGLStyleAddSource::apply(QMapLibreGL::Map *map)
{
    map->updateSource(m_id, m_params);
}

QSharedPointer<QMapLibreGLStyleChange> QMapLibreGLStyleAddSource::fromFeature(const QMapLibreGL::Feature &feature)
{
    auto source = new QMapLibreGLStyleAddSource();

    source->m_id = feature.id.toString();
    source->m_params[QStringLiteral("type")] = QStringLiteral("geojson");
    source->m_params[QStringLiteral("data")] = QVariant::fromValue<QMapLibreGL::Feature>(feature);

    return QSharedPointer<QMapLibreGLStyleChange>(source);
}

QSharedPointer<QMapLibreGLStyleChange> QMapLibreGLStyleAddSource::fromMapItem(QDeclarativeGeoMapItemBase *item)
{
    return fromFeature(featureFromMapItem(item));
}


// QMapLibreGLStyleRemoveSource

void QMapLibreGLStyleRemoveSource::apply(QMapLibreGL::Map *map)
{
    map->removeSource(m_id);
}

QMapLibreGLStyleRemoveSource::QMapLibreGLStyleRemoveSource(const QString &id) : m_id(id)
{
}


// QMapLibreGLStyleSetFilter

void QMapLibreGLStyleSetFilter::apply(QMapLibreGL::Map *map)
{
    map->setFilter(m_layer, m_filter);
}


// QMapLibreGLStyleAddImage

void QMapLibreGLStyleAddImage::apply(QMapLibreGL::Map *map)
{
    map->addImage(m_name, m_sprite);
}
