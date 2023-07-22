// Copyright (C) 2023 MapLibre contributors
// Copyright (C) 2017 Mapbox, Inc.

// SPDX-License-Identifier: LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "stylechange_p.h"

#include <QtCore/QDebug>
#include <QtCore/QMetaProperty>
#include <QtCore/QRegularExpression>
#include <QtCore/QStringList>
#include <QtPositioning/QGeoPath>
#include <QtPositioning/QGeoPolygon>
#include <QtQml/QJSValue>
#include <QtLocation/private/qdeclarativecirclemapitem_p_p.h>

#include <QMapLibre/Map>

namespace {

QString getId(QDeclarativeGeoMapItemBase *mapItem) {
    return QStringLiteral("QtLocation-") +
           ((mapItem->objectName().isEmpty()) ? QString::number(quint64(mapItem)) : mapItem->objectName());
}

// MapLibre Native supports geometry segments that spans above 180 degrees in
// longitude. To keep visual expectations in parity with Qt, we need to adapt
// the coordinates to always use the shortest path when in ambiguity.
static bool geoRectangleCrossesDateLine(const QGeoRectangle &rect) {
    return rect.topLeft().longitude() > rect.bottomRight().longitude();
}

QMapLibre::Feature featureFromMapRectangle(QDeclarativeRectangleMapItem *mapItem) {
    const QGeoRectangle *rect = static_cast<const QGeoRectangle *>(&mapItem->geoShape());
    QMapLibre::Coordinate bottomLeft{rect->bottomLeft().latitude(), rect->bottomLeft().longitude()};
    QMapLibre::Coordinate topLeft{rect->topLeft().latitude(), rect->topLeft().longitude()};
    QMapLibre::Coordinate bottomRight{rect->bottomRight().latitude(), rect->bottomRight().longitude()};
    QMapLibre::Coordinate topRight{rect->topRight().latitude(), rect->topRight().longitude()};
    if (geoRectangleCrossesDateLine(*rect)) {
        bottomRight.second += 360.0;
        topRight.second += 360.0;
    }
    QMapLibre::CoordinatesCollections geometry{{{bottomLeft, bottomRight, topRight, topLeft, bottomLeft}}};

    return QMapLibre::Feature(QMapLibre::Feature::PolygonType, geometry, {}, getId(mapItem));
}

QMapLibre::Feature featureFromMapCircle(QDeclarativeCircleMapItem *mapItem) {
    static const int circleSamples = 128;
    const QGeoProjectionWebMercator &p = static_cast<const QGeoProjectionWebMercator &>(
        mapItem->map()->geoProjection());
#if QT_VERSION < QT_VERSION_CHECK(6, 5, 0)
    QList<QGeoCoordinate> path;
    QGeoCoordinate leftBound;
    QDeclarativeCircleMapItemPrivate::calculatePeripheralPoints(
        path, mapItem->center(), mapItem->radius(), circleSamples, leftBound);
#endif
    QList<QDoubleVector2D> pathProjected;
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    QDeclarativeCircleMapItemPrivate::calculatePeripheralPoints(
        pathProjected, mapItem->center(), mapItem->radius(), p, circleSamples);
    // TODO: Removed for now. Update when the 6.6 API for this is fixed
    // if (QDeclarativeCircleMapItemPrivateCPU::crossEarthPole(mapItem->center(), mapItem->radius()))
    //     QDeclarativeCircleMapItemPrivateCPU::preserveCircleGeometry(pathProjected, mapItem->center(),
    //     mapItem->radius(), p);
    QList<QGeoCoordinate> path;
    for (const QDoubleVector2D &c : std::as_const(pathProjected)) path << p.mapProjectionToGeo(c);
#else
    for (const QGeoCoordinate &c : qAsConst(path)) pathProjected << p.geoToMapProjection(c);
    if (QDeclarativeCircleMapItemPrivateCPU::crossEarthPole(mapItem->center(), mapItem->radius()))
        QDeclarativeCircleMapItemPrivateCPU::preserveCircleGeometry(
            pathProjected, mapItem->center(), mapItem->radius(), p);
    path.clear();
    for (const QDoubleVector2D &c : qAsConst(pathProjected)) path << p.mapProjectionToGeo(c);
#endif

    QMapLibre::Coordinates coordinates;
    for (const QGeoCoordinate &coordinate : path) {
        coordinates << QMapLibre::Coordinate{coordinate.latitude(), coordinate.longitude()};
    }
    coordinates.append(coordinates.first()); // closing the path
    QMapLibre::CoordinatesCollections geometry{{coordinates}};
    return QMapLibre::Feature(QMapLibre::Feature::PolygonType, geometry, {}, getId(mapItem));
}

static QMapLibre::Coordinates qgeocoordinate2mapboxcoordinate(const QList<QGeoCoordinate> &crds,
                                                              const bool crossesDateline,
                                                              bool closed = false) {
    QMapLibre::Coordinates coordinates;
    for (const QGeoCoordinate &coordinate : crds) {
        if (!coordinates.empty() && crossesDateline &&
            qAbs(coordinate.longitude() - coordinates.last().second) > 180.0) {
            coordinates << QMapLibre::Coordinate{
                coordinate.latitude(), coordinate.longitude() + (coordinate.longitude() >= 0 ? -360.0 : 360.0)};
        } else {
            coordinates << QMapLibre::Coordinate{coordinate.latitude(), coordinate.longitude()};
        }
    }
    if (closed && !coordinates.empty() && coordinates.last() != coordinates.first())
        coordinates.append(coordinates.first()); // closing the path
    return coordinates;
}

QMapLibre::Feature featureFromMapPolygon(QDeclarativePolygonMapItem *mapItem) {
    const QGeoPolygon *polygon = static_cast<const QGeoPolygon *>(&mapItem->geoShape());
    const bool crossesDateline = geoRectangleCrossesDateLine(polygon->boundingGeoRectangle());
    QMapLibre::CoordinatesCollections geometry;
    QMapLibre::CoordinatesCollection poly;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QMapLibre::Coordinates coordinates = qgeocoordinate2mapboxcoordinate(polygon->perimeter(), crossesDateline, true);
#else
    QMapLibre::Coordinates coordinates = qgeocoordinate2mapboxcoordinate(polygon->path(), crossesDateline, true);
#endif
    poly.push_back(coordinates);
    for (int i = 0; i < polygon->holesCount(); ++i) {
        coordinates = qgeocoordinate2mapboxcoordinate(polygon->holePath(i), crossesDateline, true);
        poly.push_back(coordinates);
    }

    geometry.push_back(poly);
    return QMapLibre::Feature(QMapLibre::Feature::PolygonType, geometry, {}, getId(mapItem));
}

QMapLibre::Feature featureFromMapPolyline(QDeclarativePolylineMapItem *mapItem) {
    const QGeoPath *path = static_cast<const QGeoPath *>(&mapItem->geoShape());
    QMapLibre::Coordinates coordinates;
    const bool crossesDateline = geoRectangleCrossesDateLine(path->boundingGeoRectangle());
    for (const QGeoCoordinate &coordinate : path->path()) {
        if (!coordinates.empty() && crossesDateline &&
            qAbs(coordinate.longitude() - coordinates.last().second) > 180.0) {
            coordinates << QMapLibre::Coordinate{
                coordinate.latitude(), coordinate.longitude() + (coordinate.longitude() >= 0 ? -360.0 : 360.0)};
        } else {
            coordinates << QMapLibre::Coordinate{coordinate.latitude(), coordinate.longitude()};
        }
    }
    QMapLibre::CoordinatesCollections geometry{{coordinates}};

    return QMapLibre::Feature(QMapLibre::Feature::LineStringType, geometry, {}, getId(mapItem));
}

QMapLibre::Feature featureFromMapItem(QDeclarativeGeoMapItemBase *item) {
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
            return QMapLibre::Feature();
    }
}

} // namespace

