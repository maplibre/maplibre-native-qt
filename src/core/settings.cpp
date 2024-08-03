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
    \class Settings
    \brief The Settings class stores the initial configuration for Map.
    \ingroup QMapLibre

    \headerfile settings.hpp <QMapLibre/Settings>

    Settings is used to configure Map at the moment of its creation.
    Once created, the Settings of a Map can no longer be changed.

    Cache-related settings are shared between all Map instances using the same cache path.
    The first map to configure cache properties such as size will force the configuration
    to all newly instantiated Map objects using the same cache in the same process.
*/

/*!
    \enum Settings::GLContextMode

    This enum sets the expectations for the OpenGL state.

    \sa contextMode()
*/
/*!
    \var Settings::UniqueGLContext
    The OpenGL context is only used by Map, so it is not reset before each rendering.
    Use this mode if the intention is to only draw a fullscreen map.
*/
/*!
    \var Settings::SharedGLContext
    The OpenGL context is shared and the state will be marked dirty - which
    invalidates any previously assumed GL state. The embedder is responsible for
    clearing up the viewport prior to calling Map::render. The embedder
    is also responsible for resetting its own GL state after Map::render
    has finished, if needed.
*/

/*!
    \enum Settings::MapMode

    This enum sets the map rendering mode

    \sa mapMode()
*/
/*!
    \var Settings::Continuous
    The map will render as data arrives from the network and react immediately
    to state changes.

    This is the default mode and the preferred when the map is intended to be
    interactive.
*/
/*!
    \var Settings::Static
    The map will no longer react to state changes and will only be rendered
    when Map::startStaticRender is called. After all the resources
    are loaded, the Map::staticRenderFinished signal is emitted.

    This mode is useful for taking a snapshot of the finished rendering result
    of the map into a \c QImage.
*/

/*!
    \enum Settings::ConstrainMode

    This enum determines if the map wraps.

    \sa constrainMode()
*/
/*!
    \var Settings::NoConstrain
    The map will wrap on the horizontal axis. Since it doesn't make sense
    to wrap on the vertical axis in a Web Mercator projection, the map will
    scroll and show some empty space.
*/
/*!
    \var Settings::ConstrainHeightOnly
    The map will wrap around the horizontal axis, like a spinning globe.
    This is the recommended constrain mode.
*/
/*!
    \var Settings::ConstrainWidthAndHeight
    The map won't wrap and panning is restricted to the boundaries of the map.
*/

/*!
    \enum Settings::ViewportMode

    This enum flips the map vertically.

    \sa viewportMode()
*/
/*!
    \var Settings::DefaultViewport
    The map is rendered in its native orientation.
*/
/*!
    \var Settings::FlippedYViewport
    The map is rendered upside down.
*/

/*!
    \enum Settings::ProviderTemplate

    This enum sets the default configuration for a specific provider.

    \sa setProviderTemplate()
*/
/*!
    \var Settings::NoProvider
    No template is set. This is the default value.
*/
/*!
    \var Settings::MapLibreProvider
    The template for MapLibre-hosted vector tiles and styles.
*/
/*!
    \var Settings::MapTilerProvider
    The template for MapTiler-hosted vector tiles and styles.
*/
/*!
    \var Settings::MapboxProvider
    The template for Mapbox-hosted vector tiles and styles.
*/

/*!
    \typedef Settings::ResourceTransformFunction
    \brief Resource transformation callback type.

    This callback is used to transform the requested resource URLs before they
    are requested from internet. This can be used add or remove custom parameters,
    or reroute certain requests to other servers or endpoints.

    \sa resourceTransform()
    \sa setResourceTransform()
*/

/*!
    \brief Default constructor.
    \param provider The provider template to use.

    Constructs a Settings object with the default values.
    The default configuration is not valid for initializing a Map,
    but a provider template can be provided.
*/
Settings::Settings(ProviderTemplate provider)
    : d_ptr(std::make_unique<SettingsPrivate>()) {
    d_ptr->setProviderTemplate(provider);
}

Settings::~Settings() = default;

/*!
    \brief Copy constructor.
*/
Settings::Settings(const Settings &s)
    : d_ptr(std::make_unique<SettingsPrivate>(*s.d_ptr)) {}

/*!
    \brief Move constructor.
*/
Settings::Settings(Settings &&s) noexcept = default;

/*!
    \brief Copy assignment operator.
*/
Settings &Settings::operator=(const Settings &s) {
    d_ptr = std::make_unique<SettingsPrivate>(*s.d_ptr);
    return *this;
}

/*!
    \brief Move assignment operator.
*/
Settings &Settings::operator=(Settings &&s) noexcept = default;

