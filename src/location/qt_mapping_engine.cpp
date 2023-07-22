// Copyright (C) 2023 MapLibre contributors
// Copyright (C) 2017 The Qt Company Ltd.
// Copyright (C) 2017 Mapbox, Inc.

// SPDX-License-Identifier: LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qt_mapping_engine.hpp"
#include "qgeomap.hpp"

#include <QtCore/qstandardpaths.h>
#include <QDir>
#include <QtLocation/private/qabstractgeotilecache_p.h>
#include <QtLocation/private/qgeocameracapabilities_p.h>
#include <QtLocation/private/qgeomaptype_p.h>

namespace QMapLibre {

QtMappingEngine::QtMappingEngine(const QVariantMap &parameters, QGeoServiceProvider::Error *error, QString *errorString)
    : QGeoMappingManagerEngine() {
    *error = QGeoServiceProvider::NoError;
    errorString->clear();

    // capabilities
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

    // API settings
    if (parameters.contains(QStringLiteral("maplibre.api.provider"))) {
        const QString api_provider = parameters.value(QStringLiteral("maplibre.api.provider")).toString();
        if (api_provider == "maptiler") {
            m_settings.resetToTemplate(Settings::MapTilerSettings);
        } else if (api_provider == "mapbox") {
            m_settings.resetToTemplate(Settings::MapboxSettings);
        } else {
            qWarning() << "Unknown API provider" << api_provider;
        }
    }

    if (parameters.contains(QStringLiteral("maplibre.api.base_url"))) {
        m_settings.setApiBaseUrl(parameters.value(QStringLiteral("maplibre.api.base_url")).toString());
    }

    if (parameters.contains(QStringLiteral("maplibre.api.key"))) {
        m_settings.setApiKey(parameters.value(QStringLiteral("maplibre.api.key")).toString());
    }

    // map styles
    const QByteArray pluginName = "maplibre";
    QList<QGeoMapType> mapTypes;
    int mapId{};
    QVariantMap mapMetadata;
    mapMetadata["isHTTPS"] = true;

    if (parameters.contains(QStringLiteral("maplibre.map.styles"))) {
        const QString ids = parameters.value(QStringLiteral("maplibre.map.styles")).toString();
        const QStringList idList = ids.split(',', Qt::SkipEmptyParts);

        for (const QString &url : idList) {
            if (url.isEmpty()) continue;

            mapMetadata["isHTTPS"] = url.startsWith(QStringLiteral("http:"));

            mapTypes << QGeoMapType(QGeoMapType::CustomMap,
                                    url,
                                    tr("User provided style"),
                                    false,
                                    false,
                                    ++mapId,
                                    pluginName,
                                    cameraCaps,
                                    mapMetadata);
        }
    }

    for (const QPair<QString, QString> &style : m_settings.defaultStyles()) {
        mapTypes << QGeoMapType(QGeoMapType::StreetMap,
                                style.first,
                                style.second,
                                false,
                                false,
                                ++mapId,
                                pluginName,
                                cameraCaps,
                                mapMetadata);
    }

    setSupportedMapTypes(mapTypes);

    // cache
    bool memoryCache{};
    if (parameters.contains(QStringLiteral("maplibre.cache.memory"))) {
        memoryCache = parameters.value(QStringLiteral("maplibre.cache.memory")).toBool();
        m_settings.setCacheDatabasePath(QStringLiteral(":memory:"));
    }

    QString cacheDirectory;
    if (parameters.contains(QStringLiteral("maplibre.cache.directory"))) {
        cacheDirectory = parameters.value(QStringLiteral("maplibre.cache.directory")).toString();
    } else {
        cacheDirectory = QAbstractGeoTileCache::baseLocationCacheDirectory() + QStringLiteral("maplibre");
    }

    if (!memoryCache && QDir::root().mkpath(cacheDirectory)) {
        m_settings.setCacheDatabasePath(cacheDirectory + "/maplibre.db");
    }

    if (parameters.contains(QStringLiteral("maplibre.cache.size"))) {
        bool ok{};
        const QString cacheSizeString = parameters.value(QStringLiteral("maplibre.cache.size")).toString();
        const int cacheSize = cacheSizeString.toInt(&ok);

        if (ok) {
            m_settings.setCacheDatabaseMaximumSize(cacheSize);
        } else {
            qWarning() << "Invalid cache size" << cacheSizeString;
        }
    }

    // rendering
    if (parameters.contains(QStringLiteral("maplibre.items.insert_before"))) {
        m_mapItemsBefore = parameters.value(QStringLiteral("maplibre.items.insert_before")).toString();
    }

    // client
    if (parameters.contains(QStringLiteral("maplibre.client.name"))) {
        m_settings.setClientName(parameters.value(QStringLiteral("maplibre.client.name")).toString());
    }

    if (parameters.contains(QStringLiteral("maplibre.client.version"))) {
        m_settings.setClientVersion(parameters.value(QStringLiteral("maplibre.client.version")).toString());
    }

    engineInitialized();
}

QGeoMap *QtMappingEngine::createMap() {
    QGeoMapMapLibre *map = new QGeoMapMapLibre(this);
    map->setSettings(m_settings);
    map->setMapItemsBefore(m_mapItemsBefore);

    return map;
}

} // namespace QMapLibre
