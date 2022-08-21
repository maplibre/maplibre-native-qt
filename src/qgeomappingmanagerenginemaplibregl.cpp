// Copyright (C) 2022 MapLibre contributors
// Copyright (C) 2017 The Qt Company Ltd.
// Copyright (C) 2017 Mapbox, Inc.

// SPDX-License-Identifier: LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgeomappingmanagerenginemaplibregl.h"
#include "qgeomapmaplibregl.h"

#include <QtCore/qstandardpaths.h>
#include <QtLocation/private/qabstractgeotilecache_p.h>
#include <QtLocation/private/qgeocameracapabilities_p.h>
#include <QtLocation/private/qgeomaptype_p.h>

#include <QDir>

QT_BEGIN_NAMESPACE

QGeoMappingManagerEngineMapLibreGL::QGeoMappingManagerEngineMapLibreGL(const QVariantMap &parameters, QGeoServiceProvider::Error *error, QString *errorString)
:   QGeoMappingManagerEngine()
{
    *error = QGeoServiceProvider::NoError;
    errorString->clear();

    QGeoCameraCapabilities cameraCaps;
    cameraCaps.setMinimumZoomLevel(0.0);
    cameraCaps.setMaximumZoomLevel(20.0);
    cameraCaps.setTileSize(512);
    cameraCaps.setSupportsBearing(true);
    cameraCaps.setSupportsTilting(true);
    cameraCaps.setMinimumTilt(0);
    cameraCaps.setMaximumTilt(60);
    cameraCaps.setMinimumFieldOfView(36.87);
    cameraCaps.setMaximumFieldOfView(36.87);
    setCameraCapabilities(cameraCaps);

    QList<QGeoMapType> mapTypes;
    int mapId = 0;
    const QByteArray pluginName = "maplibregl";

    if (parameters.contains(QStringLiteral("maplibregl.settings_template"))) {
        auto settings_template = parameters.value(QStringLiteral("maplibregl.settings_template")).toString();
        if (settings_template == "maptiler"){
            m_settings.resetToTemplate(QMapLibreGL::Settings::MapTilerSettings);
        }else if (settings_template == "mapbox"){
            m_settings.resetToTemplate(QMapLibreGL::Settings::MapboxSettings);
        }
    }

    if (parameters.contains(QStringLiteral("maplibregl.api_base_url"))) {
        const QString apiBaseUrl = parameters.value(QStringLiteral("maplibregl.api_base_url")).toString();
        m_settings.setApiBaseUrl(apiBaseUrl);
    }

    QVariantMap metadata;
    metadata["isHTTPS"] = true;

    for (auto &style: m_settings.defaultStyles()){
        mapTypes << QGeoMapType(QGeoMapType::StreetMap, style.first,
                style.second, false, false, ++mapId, pluginName, cameraCaps, metadata);
    }

    if (parameters.contains(QStringLiteral("maplibregl.mapping.additional_style_urls"))) {
        const QString ids = parameters.value(QStringLiteral("maplibregl.mapping.additional_style_urls")).toString();
        const QStringList idList = ids.split(',', Qt::SkipEmptyParts);

        for (auto it = idList.crbegin(), end = idList.crend(); it != end; ++it) {
            if ((*it).isEmpty())
                continue;
            if ((*it).startsWith(QStringLiteral("http:")))
                metadata["isHTTPS"] = false;
            else
                metadata["isHTTPS"] = true;

            mapTypes.prepend(QGeoMapType(QGeoMapType::CustomMap, *it,
                    tr("User provided style"), false, false, ++mapId, pluginName, cameraCaps, metadata));
        }
    }

    setSupportedMapTypes(mapTypes);

    if (parameters.contains(QStringLiteral("maplibregl.access_token"))) {
        m_settings.setApiKey(parameters.value(QStringLiteral("maplibregl.access_token")).toString());
    }

    bool memoryCache = false;
    if (parameters.contains(QStringLiteral("maplibregl.mapping.cache.memory"))) {
        memoryCache = parameters.value(QStringLiteral("maplibregl.mapping.cache.memory")).toBool();
        m_settings.setCacheDatabasePath(QStringLiteral(":memory:"));
    }

    QString cacheDirectory;
    if (parameters.contains(QStringLiteral("maplibregl.mapping.cache.directory"))) {
        cacheDirectory = parameters.value(QStringLiteral("maplibregl.mapping.cache.directory")).toString();
    } else {
        cacheDirectory = QAbstractGeoTileCache::baseLocationCacheDirectory() + QStringLiteral("maplibregl/");
    }

    if (!memoryCache && QDir::root().mkpath(cacheDirectory)) {
        m_settings.setCacheDatabasePath(cacheDirectory + "/maplibregl.db");
    }

    if (parameters.contains(QStringLiteral("maplibregl.mapping.cache.size"))) {
        bool ok = false;
        int cacheSize = parameters.value(QStringLiteral("maplibregl.mapping.cache.size")).toString().toInt(&ok);

        if (ok)
            m_settings.setCacheDatabaseMaximumSize(cacheSize);
    }

    if (parameters.contains(QStringLiteral("maplibregl.mapping.use_fbo"))) {
        m_useFBO = parameters.value(QStringLiteral("maplibregl.mapping.use_fbo")).toBool();
    }

    if (parameters.contains(QStringLiteral("maplibregl.mapping.items.insert_before"))) {
        m_mapItemsBefore = parameters.value(QStringLiteral("maplibregl.mapping.items.insert_before")).toString();
    }

    if (parameters.contains(QStringLiteral("maplibregl.client.name"))) {
        m_settings.setClientName(parameters.value(QStringLiteral("maplibregl.client.name")).toString());
    }

    if (parameters.contains(QStringLiteral("maplibregl.client.version"))) {
        m_settings.setClientVersion(parameters.value(QStringLiteral("maplibregl.client.version")).toString());
    }

    engineInitialized();
}

QGeoMappingManagerEngineMapLibreGL::~QGeoMappingManagerEngineMapLibreGL()
{
}

QGeoMap *QGeoMappingManagerEngineMapLibreGL::createMap()
{
    QGeoMapMapLibreGL* map = new QGeoMapMapLibreGL(this, 0);
    map->setMapLibreGLSettings(m_settings);
    map->setUseFBO(m_useFBO);
    map->setMapItemsBefore(m_mapItemsBefore);

    return map;
}

QT_END_NAMESPACE