namespace QMapLibre {

// StyleChange
QList<QSharedPointer<StyleChange>> StyleChange::addMapItem(QDeclarativeGeoMapItemBase *item, const QString &before) {
    QList<QSharedPointer<StyleChange>> changes;

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

    Feature feature = featureFromMapItem(item);

    changes << StyleAddLayer::fromFeature(feature, before);
    changes << StyleAddSource::fromFeature(feature);
    changes << StyleSetPaintProperty::fromMapItem(item);
    changes << StyleSetLayoutProperty::fromMapItem(item);

    return changes;
}

QList<QSharedPointer<StyleChange>> StyleChange::removeMapItem(QDeclarativeGeoMapItemBase *item) {
    QList<QSharedPointer<StyleChange>> changes;

    const QString id = getId(item);

    changes << QSharedPointer<StyleChange>(new StyleRemoveLayer(id));
    changes << QSharedPointer<StyleChange>(new StyleRemoveSource(id));

    return changes;
}

// StyleSetLayoutProperty

void StyleSetLayoutProperty::apply(Map *map) {
    map->setLayoutProperty(m_layer, m_property, m_value);
}

QList<QSharedPointer<StyleChange>> StyleSetLayoutProperty::fromMapItem(QDeclarativeGeoMapItemBase *item) {
    QList<QSharedPointer<StyleChange>> changes;

    switch (item->itemType()) {
        case QGeoMap::MapPolyline:
            changes = fromMapItem(static_cast<QDeclarativePolylineMapItem *>(item));
        default:
            break;
    }

    changes << QSharedPointer<StyleChange>(
        new StyleSetLayoutProperty(getId(item),
                                   QStringLiteral("visibility"),
                                   item->isVisible() ? QStringLiteral("visible") : QStringLiteral("none")));

    return changes;
}

QList<QSharedPointer<StyleChange>> StyleSetLayoutProperty::fromMapItem(QDeclarativePolylineMapItem *item) {
    QList<QSharedPointer<StyleChange>> changes;
    changes.reserve(2);

    const QString id = getId(item);

    changes << QSharedPointer<StyleChange>(
        new StyleSetLayoutProperty(id, QStringLiteral("line-cap"), QStringLiteral("square")));
    changes << QSharedPointer<StyleChange>(
        new StyleSetLayoutProperty(id, QStringLiteral("line-join"), QStringLiteral("bevel")));

    return changes;
}

StyleSetLayoutProperty::StyleSetLayoutProperty(const QString &layer, const QString &property, const QVariant &value)
    : m_layer(layer),
      m_property(property),
      m_value(value) {}

// StyleSetPaintProperty

StyleSetPaintProperty::StyleSetPaintProperty(const QString &layer, const QString &property, const QVariant &value)
    : m_layer(layer),
      m_property(property),
      m_value(value) {}

void StyleSetPaintProperty::apply(Map *map) {
    map->setPaintProperty(m_layer, m_property, m_value);
}

QList<QSharedPointer<StyleChange>> StyleSetPaintProperty::fromMapItem(QDeclarativeGeoMapItemBase *item) {
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
            return QList<QSharedPointer<StyleChange>>();
    }
}

