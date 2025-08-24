// Copyright (C) 2023 MapLibre contributors
// Copyright (C) 2017 Mapbox, Inc.

// SPDX-License-Identifier: LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "style_change_utils_p.hpp"
#include "types.hpp"

#include <QtLocation/private/qdeclarativecirclemapitem_p_p.h>

#include <algorithm>

namespace {

constexpr double fullCircle{360.0};
constexpr double halfCircle{180.0};

// MapLibre Native supports geometry segments that spans above 180 degrees in
// longitude. To keep visual expectations in parity with Qt, we need to adapt
// the coordinates to always use the shortest path when in ambiguity.
bool geoRectangleCrossesDateLine(const QGeoRectangle &rect) {
    return rect.topLeft().longitude() > rect.bottomRight().longitude();
}

QMapLibre::Coordinates qgeocoordinate2mapboxcoordinate(const QList<QGeoCoordinate> &crds,
                                                       const bool crossesDateline,
                                                       bool closed = false) {
    QMapLibre::Coordinates coordinates;
    for (const QGeoCoordinate &coordinate : crds) {
        if (!coordinates.empty() && crossesDateline &&
            qAbs(coordinate.longitude() - coordinates.last().second) > halfCircle) {
            coordinates << QMapLibre::Coordinate{
                coordinate.latitude(),
                coordinate.longitude() + (coordinate.longitude() >= 0 ? -fullCircle : fullCircle)};
        } else {
            coordinates << QMapLibre::Coordinate{coordinate.latitude(), coordinate.longitude()};
        }
    }
    if (closed && !coordinates.empty() && coordinates.last() != coordinates.first()) {
        coordinates.append(coordinates.first()); // closing the path
    }
    return coordinates;
}

} // namespace

namespace QMapLibre::StyleChangeUtils {

QString featureId(QDeclarativeGeoMapItemBase *item) {
    return QStringLiteral("QtLocation-") + ((item->objectName().isEmpty())
                                                ? QString::number(quint64(item)) // NOLINT(google-readability-casting)
                                                : item->objectName());
}

Feature featureFromMapRectangle(QDeclarativeRectangleMapItem *item) {
    const auto *rect = static_cast<const QGeoRectangle *>(&item->geoShape());
    Coordinate bottomLeft{rect->bottomLeft().latitude(), rect->bottomLeft().longitude()};
    Coordinate topLeft{rect->topLeft().latitude(), rect->topLeft().longitude()};
    Coordinate bottomRight{rect->bottomRight().latitude(), rect->bottomRight().longitude()};
    Coordinate topRight{rect->topRight().latitude(), rect->topRight().longitude()};
    if (geoRectangleCrossesDateLine(*rect)) {
        bottomRight.second += fullCircle;
        topRight.second += fullCircle;
    }
    const CoordinatesCollections geometry{{{bottomLeft, bottomRight, topRight, topLeft, bottomLeft}}};

    return Feature(Feature::PolygonType, geometry, {}, featureId(item));
}

Feature featureFromMapCircle(QDeclarativeCircleMapItem *item) {
    constexpr int circleSamples{128};
    const auto &p = static_cast<const QGeoProjectionWebMercator &>(item->map()->geoProjection());
#if QT_VERSION < QT_VERSION_CHECK(6, 5, 0)
    QList<QGeoCoordinate> path;
    QGeoCoordinate leftBound;
    QDeclarativeCircleMapItemPrivate::calculatePeripheralPoints(
        path, item->center(), item->radius(), circleSamples, leftBound);
#endif
    QList<QDoubleVector2D> pathProjected;
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    QDeclarativeCircleMapItemPrivate::calculatePeripheralPointsSimple(
        pathProjected, item->center(), item->radius(), p, circleSamples);
    // TODO: Removed for now. Update when the 6.6 API for this is fixed
    // if (QDeclarativeCircleMapItemPrivateCPU::crossEarthPole(item->center(), item->radius()))
    //     QDeclarativeCircleMapItemPrivateCPU::preserveCircleGeometry(pathProjected, item->center(),
    //     item->radius(), p);
    QList<QGeoCoordinate> path;
    for (const QDoubleVector2D &c : std::as_const(pathProjected)) {
        path << p.mapProjectionToGeo(c);
    }
#elif QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    QDeclarativeCircleMapItemPrivate::calculatePeripheralPoints(
        pathProjected, item->center(), item->radius(), p, circleSamples);
    QList<QGeoCoordinate> path;
    for (const QDoubleVector2D &c : std::as_const(pathProjected)) path << p.mapProjectionToGeo(c);
#else
    for (const QGeoCoordinate &c : qAsConst(path)) pathProjected << p.geoToMapProjection(c);
    if (QDeclarativeCircleMapItemPrivateCPU::crossEarthPole(item->center(), item->radius()))
        QDeclarativeCircleMapItemPrivateCPU::preserveCircleGeometry(pathProjected, item->center(), item->radius(), p);
    path.clear();
    for (const QDoubleVector2D &c : qAsConst(pathProjected)) path << p.mapProjectionToGeo(c);
#endif

