// Copyright (C) 2023 MapLibre contributors
// Copyright (C) 2019 Mapbox, Inc.

// SPDX-License-Identifier: BSD-2-Clause

#include "utils.hpp"

#include <mln/storage/network_status.hpp>
#include <mln/util/geometry.hpp>
#include <mln/util/projection.hpp>
#include <mln/util/traits.hpp>

/*!
    \defgroup QMapLibre QMapLibre Core
    \brief Core types and utilities used throughout MapLibre Qt bindings.
*/

// mln::NetworkStatus::Status
static_assert(mln::underlying_type(QMapLibre::Online) == mln::underlying_type(mln::NetworkStatus::Status::Online),
              "error");
static_assert(mln::underlying_type(QMapLibre::Offline) == mln::underlying_type(mln::NetworkStatus::Status::Offline),
              "error");

namespace QMapLibre {

/*!
    \enum NetworkMode

    This enum represents whether server requests can be performed via network.
*/
/*!
    \var Online
    Server network requests are accessible.
*/
/*!
    \var Offline
    Only requests to the local cache are accessible.
*/

/*!
    \enum RendererType

    This enum represents possible renderer types used by MapLibre Qt bindings.
*/
/*!
    \var OpenGL
    OpenGL renderer type.
*/
/*!
    \var Vulkan
    Vulkan renderer type.
*/
/*!
    \var Metal
    Metal renderer type.
*/

/*!
    Returns the \ref RendererType the library was built with.
*/
RendererType supportedRendererType() {
#ifdef MLN_RENDER_BACKEND_VULKAN
    return RendererType::Vulkan;
#elif defined(MLN_RENDER_BACKEND_METAL)
    return RendererType::Metal;
#else
    return RendererType::OpenGL;
#endif
}

/*!
    Returns the current \ref NetworkMode.
*/
NetworkMode networkMode() {
    return static_cast<NetworkMode>(mln::NetworkStatus::Get());
}

/*!
    Forwards the network status \a mode to MapLibre Native engine.

    File source requests uses the available network when \a mode is set
    to \ref Online, otherwise scoped to the local cache.
*/
void setNetworkMode(NetworkMode mode) {
    mln::NetworkStatus::Set(static_cast<mln::NetworkStatus::Status>(mode));
}

/*!
    Returns the amount of meters per pixel from a given \a latitude and \a zoom.
*/
double metersPerPixelAtLatitude(double latitude, double zoom) {
    return mln::Projection::getMetersPerPixelAtLatitude(latitude, zoom);
}

/*!
    Returns the projected meters for a given \a coordinate object.
*/
ProjectedMeters projectedMetersForCoordinate(const Coordinate &coordinate) {
    auto projectedMeters = mln::Projection::projectedMetersForLatLng(
        mln::LatLng{coordinate.first, coordinate.second});
    return {projectedMeters.northing(), projectedMeters.easting()};
}

/*!
    Returns the \ref Coordinate for a given \a projectedMeters object.
*/
Coordinate coordinateForProjectedMeters(const ProjectedMeters &projectedMeters) {
    auto latLng = mln::Projection::latLngForProjectedMeters(
        mln::ProjectedMeters{projectedMeters.first, projectedMeters.second});
    return {latLng.latitude(), latLng.longitude()};
}

} // namespace QMapLibre
