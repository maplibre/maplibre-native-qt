// Copyright (C) 2022 MapLibre contributors
// Copyright (C) 2017 The Qt Company Ltd.
// Copyright (C) 2017 Mapbox, Inc.

// SPDX-License-Identifier: LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgeoserviceproviderpluginmaplibregl.h"
#include "qgeomappingmanagerenginemaplibregl.h"

#include <QtGui/QOpenGLContext>

QT_BEGIN_NAMESPACE

QGeoServiceProviderFactoryMapLibreGL::QGeoServiceProviderFactoryMapLibreGL()
{
}

QGeoCodingManagerEngine *QGeoServiceProviderFactoryMapLibreGL::createGeocodingManagerEngine(
    const QVariantMap &parameters, QGeoServiceProvider::Error *error, QString *errorString) const
{
    Q_UNUSED(parameters);
    Q_UNUSED(error);
    Q_UNUSED(errorString);

    return nullptr;
}

QGeoMappingManagerEngine *QGeoServiceProviderFactoryMapLibreGL::createMappingManagerEngine(
    const QVariantMap &parameters, QGeoServiceProvider::Error *error, QString *errorString) const
{
    return new QGeoMappingManagerEngineMapLibreGL(parameters, error, errorString);
}

QGeoRoutingManagerEngine *QGeoServiceProviderFactoryMapLibreGL::createRoutingManagerEngine(
    const QVariantMap &parameters, QGeoServiceProvider::Error *error, QString *errorString) const
{
    Q_UNUSED(parameters);
    Q_UNUSED(error);
    Q_UNUSED(errorString);

    return nullptr;
}

QPlaceManagerEngine *QGeoServiceProviderFactoryMapLibreGL::createPlaceManagerEngine(
    const QVariantMap &parameters, QGeoServiceProvider::Error *error, QString *errorString) const
{
    Q_UNUSED(parameters);
    Q_UNUSED(error);
    Q_UNUSED(errorString);

    return nullptr;
}

QT_END_NAMESPACE
