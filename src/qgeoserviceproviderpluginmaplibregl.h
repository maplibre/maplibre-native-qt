// Copyright (C) 2023 MapLibre contributors
// Copyright (C) 2017 The Qt Company Ltd.
// Copyright (C) 2017 Mapbox, Inc.

// SPDX-License-Identifier: LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGEOSERVICEPROVIDER_MAPLIBREGL_H
#define QGEOSERVICEPROVIDER_MAPLIBREGL_H

#include <QtCore/QObject>
#include <QtLocation/QGeoServiceProviderFactory>

QT_BEGIN_NAMESPACE

class QGeoServiceProviderFactoryMapLibreGL: public QObject, public QGeoServiceProviderFactory
{
    Q_OBJECT
    Q_INTERFACES(QGeoServiceProviderFactory)
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    Q_PLUGIN_METADATA(IID "org.qt-project.qt.geoservice.serviceproviderfactory/6.0"
                      FILE "maplibregl_plugin.json")
#else
    Q_PLUGIN_METADATA(IID "org.qt-project.qt.geoservice.serviceproviderfactory/5.0"
                      FILE "maplibregl_plugin.json")
#endif

public:
    QGeoServiceProviderFactoryMapLibreGL();

    QGeoCodingManagerEngine *createGeocodingManagerEngine(const QVariantMap &parameters,
                                                          QGeoServiceProvider::Error *error,
                                                          QString *errorString) const override;
    QGeoMappingManagerEngine *createMappingManagerEngine(const QVariantMap &parameters,
                                                         QGeoServiceProvider::Error *error,
                                                         QString *errorString) const override;
    QGeoRoutingManagerEngine *createRoutingManagerEngine(const QVariantMap &parameters,
                                                         QGeoServiceProvider::Error *error,
                                                         QString *errorString) const override;
    QPlaceManagerEngine *createPlaceManagerEngine(const QVariantMap &parameters,
                                                  QGeoServiceProvider::Error *error,
                                                  QString *errorString) const override;
};

QT_END_NAMESPACE

#endif
