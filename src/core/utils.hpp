// Copyright (C) 2023 MapLibre contributors
// Copyright (C) 2019 Mapbox, Inc.

// SPDX-License-Identifier: BSD-2-Clause

#ifndef QMAPLIBRE_UTILS_H
#define QMAPLIBRE_UTILS_H

#include "export.hpp"
#include "types.hpp"

// This header follows the Qt coding style: https://wiki.qt.io/Qt_Coding_Style

namespace QMapLibre {

enum NetworkMode {
    Online, // Default
    Offline,
};

Q_MAPLIBRE_EXPORT NetworkMode networkMode();
Q_MAPLIBRE_EXPORT void setNetworkMode(NetworkMode);

Q_MAPLIBRE_EXPORT double metersPerPixelAtLatitude(double latitude, double zoom);
Q_MAPLIBRE_EXPORT ProjectedMeters projectedMetersForCoordinate(const Coordinate &);
Q_MAPLIBRE_EXPORT Coordinate coordinateForProjectedMeters(const ProjectedMeters &);

} // namespace QMapLibre

#endif // QMAPLIBRE_UTILS_H
