// Copyright (C) 2023 MapLibre contributors
// Copyright (C) 2019 Mapbox, Inc.

// SPDX-License-Identifier: BSD-2-Clause

#ifndef QMAPLIBRE_SETTINGS_H
#define QMAPLIBRE_SETTINGS_H

#include <QMapLibre/Export>

#include <QtCore/QString>
#include <QtGui/QImage>

#include <functional>

// TODO: this will be wrapped at some point
namespace mbgl {
class TileServerOptions;
}

namespace QMapLibre {

class Q_MAPLIBRE_CORE_EXPORT Settings {
public:
    Settings();

    enum GLContextMode {
        UniqueGLContext = 0,
        SharedGLContext
    };

    enum MapMode {
        Continuous = 0,
        Static
    };

    enum ConstrainMode {
        NoConstrain = 0,
        ConstrainHeightOnly,
        ConstrainWidthAndHeight
    };

    enum ViewportMode {
        DefaultViewport = 0,
        FlippedYViewport
    };

    enum SettingsTemplate {
        DefaultSettings = 0,
        MapLibreSettings,
        MapTilerSettings,
        MapboxSettings
    };

    GLContextMode contextMode() const;
    void setContextMode(GLContextMode);

    MapMode mapMode() const;
    void setMapMode(MapMode);

    ConstrainMode constrainMode() const;
    void setConstrainMode(ConstrainMode);

    ViewportMode viewportMode() const;
    void setViewportMode(ViewportMode);

    unsigned cacheDatabaseMaximumSize() const;
    void setCacheDatabaseMaximumSize(unsigned);

    QString cacheDatabasePath() const;
    void setCacheDatabasePath(const QString &);

    QString assetPath() const;
    void setAssetPath(const QString &);

    QString apiKey() const;
    void setApiKey(const QString &);

    QString apiBaseUrl() const;
    void setApiBaseUrl(const QString &);

    QString localFontFamily() const;
    void setLocalFontFamily(const QString &);

    QString clientName() const;
    void setClientName(const QString &);

    QString clientVersion() const;
    void setClientVersion(const QString &);

    std::function<std::string(const std::string &)> resourceTransform() const;
    void setResourceTransform(const std::function<std::string(const std::string &)> &);

    void resetToTemplate(SettingsTemplate);

    QVector<QPair<QString, QString>> defaultStyles() const;

    mbgl::TileServerOptions *tileServerOptionsInternal() const;

private:
    GLContextMode m_contextMode;
    MapMode m_mapMode;
    ConstrainMode m_constrainMode;
    ViewportMode m_viewportMode;

    unsigned m_cacheMaximumSize;
    QString m_cacheDatabasePath;
    QString m_assetPath;
    QString m_apiKey;
    QString m_localFontFamily;
    QString m_clientName;
    QString m_clientVersion;
    std::function<std::string(const std::string &)> m_resourceTransform;

    mbgl::TileServerOptions *m_tileServerOptionsInternal{};
};

} // namespace QMapLibre

#endif // QMAPLIBRE_SETTINGS_H
