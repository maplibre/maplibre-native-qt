// Copyright (C) 2023 MapLibre contributors
// Copyright (C) 2020 Mapbox, Inc.

// SPDX-License-Identifier: BSD-2-Clause

#include "settings.hpp"
#include "settings_p.hpp"

#include <mbgl/gfx/renderer_backend.hpp>
#include <mbgl/map/mode.hpp>
#include <mbgl/util/constants.hpp>
#include <mbgl/util/traits.hpp>

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4805)
#endif

// mbgl::GLContextMode
static_assert(mbgl::underlying_type(QMapLibre::Settings::UniqueGLContext) ==
                  mbgl::underlying_type(mbgl::gfx::ContextMode::Unique),
              "error");
static_assert(mbgl::underlying_type(QMapLibre::Settings::SharedGLContext) ==
                  mbgl::underlying_type(mbgl::gfx::ContextMode::Shared),
              "error");

// mbgl::MapMode
static_assert(mbgl::underlying_type(QMapLibre::Settings::Continuous) ==
                  mbgl::underlying_type(mbgl::MapMode::Continuous),
              "error");
static_assert(mbgl::underlying_type(QMapLibre::Settings::Static) == mbgl::underlying_type(mbgl::MapMode::Static),
              "error");

// mbgl::ConstrainMode
static_assert(mbgl::underlying_type(QMapLibre::Settings::NoConstrain) ==
                  mbgl::underlying_type(mbgl::ConstrainMode::None),
              "error");
static_assert(mbgl::underlying_type(QMapLibre::Settings::ConstrainHeightOnly) ==
                  mbgl::underlying_type(mbgl::ConstrainMode::HeightOnly),
              "error");
static_assert(mbgl::underlying_type(QMapLibre::Settings::ConstrainWidthAndHeight) ==
                  mbgl::underlying_type(mbgl::ConstrainMode::WidthAndHeight),
              "error");

// mbgl::ViewportMode
static_assert(mbgl::underlying_type(QMapLibre::Settings::DefaultViewport) ==
                  mbgl::underlying_type(mbgl::ViewportMode::Default),
              "error");
static_assert(mbgl::underlying_type(QMapLibre::Settings::FlippedYViewport) ==
                  mbgl::underlying_type(mbgl::ViewportMode::FlippedY),
              "error");

#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace QMapLibre {

/*!
    \class QMapLibre::Settings
    \brief The Settings class stores the initial configuration for QMapLibre::Map.

    \inmodule MapLibre Maps SDK for Qt

    Settings is used to configure QMapLibre::Map at the moment of its creation.
    Once created, the Settings of a QMapLibre::Map can no longer be changed.

    Cache-related settings are shared between all QMapLibre::Map instances using the same cache path.
    The first map to configure cache properties such as size will force the configuration
    to all newly instantiated QMapLibre::Map objects using the same cache in the same process.
*/

/*!
    \enum QMapLibre::Settings::GLContextMode

    This enum sets the expectations for the OpenGL state.

    \value UniqueGLContext  The OpenGL context is only used by QMapLibre::Map, so it is not
    reset before each rendering. Use this mode if the intention is to only draw a
    fullscreen map.

    \value SharedGLContext  The OpenGL context is shared and the state will be
    marked dirty - which invalidates any previously assumed GL state. The
    embedder is responsible for clearing up the viewport prior to calling
    QMapLibre::Map::render. The embedder is also responsible for resetting its own
    GL state after QMapLibre::Map::render has finished, if needed.

    \sa contextMode()
*/

/*!
    \enum QMapLibre::Settings::MapMode

    This enum sets the map rendering mode

    \value Continuous  The map will render as data arrives from the network and
    react immediately to state changes.

    This is the default mode and the preferred when the map is intended to be
    interactive.

    \value Static  The map will no longer react to state changes and will only
    be rendered when QMapLibre::Map::startStaticRender is called. After all the
    resources are loaded, the QMapLibre::Map::staticRenderFinished signal is emitted.

    This mode is useful for taking a snapshot of the finished rendering result
    of the map into a QImage.

    \sa mapMode()
*/