QList<QSharedPointer<StyleChange>> StyleSetPaintProperty::fromMapItem(QDeclarativeRectangleMapItem *item) {
    QList<QSharedPointer<StyleChange>> changes;
    changes.reserve(3);

    const QString id = getId(item);

    changes << QSharedPointer<StyleChange>(
        new StyleSetPaintProperty(id, QStringLiteral("fill-opacity"), item->color().alphaF() * item->mapItemOpacity()));
    changes << QSharedPointer<StyleChange>(new StyleSetPaintProperty(id, QStringLiteral("fill-color"), item->color()));
    changes << QSharedPointer<StyleChange>(
        new StyleSetPaintProperty(id, QStringLiteral("fill-outline-color"), item->border()->color()));

    return changes;
}

QList<QSharedPointer<StyleChange>> StyleSetPaintProperty::fromMapItem(QDeclarativeCircleMapItem *item) {
    QList<QSharedPointer<StyleChange>> changes;
    changes.reserve(3);

    const QString id = getId(item);

    changes << QSharedPointer<StyleChange>(
        new StyleSetPaintProperty(id, QStringLiteral("fill-opacity"), item->color().alphaF() * item->mapItemOpacity()));
    changes << QSharedPointer<StyleChange>(new StyleSetPaintProperty(id, QStringLiteral("fill-color"), item->color()));
    changes << QSharedPointer<StyleChange>(
        new StyleSetPaintProperty(id, QStringLiteral("fill-outline-color"), item->border()->color()));

    return changes;
}