    Coordinates coordinates;
    for (const QGeoCoordinate &coordinate : path) {
        coordinates << Coordinate{coordinate.latitude(), coordinate.longitude()};
    }
    coordinates.append(coordinates.first()); // closing the path
    const CoordinatesCollections geometry{{coordinates}};
    return Feature(Feature::PolygonType, geometry, {}, featureId(item));
}

Feature featureFromMapPolygon(QDeclarativePolygonMapItem *item) {
    const auto *polygon = static_cast<const QGeoPolygon *>(&item->geoShape());
    const bool crossesDateline = geoRectangleCrossesDateLine(polygon->boundingGeoRectangle());
    CoordinatesCollections geometry;
    CoordinatesCollection poly;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    Coordinates coordinates = qgeocoordinate2mapboxcoordinate(polygon->perimeter(), crossesDateline, true);
#else
    Coordinates coordinates = qgeocoordinate2mapboxcoordinate(polygon->path(), crossesDateline, true);
#endif
    poly.push_back(coordinates);
    for (int i = 0; i < polygon->holesCount(); ++i) {
        coordinates = qgeocoordinate2mapboxcoordinate(polygon->holePath(i), crossesDateline, true);
        poly.push_back(coordinates);
    }

    geometry.push_back(poly);
    return Feature(Feature::PolygonType, geometry, {}, featureId(item));
}

Feature featureFromMapPolyline(QDeclarativePolylineMapItem *item) {
    const auto *path = static_cast<const QGeoPath *>(&item->geoShape());
    Coordinates coordinates;
    const bool crossesDateline = geoRectangleCrossesDateLine(path->boundingGeoRectangle());
    for (const QGeoCoordinate &coordinate : path->path()) {
        if (!coordinates.empty() && crossesDateline &&
            qAbs(coordinate.longitude() - coordinates.last().second) > halfCircle) {
            coordinates << Coordinate{
                coordinate.latitude(),
                coordinate.longitude() + (coordinate.longitude() >= 0 ? -fullCircle : fullCircle)};
        } else {
            coordinates << Coordinate{coordinate.latitude(), coordinate.longitude()};
        }
    }
    const CoordinatesCollections geometry{{coordinates}};

    return Feature(Feature::LineStringType, geometry, {}, featureId(item));
}

Feature featureFromMapItem(QDeclarativeGeoMapItemBase *item) {
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
            return Feature();
    }
}

std::vector<FeatureProperty> featureLayoutPropertiesFromMapPolyline(QDeclarativePolylineMapItem * /* item */) {
    std::vector<FeatureProperty> properties;
    properties.reserve(2);

    properties.emplace_back(FeatureProperty::LayoutProperty, QStringLiteral("line-cap"), QStringLiteral("square"));
    properties.emplace_back(FeatureProperty::LayoutProperty, QStringLiteral("line-join"), QStringLiteral("bevel"));

    return properties;
}

std::vector<FeatureProperty> featureLayoutPropertiesFromMapItem(QDeclarativeGeoMapItemBase *item) {
    std::vector<FeatureProperty> properties;

    switch (item->itemType()) {
        case QGeoMap::MapPolyline:
            properties = featureLayoutPropertiesFromMapPolyline(static_cast<QDeclarativePolylineMapItem *>(item));
        default:
            break;
    }

    properties.emplace_back(FeatureProperty::LayoutProperty,
                            QStringLiteral("visibility"),
                            item->isVisible() ? QStringLiteral("visible") : QStringLiteral("none"));

    return properties;
}