/*!
    \brief Get the OpenGL context mode.
    \return The OpenGL context mode.

    Returns the OpenGL context mode. This is specially important when mixing
    with other OpenGL draw calls.

    By default, it is set to Settings::SharedGLContext.
*/
Settings::GLContextMode Settings::contextMode() const {
    return d_ptr->m_contextMode;
}

/*!
    \brief Set the OpenGL context mode.
    \param mode The OpenGL context mode.
*/
void Settings::setContextMode(GLContextMode mode) {
    d_ptr->m_contextMode = mode;
}

/*!
    \brief Get the map mode.
    \return The map mode.

    Returns the map mode. Static mode will emit a signal for
    rendering a map only when the map is fully loaded.
    Animations like style transitions and labels fading won't
    be seen.

    The Continuous mode will emit the signal for every new
    change on the map and it is usually what you expect for
    a interactive map.

    By default, it is set to Settings::Continuous.
*/
Settings::MapMode Settings::mapMode() const {
    return d_ptr->m_mapMode;
}

/*!
    \brief Set the map mode.
    \param mode The map mode.
*/
void Settings::setMapMode(MapMode mode) {
    d_ptr->m_mapMode = mode;
}

/*!
    \brief Get the constrain mode.
    \return The constrain mode.

    Returns the constrain mode. This is used to limit the map to wrap
    around the globe horizontally.

    By default, it is set to Settings::ConstrainHeightOnly.
*/
Settings::ConstrainMode Settings::constrainMode() const {
    return d_ptr->m_constrainMode;
}

/*!
    \brief Set the map constrain mode.
    \param mode The constrain mode.
*/
void Settings::setConstrainMode(ConstrainMode mode) {
    d_ptr->m_constrainMode = mode;
}

/*!
    \brief Get the viewport mode.
    \return The viewport mode.

    Returns the viewport mode. This is used to flip the vertical
    orientation of the map as some devices may use inverted orientation.

    By default, it is set to Settings::DefaultViewport.
*/
Settings::ViewportMode Settings::viewportMode() const {
    return d_ptr->m_viewportMode;
}

/*!
    \brief Set the viewport mode.
    \param mode The viewport mode.
*/
void Settings::setViewportMode(ViewportMode mode) {
    d_ptr->m_viewportMode = mode;
}

/*!
    \brief Get the cache database maximum size.
    \return The cache database maximum size.

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
    \brief Set the maximum allowed cache database size in bytes.
    \param size The maximum allowed cache database size in bytes.
*/
void Settings::setCacheDatabaseMaximumSize(unsigned size) {
    d_ptr->m_cacheMaximumSize = size;
}

/*!
    \brief Get the cache database path.
    \return The cache database path as \c QString.

    Returns the cache database path. The cache is used for storing
    recently used resources like tiles and also an offline tile database
    pre-populated by the
    <a href="https://github.com/maplibre/maplibre-native/blob/main/bin/offline.sh">
    Offline Tool</a>.

    By default, it is set to `:memory:` meaning it will create an in-memory
    cache instead of a file on disk.
*/
QString Settings::cacheDatabasePath() const {
    return d_ptr->m_cacheDatabasePath;
}

/*!
    \brief Set the cache database path.
    \param path The cache database path.

    Setting the \a path to `:memory:` will create an in-memory cache.
*/
void Settings::setCacheDatabasePath(const QString &path) {
    d_ptr->m_cacheDatabasePath = path;
}

/*!
    \brief Get the asset path.
    \return The asset path as \c QString.

    Returns the asset path, which is the root directory from where
    the `asset://` scheme gets resolved in a style. `asset://` can be used
    for loading a resource from the disk in a style rather than fetching
    it from the network.

    By default, it is set to the value returned by \c QCoreApplication::applicationDirPath().
*/
QString Settings::assetPath() const {
    return d_ptr->m_assetPath;
}

/*!
    \brief Set the asset path.
    \param path The asset path.
*/
void Settings::setAssetPath(const QString &path) {
    d_ptr->m_assetPath = path;
}

/*!
    \brief Get the API key.
    \return The API key as \c QString.

    By default, it is taken from the environment variable \c MLN_API_KEY
    or empty if the variable is not set.
*/
QString Settings::apiKey() const {
    return d_ptr->m_apiKey;
}

/*!
    \brief Set the API key.
    \param key The API key.

    MapTiler-hosted and Mapbox-hosted vector tiles and styles require an API
    key or access token.
*/
void Settings::setApiKey(const QString &key) {
    d_ptr->m_apiKey = key;
}

/*!
    \brief Get the API base URL.
    \return The API base URL as \c QString.

    Only works if a provider template is set. Otherwise, it returns an empty string.

    \sa setProviderTemplate()
*/
QString Settings::apiBaseUrl() const {
    if (!customTileServerOptions()) {
        return {};
    }

    return QString::fromStdString(tileServerOptions().baseURL());
}