QList<QSharedPointer<StyleChange>> StyleSetPaintProperty::fromMapItem(QDeclarativePolygonMapItem *item) {
    QList<QSharedPointer<StyleChange>> changes;
    changes.reserve(3);

    const QString id = getId(item);

    changes << QSharedPointer<StyleChange>(
        new StyleSetPaintProperty(id, QStringLiteral("fill-opacity"), item->color().alphaF() * item->mapItemOpacity()));
    changes << QSharedPointer<StyleChange>(new StyleSetPaintProperty(id, QStringLiteral("fill-color"), item->color()));
    changes << QSharedPointer<StyleChange>(
        new StyleSetPaintProperty(id, QStringLiteral("fill-outline-color"), item->border()->color()));

    return changes;
}

QList<QSharedPointer<StyleChange>> StyleSetPaintProperty::fromMapItem(QDeclarativePolylineMapItem *item) {
    QList<QSharedPointer<StyleChange>> changes;
    changes.reserve(3);

    const QString id = getId(item);

    changes << QSharedPointer<StyleChange>(new StyleSetPaintProperty(
        id, QStringLiteral("line-opacity"), item->line()->color().alphaF() * item->mapItemOpacity()));
    changes << QSharedPointer<StyleChange>(
        new StyleSetPaintProperty(id, QStringLiteral("line-color"), item->line()->color()));
    changes << QSharedPointer<StyleChange>(
        new StyleSetPaintProperty(id, QStringLiteral("line-width"), item->line()->width()));

    return changes;
}

// StyleAddLayer

void StyleAddLayer::apply(Map *map) {
    map->addLayer(m_params, m_before);
}

QSharedPointer<StyleChange> StyleAddLayer::fromFeature(const Feature &feature, const QString &before) {
    auto layer = new StyleAddLayer();
    layer->m_params[QStringLiteral("id")] = feature.id;
    layer->m_params[QStringLiteral("source")] = feature.id;

    switch (feature.type) {
        case Feature::PointType:
            layer->m_params[QStringLiteral("type")] = QStringLiteral("circle");
            break;
        case Feature::LineStringType:
            layer->m_params[QStringLiteral("type")] = QStringLiteral("line");
            break;
        case Feature::PolygonType:
            layer->m_params[QStringLiteral("type")] = QStringLiteral("fill");
            break;
    }

    layer->m_before = before;

    return QSharedPointer<StyleChange>(layer);
}

// StyleRemoveLayer

void StyleRemoveLayer::apply(Map *map) {
    map->removeLayer(m_id);
}

StyleRemoveLayer::StyleRemoveLayer(const QString &id)
    : m_id(id) {}

// StyleAddSource

void StyleAddSource::apply(Map *map) {
    map->updateSource(m_id, m_params);
}

QSharedPointer<StyleChange> StyleAddSource::fromFeature(const Feature &feature) {
    auto source = new StyleAddSource();

    source->m_id = feature.id.toString();
    source->m_params[QStringLiteral("type")] = QStringLiteral("geojson");
    source->m_params[QStringLiteral("data")] = QVariant::fromValue<Feature>(feature);

    return QSharedPointer<StyleChange>(source);
}

QSharedPointer<StyleChange> StyleAddSource::fromMapItem(QDeclarativeGeoMapItemBase *item) {
    return fromFeature(featureFromMapItem(item));
}

// StyleRemoveSource

void StyleRemoveSource::apply(Map *map) {
    map->removeSource(m_id);
}

StyleRemoveSource::StyleRemoveSource(const QString &id)
    : m_id(id) {}

// StyleSetFilter

void StyleSetFilter::apply(Map *map) {
    map->setFilter(m_layer, m_filter);
}

// StyleAddImage

void StyleAddImage::apply(Map *map) {
    map->addImage(m_name, m_sprite);
}

} // namespace QMapLibre