std::vector<FeatureProperty> featurePaintPropertiesFromMapRectangle(QDeclarativeRectangleMapItem *item) {
    std::vector<FeatureProperty> properties;
    properties.reserve(3);

    properties.emplace_back(FeatureProperty::PaintProperty,
                            QStringLiteral("fill-opacity"),
                            item->color().alphaF() * item->mapItemOpacity());
    properties.emplace_back(FeatureProperty::PaintProperty, QStringLiteral("fill-color"), item->color());
    properties.emplace_back(
        FeatureProperty::PaintProperty, QStringLiteral("fill-outline-color"), item->border()->color());

    return properties;
}

std::vector<FeatureProperty> featurePaintPropertiesFromMapCircle(QDeclarativeCircleMapItem *item) {
    std::vector<FeatureProperty> properties;
    properties.reserve(3);

    properties.emplace_back(FeatureProperty::PaintProperty,
                            QStringLiteral("fill-opacity"),
                            item->color().alphaF() * item->mapItemOpacity());
    properties.emplace_back(FeatureProperty::PaintProperty, QStringLiteral("fill-color"), item->color());
    properties.emplace_back(
        FeatureProperty::PaintProperty, QStringLiteral("fill-outline-color"), item->border()->color());

    return properties;
}

std::vector<FeatureProperty> featurePaintPropertiesFromMapPolygon(QDeclarativePolygonMapItem *item) {
    std::vector<FeatureProperty> properties;
    properties.reserve(3);

    properties.emplace_back(FeatureProperty::PaintProperty,
                            QStringLiteral("fill-opacity"),
                            item->color().alphaF() * item->mapItemOpacity());
    properties.emplace_back(FeatureProperty::PaintProperty, QStringLiteral("fill-color"), item->color());
    properties.emplace_back(
        FeatureProperty::PaintProperty, QStringLiteral("fill-outline-color"), item->border()->color());

    return properties;
}

std::vector<FeatureProperty> featurePaintPropertiesFromMapPolyline(QDeclarativePolylineMapItem *item) {
    std::vector<FeatureProperty> properties;
    properties.reserve(3);

    properties.emplace_back(FeatureProperty::PaintProperty,
                            QStringLiteral("line-opacity"),
                            item->line()->color().alphaF() * item->mapItemOpacity());
    properties.emplace_back(FeatureProperty::PaintProperty, QStringLiteral("line-color"), item->line()->color());
    properties.emplace_back(FeatureProperty::PaintProperty, QStringLiteral("line-width"), item->line()->width());

    return properties;
}

std::vector<FeatureProperty> featurePaintPropertiesFromMapItem(QDeclarativeGeoMapItemBase *item) {
    switch (item->itemType()) {
        case QGeoMap::MapRectangle:
            return featurePaintPropertiesFromMapRectangle(static_cast<QDeclarativeRectangleMapItem *>(item));
        case QGeoMap::MapCircle:
            return featurePaintPropertiesFromMapCircle(static_cast<QDeclarativeCircleMapItem *>(item));
        case QGeoMap::MapPolygon:
            return featurePaintPropertiesFromMapPolygon(static_cast<QDeclarativePolygonMapItem *>(item));
        case QGeoMap::MapPolyline:
            return featurePaintPropertiesFromMapPolyline(static_cast<QDeclarativePolylineMapItem *>(item));
        default:
            qWarning() << "Unsupported QGeoMap item type: " << item->itemType();
            return {};
    }
}

std::vector<FeatureProperty> featurePropertiesFromMapItem(QDeclarativeGeoMapItemBase *item) {
    std::vector<FeatureProperty> layoutProperties = featureLayoutPropertiesFromMapItem(item);
    std::vector<FeatureProperty> paintProperties = featurePaintPropertiesFromMapItem(item);

    std::vector<FeatureProperty> properties;
    properties.reserve(layoutProperties.size() + paintProperties.size());
    std::ranges::move(layoutProperties, std::back_inserter(properties));
    std::ranges::move(paintProperties, std::back_inserter(properties));
    return properties;
}

} // namespace QMapLibre::StyleChangeUtils