/*!
    \brief Set the API base url.
    \param url The API base URL.

    The API base URL is the URL that the `mapbox://` protocol will
    be resolved to. It defaults to `https://api.mapbox.com` but can be
    changed, for instance, to a tile cache server address.
*/
void Settings::setApiBaseUrl(const QString &url) {
    d_ptr->setProviderApiBaseUrl(url);
}

/*!
    \brief Get the local font family.
    \return The local font family as \c QString.

    Returns an empty string if no local font family is set.
*/
QString Settings::localFontFamily() const {
    return d_ptr->m_localFontFamily;
}

/*!
    \brief Set the local font family.
    \param family The local font family.

    Rendering Chinese/Japanese/Korean (CJK) ideographs and precomposed Hangul
    Syllables requires downloading large amounts of font data, which can
    significantly slow map load times. Use the \ref localFontFamily setting
    to speed up map load times by using locally available fonts instead
    of font data fetched from the server.
*/
void Settings::setLocalFontFamily(const QString &family) {
    d_ptr->m_localFontFamily = family;
}

/*!
    \brief Get the client name.
    \return The client name as \c QString.

    Returns an empty string if no client name is set.
*/
QString Settings::clientName() const {
    return d_ptr->m_clientName;
}

/*!
    \brief Set the client name.
    \param name The client name.
*/
void Settings::setClientName(const QString &name) {
    d_ptr->m_clientName = name;
}

/*!
    \brief Get the client version.
    \return The client version as \c QString.

    Returns an empty string if no client version is set.
*/
QString Settings::clientVersion() const {
    return d_ptr->m_clientVersion;
}

/*!
    \brief Set the client version.
    \param version The client version.
*/
void Settings::setClientVersion(const QString &version) {
    d_ptr->m_clientVersion = version;
}

/*!
    \brief Get resource transformation callback used to transform requested URLs.
    \return The resource transformation callback.
*/
Settings::ResourceTransformFunction Settings::resourceTransform() const {
    return d_ptr->m_resourceTransform;
}

/*!
    \brief Sets the resource transform callback.
    \param transform The resource transformation callback.

    When given, resource transformation callback will be used to transform the
    requested resource URLs before they are requested from internet. This can be
    used add or remove custom parameters, or reroute certain requests to other
    servers or endpoints.
*/
void Settings::setResourceTransform(const ResourceTransformFunction &transform) {
    d_ptr->m_resourceTransform = transform;
}

/*!
    \brief Reset all settings based on the given template.
    \param providerTemplate The provider template.

    MapLibre can support servers with different resource path structure.
    Some of the most common servers like Maptiler and Mapbox are defined
    in the library. This function will re-initialise all settings based
    on the default values of specific service provider defaults.

    \sa apiBaseUrl()
    \sa providerStyles()
*/
void Settings::setProviderTemplate(ProviderTemplate providerTemplate) {
    d_ptr->setProviderTemplate(providerTemplate);
}

/*!
    \brief Get map styles set by user.
    \return The map styles.

    The styles are a list of type Style (wrapped in the helper \ref Styles).
    Each style is a pair of URL and name/label.
*/
const Styles &Settings::styles() const {
    return d_ptr->m_styles;
}

/*!
    \brief Set the map styles.
    \param styles The map styles.

    The styles are a list type Style (wrapped in the helper \ref Styles).
    Each style is a pair of URL and name/label.
*/
void Settings::setStyles(const Styles &styles) {
    d_ptr->m_styles = styles;
}

/*!
    \brief All predefined provider styles.
    \return The provider styles.

    Returns all styles that are defined in provider settings template.
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
    \brief Get the default coordinate.
    \return The default coordinate.
*/
Coordinate Settings::defaultCoordinate() const {
    return d_ptr->m_defaultCoordinate;
}

/*!
    \brief Set the default coordinate.
    \param coordinate The default coordinate.
*/
void Settings::setDefaultCoordinate(const Coordinate &coordinate) {
    d_ptr->m_defaultCoordinate = coordinate;
}

/*!
    \brief Get the default zoom level.
    \return The default zoom level.
*/
double Settings::defaultZoom() const {
    return d_ptr->m_defaultZoom;
}

/*!
    \brief Set the default zoom level.
    \param zoom The default zoom level.
*/
void Settings::setDefaultZoom(double zoom) {
    d_ptr->m_defaultZoom = zoom;
}

/*!
    \brief Check whether the tile server options have been set by the user.
    \return \c true if the tile server options have been set by the user.
*/
bool Settings::customTileServerOptions() const {
    return d_ptr->m_customTileServerOptions;
}

/*!
    \brief Get the provider tile server options.

    \warning Mainly for internal use.
*/
const mbgl::TileServerOptions &Settings::tileServerOptions() const {
    return d_ptr->m_tileServerOptions;
}

/*! \cond PRIVATE */

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

/*! \endcond */

} // namespace QMapLibre