/*!
    \enum QMapLibre::Settings::ConstrainMode

    This enum determines if the map wraps.

    \value NoConstrain              The map will wrap on the horizontal axis. Since it doesn't
    make sense to wrap on the vertical axis in a Web Mercator projection, the map will scroll
    and show some empty space.

    \value ConstrainHeightOnly      The map will wrap around the horizontal axis, like a spinning
    globe. This is the recommended constrain mode.

    \value ConstrainWidthAndHeight  The map won't wrap and panning is restricted to the boundaries
    of the map.

    \sa constrainMode()
*/

/*!
    \enum QMapLibre::Settings::ViewportMode

    This enum flips the map vertically.

    \value DefaultViewport  Native orientation.

    \value FlippedYViewport  Mirrored vertically.

    \sa viewportMode()
*/

/*!
    Constructs a Settings object with the default values. The default
    configuration is not valid for initializing a \a QMapLibre::Map,
    but a provider template can be provided.
*/
Settings::Settings(ProviderTemplate provider)
    : d_ptr(std::make_unique<SettingsPrivate>()) {
    d_ptr->setProviderTemplate(provider);
}

Settings::~Settings() = default;

Settings::Settings(const Settings &s)
    : d_ptr(std::make_unique<SettingsPrivate>(*s.d_ptr)) {}

Settings::Settings(Settings &&s) noexcept = default;

Settings &Settings::operator=(const Settings &s) {
    d_ptr = std::make_unique<SettingsPrivate>(*s.d_ptr);
    return *this;
}

Settings &Settings::operator=(Settings &&s) noexcept = default;

/*!
    Returns the OpenGL context mode. This is specially important when mixing
    with other OpenGL draw calls.

    By default, it is set to Settings::SharedGLContext.
*/
Settings::GLContextMode Settings::contextMode() const {
    return d_ptr->m_contextMode;
}

/*!
    Sets the OpenGL context \a mode.
*/
void Settings::setContextMode(GLContextMode mode) {
    d_ptr->m_contextMode = mode;
}

/*!
    Returns the map mode. Static mode will emit a signal for
    rendering a map only when the map is fully loaded.
    Animations like style transitions and labels fading won't
    be seen.

    The Continuous mode will emit the signal for every new
    change on the map and it is usually what you expect for
    a interactive map.

    By default, it is set to QMapLibre::Settings::Continuous.
*/
Settings::MapMode Settings::mapMode() const {
    return d_ptr->m_mapMode;
}

/*!
    Sets the map \a mode.
*/
void Settings::setMapMode(MapMode mode) {
    d_ptr->m_mapMode = mode;
}

/*!
    Returns the constrain mode. This is used to limit the map to wrap
    around the globe horizontally.

    By default, it is set to QMapLibre::Settings::ConstrainHeightOnly.
*/
Settings::ConstrainMode Settings::constrainMode() const {
    return d_ptr->m_constrainMode;
}

/*!
    Sets the map constrain \a mode.
*/
void Settings::setConstrainMode(ConstrainMode mode) {
    d_ptr->m_constrainMode = mode;
}

/*!
    Returns the viewport mode. This is used to flip the vertical
    orientation of the map as some devices may use inverted orientation.

    By default, it is set to QMapLibre::Settings::DefaultViewport.
*/
Settings::ViewportMode Settings::viewportMode() const {
    return d_ptr->m_viewportMode;
}

/*!
    Sets the viewport \a mode.
*/
void Settings::setViewportMode(ViewportMode mode) {
    d_ptr->m_viewportMode = mode;
}

/*!
    Returns the cache database maximum hard size in bytes. The database
    will grow until the limit is reached. Setting a maximum size smaller
    than the current size of an existing database results in undefined
    behavior

    By default, it is set to 50 MB.
*/
unsigned Settings::cacheDatabaseMaximumSize() const {
    return d_ptr->m_cacheMaximumSize;
}

