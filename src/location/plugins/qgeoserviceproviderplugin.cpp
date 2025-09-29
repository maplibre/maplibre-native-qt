// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#include "qgeoserviceproviderplugin.hpp"

#include "../qt_mapping_engine.hpp"

QGeoServiceProviderFactoryMapLibre::QGeoServiceProviderFactoryMapLibre() = default;

QGeoCodingManagerEngine* QGeoServiceProviderFactoryMapLibre::createGeocodingManagerEngine(
    const QVariantMap& parameters, QGeoServiceProvider::Error* error, QString* errorString) const {
    Q_UNUSED(parameters);
    Q_UNUSED(error);
    Q_UNUSED(errorString);

    return nullptr;
}

QGeoMappingManagerEngine* QGeoServiceProviderFactoryMapLibre::createMappingManagerEngine(
    const QVariantMap& parameters, QGeoServiceProvider::Error* error, QString* errorString) const {
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    return new QMapLibre::QtMappingEngine(parameters, error, errorString);
}

QGeoRoutingManagerEngine* QGeoServiceProviderFactoryMapLibre::createRoutingManagerEngine(
    const QVariantMap& parameters, QGeoServiceProvider::Error* error, QString* errorString) const {
    Q_UNUSED(parameters);
    Q_UNUSED(error);
    Q_UNUSED(errorString);

    return nullptr;
}

QPlaceManagerEngine* QGeoServiceProviderFactoryMapLibre::createPlaceManagerEngine(const QVariantMap& parameters,
                                                                                  QGeoServiceProvider::Error* error,
                                                                                  QString* errorString) const {
    Q_UNUSED(parameters);
    Q_UNUSED(error);
    Q_UNUSED(errorString);

    return nullptr;
}
