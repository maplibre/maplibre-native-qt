// Copyright (C) 2022 MapLibre contributors
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

QByteArray formatPropertyName(const QByteArray &name)
{
    QString nameAsString = QString::fromLatin1(name);
    static const QRegularExpression camelCaseRegex(QStringLiteral("([a-z0-9])([A-Z])"));
    return nameAsString.replace(camelCaseRegex, QStringLiteral("\\1-\\2")).toLower().toLatin1();
}

bool isImmutableProperty(const QByteArray &name)
{
    return name == QStringLiteral("type") || name == QStringLiteral("layer");
}

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
    QList<QGeoCoordinate> path;
    QGeoCoordinate leftBound;
    QDeclarativeCircleMapItemPrivate::calculatePeripheralPoints(path, mapItem->center(), mapItem->radius(), circleSamples, leftBound);
    QList<QDoubleVector2D> pathProjected;
    for (const QGeoCoordinate &c : qAsConst(path))
        pathProjected << p.geoToMapProjection(c);
    if (QDeclarativeCircleMapItemPrivateCPU::crossEarthPole(mapItem->center(), mapItem->radius()))
        QDeclarativeCircleMapItemPrivateCPU::preserveCircleGeometry(pathProjected, mapItem->center(), mapItem->radius(), p);
    path.clear();
    for (const QDoubleVector2D &c : qAsConst(pathProjected))
        path << p.mapProjectionToGeo(c);


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

QList<QByteArray> getAllPropertyNamesList(QObject *object)
{
    const QMetaObject *metaObject = object->metaObject();
    QList<QByteArray> propertyNames(object->dynamicPropertyNames());
    for (int i = metaObject->propertyOffset(); i < metaObject->propertyCount(); ++i) {
        propertyNames.append(metaObject->property(i).name());
    }
    return propertyNames;
}

} // namespace


// QMapLibreGLStyleChange

QList<QSharedPointer<QMapLibreGLStyleChange>> QMapLibreGLStyleChange::addMapParameter(QGeoMapParameter *param)
{
    static const QStringList acceptedParameterTypes = QStringList()
        << QStringLiteral("paint") << QStringLiteral("layout") << QStringLiteral("filter")
        << QStringLiteral("layer") << QStringLiteral("source") << QStringLiteral("image");

    QList<QSharedPointer<QMapLibreGLStyleChange>> changes;

    switch (acceptedParameterTypes.indexOf(param->type())) {
    case -1:
        qWarning() << "Invalid value for property 'type': " + param->type();
        break;
    case 0: // paint
        changes << QMapLibreGLStyleSetPaintProperty::fromMapParameter(param);
        break;
    case 1: // layout
        changes << QMapLibreGLStyleSetLayoutProperty::fromMapParameter(param);
        break;
    case 2: // filter
        changes << QMapLibreGLStyleSetFilter::fromMapParameter(param);
        break;
    case 3: // layer
        changes << QMapLibreGLStyleAddLayer::fromMapParameter(param);
        break;
    case 4: // source
        changes << QMapLibreGLStyleAddSource::fromMapParameter(param);
        break;
    case 5: // image
        changes << QMapLibreGLStyleAddImage::fromMapParameter(param);
        break;
    }

    return changes;
}

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

