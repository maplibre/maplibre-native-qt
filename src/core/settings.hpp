// Copyright (C) 2023 MapLibre contributors
// Copyright (C) 2019 Mapbox, Inc.

// SPDX-License-Identifier: BSD-2-Clause

#ifndef QMAPLIBRE_SETTINGS_H
#define QMAPLIBRE_SETTINGS_H

#include <QMapLibre/Export>
#include <QMapLibre/Types>

#include <QtCore/QString>
#include <QtGui/QImage>

#include <functional>

// TODO: this will be wrapped at some point
namespace mbgl {
class TileServerOptions;
}

namespace QMapLibre {

class SettingsPrivate;

class Q_MAPLIBRE_CORE_EXPORT Settings {
public:
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

    enum ProviderTemplate {
        NoProvider = 0,
        MapLibreProvider,
        MapTilerProvider,
        MapboxProvider
    };

    explicit Settings(ProviderTemplate provider = NoProvider);
    ~Settings();
    Settings(const Settings &);
    Settings(Settings &&) noexcept;
    Settings &operator=(const Settings &);
    Settings &operator=(Settings &&) noexcept;

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

    void setProviderTemplate(ProviderTemplate);
    void setStyles(const Styles &styles);

    const Styles &styles() const;
    Styles providerStyles() const;

    Coordinate defaultCoordinate() const;
    void setDefaultCoordinate(const Coordinate &);
    double defaultZoom() const;
    void setDefaultZoom(double);

    mbgl::TileServerOptions *tileServerOptions() const;

private:
    SettingsPrivate *d_ptr;
};

} // namespace QMapLibre

#endif // QMAPLIBRE_SETTINGS_H