/*!
    Returns the maximum allowed cache database \a size in bytes.
*/
void Settings::setCacheDatabaseMaximumSize(unsigned size) {
    d_ptr->m_cacheMaximumSize = size;
}

/*!
    Returns the cache database path. The cache is used for storing
    recently used resources like tiles and also an offline tile database
    pre-populated by the
    \l {https://github.com/maplibre/maplibre-native/blob/main/bin/offline.sh}
    {Offline Tool}.

    By default, it is set to \c :memory: meaning it will create an in-memory
    cache instead of a file on disk.
*/
QString Settings::cacheDatabasePath() const {
    return d_ptr->m_cacheDatabasePath;
}

/*!
    Sets the cache database \a path.

    Setting the \a path to \c :memory: will create an in-memory cache.
*/
void Settings::setCacheDatabasePath(const QString &path) {
    d_ptr->m_cacheDatabasePath = path;
}

/*!
    Returns the asset path, which is the root directory from where
    the \c asset:// scheme gets resolved in a style. \c asset:// can be used
    for loading a resource from the disk in a style rather than fetching
    it from the network.

    By default, it is set to the value returned by QCoreApplication::applicationDirPath().
*/
QString Settings::assetPath() const {
    return d_ptr->m_assetPath;
}

/*!
    Sets the asset \a path.
*/
void Settings::setAssetPath(const QString &path) {
    d_ptr->m_assetPath = path;
}

/*!
    Returns the API key.

    By default, it is taken from the environment variable \c MLN_API_KEY
    or empty if the variable is not set.
*/
QString Settings::apiKey() const {
    return d_ptr->m_apiKey;
}

/*!
    Sets the API key.

    MapTiler-hosted and Mapbox-hosted vector tiles and styles require an API
    key or access token.
*/
void Settings::setApiKey(const QString &key) {
    d_ptr->m_apiKey = key;
}

/*!
    Returns the API base URL.
*/
QString Settings::apiBaseUrl() const {
    if (!customTileServerOptions()) {
        return {};
    }

    return QString::fromStdString(tileServerOptions().baseURL());
}

/*!
    Sets the API base \a url.

    The API base URL is the URL that the \b "mapbox://" protocol will
    be resolved to. It defaults to "https://api.mapbox.com" but can be
    changed, for instance, to a tile cache server address.
*/
void Settings::setApiBaseUrl(const QString &url) {
    d_ptr->setProviderApiBaseUrl(url);
}

/*!
    Returns the local font family. Returns an empty string if no local font family is set.
*/
QString Settings::localFontFamily() const {
    return d_ptr->m_localFontFamily;
}

/*!
    Sets the local font family.

   Rendering Chinese/Japanese/Korean (CJK) ideographs and precomposed Hangul Syllables requires
   downloading large amounts of font data, which can significantly slow map load times. Use the
   localIdeographFontFamily setting to speed up map load times by using locally available fonts
   instead of font data fetched from the server.
*/
void Settings::setLocalFontFamily(const QString &family) {
    d_ptr->m_localFontFamily = family;
}

/*!
    Returns the client name. Returns an empty string if no client name is set.
*/
QString Settings::clientName() const {
    return d_ptr->m_clientName;
}

/*!
    Sets the client name.
*/
void Settings::setClientName(const QString &name) {
    d_ptr->m_clientName = name;
}

/*!
    Returns the client version. Returns an empty string if no client version is set.
*/
QString Settings::clientVersion() const {
    return d_ptr->m_clientVersion;
}

/*!
    Sets the client version.
*/
void Settings::setClientVersion(const QString &version) {
    d_ptr->m_clientVersion = version;
}

/*!
    Returns resource transformation callback used to transform requested URLs.
*/
std::function<std::string(const std::string &)> Settings::resourceTransform() const {
    return d_ptr->m_resourceTransform;
}

/*!
    Sets the resource \a transform callback.

    When given, resource transformation callback will be used to transform the
    requested resource URLs before they are requested from internet. This can be
    used add or remove custom parameters, or reroute certain requests to other
    servers or endpoints.
*/
void Settings::setResourceTransform(const std::function<std::string(const std::string &)> &transform) {
    d_ptr->m_resourceTransform = transform;
}