QList<QSharedPointer<QMapLibreGLStyleChange>> QMapLibreGLStyleChange::removeMapParameter(QGeoMapParameter *param)
{
    static const QStringList acceptedParameterTypes = QStringList()
        << QStringLiteral("paint") << QStringLiteral("layout") << QStringLiteral("filter")
        << QStringLiteral("layer") << QStringLiteral("source") << QStringLiteral("image");

    QList<QSharedPointer<QMapLibreGLStyleChange>> changes;

    switch (acceptedParameterTypes.indexOf(param->type())) {
    case -1:
        qWarning() << "Invalid value for property 'type': " + param->type();
        break;
    case 0: // paint
    case 1: // layout
    case 2: // filter
        break;
    case 3: // layer
        changes << QSharedPointer<QMapLibreGLStyleChange>(new QMapLibreGLStyleRemoveLayer(param->property("name").toString()));
        break;
    case 4: // source
        changes << QSharedPointer<QMapLibreGLStyleChange>(new QMapLibreGLStyleRemoveSource(param->property("name").toString()));
        break;
    case 5: // image
        break;
    }

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

QList<QSharedPointer<QMapLibreGLStyleChange>> QMapLibreGLStyleSetLayoutProperty::fromMapParameter(QGeoMapParameter *param)
{
    Q_ASSERT(param->type() == "layout");

    QList<QSharedPointer<QMapLibreGLStyleChange>> changes;

    QList<QByteArray> propertyNames = getAllPropertyNamesList(param);
    for (const QByteArray &propertyName : propertyNames) {
        if (isImmutableProperty(propertyName))
            continue;

        auto layout = new QMapLibreGLStyleSetLayoutProperty();

        layout->m_value = param->property(propertyName);
        if (layout->m_value.canConvert<QJSValue>()) {
            layout->m_value = layout->m_value.value<QJSValue>().toVariant();
        }

        layout->m_layer = param->property("layer").toString();
        layout->m_property = formatPropertyName(propertyName);

        changes << QSharedPointer<QMapLibreGLStyleChange>(layout);
    }

    return changes;
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

QList<QSharedPointer<QMapLibreGLStyleChange>> QMapLibreGLStyleSetPaintProperty::fromMapParameter(QGeoMapParameter *param)
{
    Q_ASSERT(param->type() == "paint");

    QList<QSharedPointer<QMapLibreGLStyleChange>> changes;

    QList<QByteArray> propertyNames = getAllPropertyNamesList(param);
    for (const QByteArray &propertyName : propertyNames) {
        if (isImmutableProperty(propertyName))
            continue;

        auto paint = new QMapLibreGLStyleSetPaintProperty();

        paint->m_value = param->property(propertyName);
        if (paint->m_value.canConvert<QJSValue>()) {
            paint->m_value = paint->m_value.value<QJSValue>().toVariant();
        }

        paint->m_layer = param->property("layer").toString();
        paint->m_property = formatPropertyName(propertyName);

        changes << QSharedPointer<QMapLibreGLStyleChange>(paint);
    }

    return changes;
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

QSharedPointer<QMapLibreGLStyleChange> QMapLibreGLStyleAddLayer::fromMapParameter(QGeoMapParameter *param)
{
    Q_ASSERT(param->type() == "layer");

    auto layer = new QMapLibreGLStyleAddLayer();

    static const QStringList layerProperties = QStringList()
        << QStringLiteral("name") << QStringLiteral("layerType") << QStringLiteral("before");

    QList<QByteArray> propertyNames = getAllPropertyNamesList(param);
    for (const QByteArray &propertyName : propertyNames) {
        if (isImmutableProperty(propertyName))
            continue;

        const QVariant value = param->property(propertyName);

        switch (layerProperties.indexOf(propertyName)) {
        case -1:
            layer->m_params[formatPropertyName(propertyName)] = value;
            break;
        case 0: // name
            layer->m_params[QStringLiteral("id")] = value;
            break;
        case 1: // layerType
            layer->m_params[QStringLiteral("type")] = value;
            break;
        case 2: // before
            layer->m_before = value.toString();
            break;
        }
    }

    return QSharedPointer<QMapLibreGLStyleChange>(layer);
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

QSharedPointer<QMapLibreGLStyleChange> QMapLibreGLStyleAddSource::fromMapParameter(QGeoMapParameter *param)
{
    Q_ASSERT(param->type() == "source");

    static const QStringList acceptedSourceTypes = QStringList()
        << QStringLiteral("vector") << QStringLiteral("raster") << QStringLiteral("raster-dem") << QStringLiteral("geojson") << QStringLiteral("image");

    QString sourceType = param->property("sourceType").toString();

    auto source = new QMapLibreGLStyleAddSource();
    source->m_id = param->property("name").toString();
    source->m_params[QStringLiteral("type")] = sourceType;

    switch (acceptedSourceTypes.indexOf(sourceType)) {
    case -1:
        qWarning() << "Invalid value for property 'sourceType': " + sourceType;
        break;
    case 0: // vector
    case 1: // raster
    case 2: // raster-dem
        if (param->hasProperty("url")) {
            source->m_params[QStringLiteral("url")] = param->property("url");
        }
        if (param->hasProperty("tiles")) {
            source->m_params[QStringLiteral("tiles")] = param->property("tiles");
        }
        if (param->hasProperty("tileSize")) {
            source->m_params[QStringLiteral("tileSize")] = param->property("tileSize");
        }
        break;
    case 3: { // geojson
        auto data = param->property("data").toString();
        if (data.startsWith(':')) {
            QFile geojson(data);
            geojson.open(QIODevice::ReadOnly);
            source->m_params[QStringLiteral("data")] = geojson.readAll();
        } else {
            source->m_params[QStringLiteral("data")] = data.toUtf8();
        }
    } break;
    case 4: { // image
        source->m_params[QStringLiteral("url")] = param->property("url");
        source->m_params[QStringLiteral("coordinates")] = param->property("coordinates");
    } break;
    }

    return QSharedPointer<QMapLibreGLStyleChange>(source);
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

QSharedPointer<QMapLibreGLStyleChange> QMapLibreGLStyleSetFilter::fromMapParameter(QGeoMapParameter *param)
{
    Q_ASSERT(param->type() == "filter");

    auto filter = new QMapLibreGLStyleSetFilter();
    filter->m_layer = param->property("layer").toString();
    filter->m_filter = param->property("filter");

    return QSharedPointer<QMapLibreGLStyleChange>(filter);
}


// QMapLibreGLStyleAddImage

void QMapLibreGLStyleAddImage::apply(QMapLibreGL::Map *map)
{
    map->addImage(m_name, m_sprite);
}

QSharedPointer<QMapLibreGLStyleChange> QMapLibreGLStyleAddImage::fromMapParameter(QGeoMapParameter *param)
{
    Q_ASSERT(param->type() == "image");

    auto image = new QMapLibreGLStyleAddImage();
    image->m_name = param->property("name").toString();
    image->m_sprite = QImage(param->property("sprite").toString());

    return QSharedPointer<QMapLibreGLStyleChange>(image);
}
