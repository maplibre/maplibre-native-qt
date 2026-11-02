// Copyright (C) 2023 MapLibre contributors
// Copyright (C) 2019 Mapbox, Inc.

// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include "types.hpp"

#include <mapbox/geojson.hpp>
#include <mln/util/feature.hpp>
#include <mln/util/geometry.hpp>

#include <QtCore/QVariant>

#include <string>

namespace QMapLibre::GeoJSON {

mln::Point<double> asPoint(const Coordinate &coordinate);
mln::MultiPoint<double> asMultiPoint(const Coordinates &multiPoint);
mln::LineString<double> asLineString(const Coordinates &lineString);
mln::MultiLineString<double> asMultiLineString(const CoordinatesCollection &multiLineString);
mln::Polygon<double> asPolygon(const CoordinatesCollection &polygon);
mln::MultiPolygon<double> asMultiPolygon(const CoordinatesCollections &multiPolygon);
mln::Value asPropertyValue(const QVariant &value);
mln::FeatureIdentifier asFeatureIdentifier(const QVariant &id);
mln::GeoJSONFeature asFeature(const Feature &feature);

} // namespace QMapLibre::GeoJSON