/*!
    Reset all settings based on the given template.

    MapLibre can support servers with different resource path structure.
    Some of the most common servers like Maptiler and Mapbox are defined
    in the library. This function will re-initialise all settings based
    on the default values of specific service provider defaults.
*/
void Settings::setProviderTemplate(ProviderTemplate providerTemplate) {
    d_ptr->setProviderTemplate(providerTemplate);
}

/*!
    Returns the map styles set by user.

    The styles are a list type \c QMapLibre::Style. Each style is a pair
    of URL and name/label.
*/
const Styles &Settings::styles() const {
    return d_ptr->m_styles;
}

/*!
    Sets the map styles.

    The styles are a list type \c QMapLibre::Style. Each style is a pair
    of URL and name/label.
*/
void Settings::setStyles(const Styles &styles) {
    d_ptr->m_styles = styles;
}

/*!
    All predefined provider styles.

    Return all styles that are defined in provider settings template.
*/
Styles Settings::providerStyles() const {
    Styles styles;
    if (!customTileServerOptions()) {
        return styles;
    }

    for (const auto &style : tileServerOptions().defaultStyles()) {
        styles.append(Style(QString::fromStdString(style.getUrl()), QString::fromStdString(style.getName())));
    }
    return styles;
}

/*!
    Returns the default coordinate.
*/
Coordinate Settings::defaultCoordinate() const {
    return d_ptr->m_defaultCoordinate;
}

/*!
    Sets the default coordinate.
*/
void Settings::setDefaultCoordinate(const Coordinate &coordinate) {
    d_ptr->m_defaultCoordinate = coordinate;
}

/*!
    Returns the default zoom level.
*/
double Settings::defaultZoom() const {
    return d_ptr->m_defaultZoom;
}

/*!
    Sets the default zoom level.
*/
void Settings::setDefaultZoom(double zoom) {
    d_ptr->m_defaultZoom = zoom;
}

/*!
    Returns whether the tile server options have been set by the user.
*/
bool Settings::customTileServerOptions() const {
    return d_ptr->m_customTileServerOptions;
}

/*!
    Returns the provider tile server options.

    Note that this is mainly for internal use.
*/
const mbgl::TileServerOptions &Settings::tileServerOptions() const {
    return d_ptr->m_tileServerOptions;
}

// Private implementation
SettingsPrivate::SettingsPrivate()
    : m_cacheMaximumSize(mbgl::util::DEFAULT_MAX_CACHE_SIZE),
      m_cacheDatabasePath(":memory:"),
      m_assetPath(QCoreApplication::applicationDirPath()),
      m_apiKey(qgetenv("MLN_API_KEY")) {}

void SettingsPrivate::setProviderTemplate(Settings::ProviderTemplate providerTemplate) {
    m_providerTemplate = providerTemplate;

    if (providerTemplate == Settings::MapLibreProvider) {
        m_tileServerOptions = mbgl::TileServerOptions::MapLibreConfiguration();
        m_customTileServerOptions = true;
    } else if (providerTemplate == Settings::MapTilerProvider) {
        m_tileServerOptions = mbgl::TileServerOptions::MapTilerConfiguration();
        m_customTileServerOptions = true;
    } else if (providerTemplate == Settings::MapboxProvider) {
        m_tileServerOptions = mbgl::TileServerOptions::MapboxConfiguration();
        m_customTileServerOptions = true;
    } else {
        m_tileServerOptions = mbgl::TileServerOptions();
        m_customTileServerOptions = false;
    }
}

void SettingsPrivate::setProviderApiBaseUrl(const QString &url) {
    if (!m_customTileServerOptions) {
        qWarning() << "No provider set so not setting API URL.";
        return;
    }

    m_tileServerOptions = std::move(m_tileServerOptions.withBaseURL(url.toStdString()));
}

} // namespace QMapLibre
