// Copyright (C) 2023 MapLibre contributors
// Copyright (C) 2019 Mapbox, Inc.

// SPDX-License-Identifier: BSD-2-Clause

#include "geojson_p.hpp"

#include <QtCore/QDebug>

namespace QMapLibre::GeoJSON {

mln::Point<double> asPoint(const Coordinate &coordinate) {
    return mln::Point<double>{coordinate.second, coordinate.first};
}

mln::MultiPoint<double> asMultiPoint(const Coordinates &multiPoint) {
    mln::MultiPoint<double> mbglMultiPoint;
    mbglMultiPoint.reserve(multiPoint.size());
    for (const auto &point : multiPoint) {
        mbglMultiPoint.emplace_back(asPoint(point));
    }
    return mbglMultiPoint;
};

mln::LineString<double> asLineString(const Coordinates &lineString) {
    mln::LineString<double> mbglLineString;
    mbglLineString.reserve(lineString.size());
    for (const auto &coordinate : lineString) {
        mbglLineString.emplace_back(asPoint(coordinate));
    }
    return mbglLineString;
};

mln::MultiLineString<double> asMultiLineString(const CoordinatesCollection &multiLineString) {
    mln::MultiLineString<double> mbglMultiLineString;
    mbglMultiLineString.reserve(multiLineString.size());
    for (const auto &lineString : multiLineString) {
        mbglMultiLineString.emplace_back(std::forward<mln::LineString<double>>(asLineString(lineString)));
    }
    return mbglMultiLineString;
};

mln::Polygon<double> asPolygon(const CoordinatesCollection &polygon) {
    mln::Polygon<double> mbglPolygon;
    mbglPolygon.reserve(polygon.size());
    for (const auto &linearRing : polygon) {
        mln::LinearRing<double> mbglLinearRing;
        mbglLinearRing.reserve(linearRing.size());
        for (const auto &coordinate : linearRing) {
            mbglLinearRing.emplace_back(asPoint(coordinate));
        }
        mbglPolygon.emplace_back(std::move(mbglLinearRing));
    }
    return mbglPolygon;
};

mln::MultiPolygon<double> asMultiPolygon(const CoordinatesCollections &multiPolygon) {
    mln::MultiPolygon<double> mbglMultiPolygon;
    mbglMultiPolygon.reserve(multiPolygon.size());
    for (const auto &polygon : multiPolygon) {
        mbglMultiPolygon.emplace_back(std::forward<mln::Polygon<double>>(asPolygon(polygon)));
    }
    return mbglMultiPolygon;
};

mln::Value asPropertyValue(const QVariant &value) {
    auto valueList = [](const QVariantList &list) {
        std::vector<mln::Value> mbglList;
        mbglList.reserve(list.size());
        for (const auto &listValue : list) {
            mbglList.emplace_back(asPropertyValue(listValue));
        }
        return mbglList;
    };

    auto valueMap = [](const QVariantMap &map) {
        std::unordered_map<std::string, mln::Value> mbglMap;
        mbglMap.reserve(map.size());
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            mbglMap.emplace(it.key().toStdString(), asPropertyValue(it.value()));
        }
        return mbglMap;
    };

    switch (value.typeId()) {
        case QMetaType::UnknownType:
            return mln::NullValue{};
        case QMetaType::Bool:
            return {value.toBool()};
        case QMetaType::ULongLong:
            return {static_cast<uint64_t>(value.toULongLong())};
        case QMetaType::LongLong:
            return {static_cast<int64_t>(value.toLongLong())};
        case QMetaType::Double:
            return {value.toDouble()};
        case QMetaType::QString:
            return {value.toString().toStdString()};
        case QMetaType::QVariantList:
            return valueList(value.toList());
        case QMetaType::QVariantMap:
            return valueMap(value.toMap());
        default:
            qWarning() << "Unsupported feature property value:" << value;
            return {};
    }
}

mln::FeatureIdentifier asFeatureIdentifier(const QVariant &id) {
    switch (id.typeId()) {
        case QMetaType::UnknownType:
            return {};
        case QMetaType::ULongLong:
            return {static_cast<uint64_t>(id.toULongLong())};
        case QMetaType::LongLong:
            return {static_cast<int64_t>(id.toLongLong())};
        case QMetaType::Double:
            return {id.toDouble()};
        case QMetaType::QString:
            return {id.toString().toStdString()};
        default:
            qWarning() << "Unsupported feature identifier:" << id;
            return {};
    }
}

mln::GeoJSONFeature asFeature(const Feature &feature) {
    mln::PropertyMap properties;
    properties.reserve(feature.properties.size());
    for (auto it = feature.properties.constBegin(); it != feature.properties.constEnd(); ++it) {
        properties.emplace(std::make_pair(it.key().toStdString(), asPropertyValue(it.value())));
    }

    mln::FeatureIdentifier id = asFeatureIdentifier(feature.id);

    if (feature.type == Feature::PointType) {
        const Coordinates &points = feature.geometry.first().first();
        if (points.size() == 1) {
            return {asPoint(points.first()), std::move(properties), std::move(id)};
        }
        return {asMultiPoint(points), std::move(properties), std::move(id)};
    }

    if (feature.type == Feature::LineStringType) {
        const CoordinatesCollection &lineStrings = feature.geometry.first();
        if (lineStrings.size() == 1) {
            return {asLineString(lineStrings.first()), std::move(properties), std::move(id)};
        }
        return {asMultiLineString(lineStrings), std::move(properties), std::move(id)};
    }

    if (feature.type == Feature::PolygonType) {
        const CoordinatesCollections &polygons = feature.geometry;
        if (polygons.size() == 1) {
            return {asPolygon(polygons.first()), std::move(properties), std::move(id)};
        }
        return {asMultiPolygon(polygons), std::move(properties), std::move(id)};
    }

    throw std::runtime_error("Unsupported feature type");
};

} // namespace QMapLibre::GeoJSON
