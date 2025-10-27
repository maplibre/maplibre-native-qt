// Copyright (C) 2023 MapLibre contributors
// Copyright (C) 2020 Mapbox, Inc.

// SPDX-License-Identifier: BSD-2-Clause

#include "map.hpp"
#include "map_p.hpp"

#include "conversion_p.hpp"
#include "geojson_p.hpp"
#include "map_observer_p.hpp"

#include "rendering/renderer_observer_p.hpp"

#if defined(Q_OS_WINDOWS) && defined(GetObject)
#undef GetObject
#endif

#include <mbgl/actor/scheduler.hpp>
#include <mbgl/annotation/annotation.hpp>
#include <mbgl/map/camera.hpp>
#include <mbgl/map/map.hpp>
#include <mbgl/map/map_options.hpp>
#include <mbgl/math/log2.hpp>
#include <mbgl/math/minmax.hpp>
#include <mbgl/renderer/renderer.hpp>
#include <mbgl/storage/file_source_manager.hpp>
#include <mbgl/storage/network_status.hpp>
#include <mbgl/storage/online_file_source.hpp>
#include <mbgl/storage/resource_options.hpp>
#include <mbgl/style/conversion/coordinate.hpp>
#include <mbgl/style/conversion/filter.hpp>
#include <mbgl/style/conversion/geojson.hpp>
#include <mbgl/style/conversion/layer.hpp>
#include <mbgl/style/conversion/source.hpp>
#include <mbgl/style/conversion_impl.hpp>
#include <mbgl/style/filter.hpp>
#include <mbgl/style/image.hpp>
#include <mbgl/style/layers/background_layer.hpp>
#include <mbgl/style/layers/circle_layer.hpp>
#include <mbgl/style/layers/fill_extrusion_layer.hpp>
#include <mbgl/style/layers/fill_layer.hpp>
#include <mbgl/style/layers/line_layer.hpp>
#include <mbgl/style/layers/raster_layer.hpp>
#include <mbgl/style/layers/symbol_layer.hpp>
#include <mbgl/style/rapidjson_conversion.hpp>
#include <mbgl/style/sources/geojson_source.hpp>
#include <mbgl/style/sources/image_source.hpp>
#include <mbgl/style/style.hpp>
#include <mbgl/style/transition_options.hpp>
#include <mbgl/util/color.hpp>
#include <mbgl/util/constants.hpp>
#include <mbgl/util/geo.hpp>
#include <mbgl/util/geometry.hpp>
#include <mbgl/util/image.hpp>
#include <mbgl/util/projection.hpp>
#include <mbgl/util/rapidjson.hpp>
#include <mbgl/util/run_loop.hpp>
#include <mbgl/util/tile_server_options.hpp>
#include <mbgl/util/traits.hpp>

#include <mbgl/style/layers/custom_layer.hpp>

#include <QtCore/QDebug>
#include <QtCore/QThreadStorage>
#include <QtCore/QVariant>
#include <QtCore/QVariantList>
#include <QtCore/QVariantMap>
#include <QtGui/QColor>

#include <functional>
#include <memory>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4805)
#endif

// mbgl::NorthOrientation
static_assert(mbgl::underlying_type(QMapLibre::Map::NorthUpwards) ==
                  mbgl::underlying_type(mbgl::NorthOrientation::Upwards),
              "error");
static_assert(mbgl::underlying_type(QMapLibre::Map::NorthRightwards) ==
                  mbgl::underlying_type(mbgl::NorthOrientation::Rightwards),
              "error");
static_assert(mbgl::underlying_type(QMapLibre::Map::NorthDownwards) ==
                  mbgl::underlying_type(mbgl::NorthOrientation::Downwards),
              "error");
static_assert(mbgl::underlying_type(QMapLibre::Map::NorthLeftwards) ==
                  mbgl::underlying_type(mbgl::NorthOrientation::Leftwards),
              "error");

#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace {

QThreadStorage<std::shared_ptr<mbgl::util::RunLoop>> loop;

// Conversion helper functions.

QVariant variantFromValue(const mbgl::Value &value) {
    return value.match([](const mbgl::NullValue) { return QVariant(); },
                       [](const bool value_) { return QVariant(value_); },
                       [](const float value_) { return QVariant(value_); },
                       [](const int64_t value_) { return QVariant(static_cast<qlonglong>(value_)); },
                       [](const double value_) { return QVariant(value_); },
                       [](const std::string &value_) { return QVariant(value_.c_str()); },
                       [](const mbgl::Color &value_) {
                           return QColor(static_cast<int>(value_.r),
                                         static_cast<int>(value_.g),
                                         static_cast<int>(value_.b),
                                         static_cast<int>(value_.a));
                       },
                       [&](const std::vector<mbgl::Value> &vector) {
                           QVariantList list;
                           list.reserve(static_cast<int>(vector.size()));
                           for (const auto &value_ : vector) {
                               list.push_back(variantFromValue(value_));
                           }
                           return list;
                       },
                       [&](const std::unordered_map<std::string, mbgl::Value> &map) {
                           QVariantMap varMap;
                           for (const auto &item : map) {
                               varMap.insert(item.first.c_str(), variantFromValue(item.second));
                           }
                           return varMap;
                       },
                       [](const auto &) { return QVariant(); });
}

mbgl::Size sanitizeSize(const QSize &size) {
    return mbgl::Size{
        mbgl::util::max(0u, static_cast<uint32_t>(size.width())),
        mbgl::util::max(0u, static_cast<uint32_t>(size.height())),
    };
};

std::unique_ptr<mbgl::style::Image> toStyleImage(const QString &id, const QImage &sprite) {
    const QImage swapped = sprite.rgbSwapped().convertToFormat(QImage::Format_ARGB32_Premultiplied);

#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    auto img = std::make_unique<uint8_t[]>(swapped.sizeInBytes());
    memcpy(img.get(), swapped.constBits(), swapped.sizeInBytes());
#else
    auto img = std::make_unique<uint8_t[]>(swapped.byteCount());
    memcpy(img.get(), swapped.constBits(), swapped.byteCount());
#endif

    return std::make_unique<mbgl::style::Image>(
        id.toStdString(),
        mbgl::PremultipliedImage({static_cast<uint32_t>(swapped.width()), static_cast<uint32_t>(swapped.height())},
                                 std::move(img)),
        1.0);
}

mbgl::MapOptions mapOptionsFromSettings(const QMapLibre::Settings &settings, const QSize &size, qreal pixelRatio) {
    return std::move(mbgl::MapOptions()
                         .withSize(sanitizeSize(size))
                         .withPixelRatio(static_cast<float>(pixelRatio))
                         .withMapMode(static_cast<mbgl::MapMode>(settings.mapMode()))
                         .withConstrainMode(static_cast<mbgl::ConstrainMode>(settings.constrainMode()))
                         .withViewportMode(static_cast<mbgl::ViewportMode>(settings.viewportMode())));
}

mbgl::ResourceOptions resourceOptionsFromSettings(const QMapLibre::Settings &settings) {
    if (!settings.customTileServerOptions()) {
        return std::move(mbgl::ResourceOptions()
                             .withAssetPath(settings.assetPath().toStdString())
                             .withCachePath(settings.cacheDatabasePath().toStdString())
                             .withMaximumCacheSize(settings.cacheDatabaseMaximumSize()));
    }

    return std::move(mbgl::ResourceOptions()
                         .withApiKey(settings.apiKey().toStdString())
                         .withAssetPath(settings.assetPath().toStdString())
                         .withTileServerOptions(settings.tileServerOptions())
                         .withCachePath(settings.cacheDatabasePath().toStdString())
                         .withMaximumCacheSize(settings.cacheDatabaseMaximumSize()));
}

mbgl::ClientOptions clientOptionsFromSettings(const QMapLibre::Settings &settings) {
    return std::move(mbgl::ClientOptions()
                         .withName(settings.clientName().toStdString())
                         .withVersion(settings.clientVersion().toStdString()));
}

std::optional<mbgl::Annotation> asAnnotation(const QMapLibre::Annotation &annotation) {
    auto asGeometry = [](const QMapLibre::ShapeAnnotationGeometry &geometry) {
        mbgl::ShapeAnnotationGeometry result;
        switch (geometry.type) {
            case QMapLibre::ShapeAnnotationGeometry::LineStringType:
                result = QMapLibre::GeoJSON::asLineString(geometry.geometry.first().first());
                break;
            case QMapLibre::ShapeAnnotationGeometry::PolygonType:
                result = QMapLibre::GeoJSON::asPolygon(geometry.geometry.first());
                break;
            case QMapLibre::ShapeAnnotationGeometry::MultiLineStringType:
                result = QMapLibre::GeoJSON::asMultiLineString(geometry.geometry.first());
                break;
            case QMapLibre::ShapeAnnotationGeometry::MultiPolygonType:
                result = QMapLibre::GeoJSON::asMultiPolygon(geometry.geometry);
                break;
        }
        return result;
    };

    if (annotation.canConvert<QMapLibre::SymbolAnnotation>()) {
        const auto symbolAnnotation = annotation.value<QMapLibre::SymbolAnnotation>();
        const QMapLibre::Coordinate &pair = symbolAnnotation.geometry;
        return {
            mbgl::SymbolAnnotation(mbgl::Point<double>{pair.second, pair.first}, symbolAnnotation.icon.toStdString())};
    }

    if (annotation.canConvert<QMapLibre::LineAnnotation>()) {
        const auto lineAnnotation = annotation.value<QMapLibre::LineAnnotation>();
        auto color = mbgl::Color::parse(mbgl::style::conversion::convertColor(lineAnnotation.color));
        if (!color) {
            qWarning() << "Unable to convert annotation:" << annotation;
            return {};
        }
        return {mbgl::LineAnnotation(
            asGeometry(lineAnnotation.geometry), lineAnnotation.opacity, lineAnnotation.width, {*color})};
    }

    if (annotation.canConvert<QMapLibre::FillAnnotation>()) {
        const auto fillAnnotation = annotation.value<QMapLibre::FillAnnotation>();
        auto color = mbgl::Color::parse(mbgl::style::conversion::convertColor(fillAnnotation.color));
        if (!color) {
            qWarning() << "Unable to convert annotation:" << annotation;
            return {};
        }
        if (fillAnnotation.outlineColor.canConvert<QColor>()) {
            auto outlineColor = mbgl::Color::parse(
                mbgl::style::conversion::convertColor(fillAnnotation.outlineColor.value<QColor>()));
            if (!outlineColor) {
                qWarning() << "Unable to convert annotation:" << annotation;
                return {};
            }
            return {mbgl::FillAnnotation(
                asGeometry(fillAnnotation.geometry), fillAnnotation.opacity, {*color}, {*outlineColor})};
        }
        return {mbgl::FillAnnotation(asGeometry(fillAnnotation.geometry), fillAnnotation.opacity, {*color}, {})};
    }

    qWarning() << "Unable to convert annotation:" << annotation;
    return {};
}

} // namespace

namespace QMapLibre {

/*!
    \class Map
    \brief The Map class is a Qt wrapper for the MapLibre Native engine.
    \ingroup QMapLibre

    \headerfile map.hpp <QMapLibre/Map>

    Map is a Qt friendly version the MapLibre Native engine using Qt types
    and deep integration with Qt event loop. Map relies as much as possible
    on Qt, trying to minimize the external dependencies. For instance it will
    use \c QNetworkAccessManager for HTTP requests and \c QString for UTF-8
    manipulation.

    Map is not thread-safe and it is assumed that it will be accessed from
    the same thread as the thread where the OpenGL context lives.
*/

/*!
    \enum Map::MapChange

    This enum represents the last changed occurred to the map state.

    \sa mapChanged()
*/
/*!
    \var Map::MapChangeRegionWillChange
    A region of the map will change, like when resizing the map.
*/
/*!
    \var Map::MapChangeRegionWillChangeAnimated
    Not in use by Map.
*/
/*!
    \var Map::MapChangeRegionIsChanging
    A region of the map is changing.
*/
/*!
    \var Map::MapChangeRegionDidChange
    A region of the map finished changing.
*/
/*!
    \var Map::MapChangeRegionDidChangeAnimated
    Not in use by Map.
*/
/*!
    \var Map::MapChangeWillStartLoadingMap
    The map is getting loaded. This state is set only once right after Map is created
    and a style is set.
*/
/*!
    \var Map::MapChangeDidFinishLoadingMap
    All the resources were loaded and parsed and the map is fully rendered. After this state the
    mapChanged() signal won't fire again unless the is some client side interaction with the map
    or a tile expires, causing a new resource to be requested from the network.
*/
/*!
    \var Map::MapChangeDidFailLoadingMap
    An error occurred when loading the map.
*/
/*!
    \var Map::MapChangeWillStartRenderingFrame
    Just before rendering the frame. This is the state of the map just after calling render()
    and might happened many times before the map is fully loaded.
*/
/*!
    \var Map::MapChangeDidFinishRenderingFrame
    The current frame was rendered but was left in a partial state. Some parts of the map might
    be missing because have not arrived from the network or are being parsed.
*/
/*!
    \var Map::MapChangeDidFinishRenderingFrameFullyRendered
    The current frame was fully rendered.
*/
/*!
    \var Map::MapChangeWillStartRenderingMap
    Set once when the map is about to get rendered for the first time.
*/
/*!
    \var Map::MapChangeDidFinishRenderingMap
    Not in use by Map.
*/
/*!
    \var Map::MapChangeDidFinishRenderingMapFullyRendered
    Map is fully loaded and rendered.
*/
/*!
    \var Map::MapChangeDidFinishLoadingStyle
    The style was loaded.
*/
/*!
    \var Map::MapChangeSourceDidChange
    A source has changed.
*/

/*!
    \enum Map::MapLoadingFailure

    This enum represents map loading failure type.

    \sa mapLoadingFailed()
*/
/*!
    \var Map::StyleParseFailure
    Failure to parse the style.
*/
/*!
    \var Map::StyleLoadFailure
    Failure to load the style data.
*/
/*!
    \var Map::NotFoundFailure
    Failure to obtain style resource file.
*/
/*!
    \var Map::UnknownFailure
    Unknown map loading failure.
*/

/*!
    \enum Map::NorthOrientation

    This enum sets the orientation of the north bearing. It will directly affect bearing when
    resetting the north (i.e. setting bearing to 0).

    \sa northOrientation()
    \sa bearing()
*/
/*!
    \var Map::NorthUpwards
    The north is pointing up in the map. This is usually how maps are oriented.
*/
/*!
    \var Map::NorthRightwards
    The north is pointing right.
*/
/*!
    \var Map::NorthDownwards
    The north is pointing down.
*/
/*!
    \var Map::NorthLeftwards
    The north is pointing left.
*/

/*!
    \brief Main constructor for Map.
    \param parent The parent object.
    \param settings The settings for the map.
    \param size The size of the viewport.
    \param pixelRatio The initial pixel density of the screen.

    Constructs a Map object with settings and sets the parent object.
    The settings cannot be changed after the object is constructed.
*/
Map::Map(QObject *parent, const Settings &settings, const QSize &size, qreal pixelRatio)
    : QObject(parent) {
    assert(!size.isEmpty());

    // Multiple Map instances running on the same thread
    // will share the same mbgl::util::RunLoop
    if (!loop.hasLocalData()) {
        loop.setLocalData(std::make_shared<mbgl::util::RunLoop>());
    }

    d_ptr = std::make_unique<MapPrivate>(this, settings, size, pixelRatio);
}

Map::~Map() = default;

/*!
    \property Map::styleJson
    \brief The map's current style JSON.
    \sa styleJson()
    \sa setStyleJson()
*/

/*!
    \brief Get the map style JSON.
    \return The map style JSON.
*/
QString Map::styleJson() const {
    return QString::fromStdString(d_ptr->mapObj->getStyle().getJSON());
}

/*!
    \brief Set a new style from a JSON.
    \param style The style JSON.

    The JSON must conform to the
    <a href="https://maplibre.org/maplibre-style-spec/">MapLibre Style spec</a>.

    \note In case of a invalid style it will trigger a \ref mapChanged
    signal with Map::MapChangeDidFailLoadingMap as argument.
*/
void Map::setStyleJson(const QString &style) {
    d_ptr->mapObj->getStyle().loadJSON(style.toStdString());
}

/*!
    \property Map::styleUrl
    \brief The map's current style URL.
    \sa styleUrl()
    \sa setStyleUrl()
*/

/*!
    \brief Get the map style URL.
    \return The map style URL.
*/
QString Map::styleUrl() const {
    return QString::fromStdString(d_ptr->mapObj->getStyle().getURL());
}

/*!
    \brief Set the map style URL.
    \param url The map style URL.

    Sets a URL for fetching a JSON that will be later fed to
    \ref setStyleJson. URLs using the Mapbox scheme (`mapbox://`) are
    also accepted and translated automatically to an actual HTTPS
    request.

    The Mapbox scheme is not enforced and a style can be fetched
    from anything that \c QNetworkAccessManager can handle.

    \note In case of a invalid style it will trigger a \ref mapChanged
    signal with Map::MapChangeDidFailLoadingMap as argument.
*/
void Map::setStyleUrl(const QString &url) {
    d_ptr->mapObj->getStyle().loadURL(url.toStdString());
}

/*!
    \property Map::latitude
    \brief The map's current latitude in degrees.
    \sa latitude()
    \sa setLatitude()
*/

/*!
    \brief Get the map latitude.
    \return The map latitude in degrees.
*/
double Map::latitude() const {
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    return d_ptr->mapObj->getCameraOptions(d_ptr->margins).center->latitude();
}

/*!
    \brief Set the map latitude.
    \param latitude The map latitude in degrees.

    Setting a latitude doesn't necessarily mean it will be accepted since Map
    might constrain it within the limits of the Web Mercator projection.
*/
void Map::setLatitude(double latitude) {
    d_ptr->mapObj->jumpTo(
        mbgl::CameraOptions().withCenter(mbgl::LatLng{latitude, longitude()}).withPadding(d_ptr->margins));
}

/*!
    \property Map::longitude
    \brief The map current longitude in degrees.
    \sa longitude()
    \sa setLongitude()
*/

/*!
    \brief Get the map longitude.
    \return The map longitude in degrees.
*/
double Map::longitude() const {
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    return d_ptr->mapObj->getCameraOptions(d_ptr->margins).center->longitude();
}

/*!
    \brief Set the map longitude.
    \param longitude The map longitude in degrees.

    Setting a longitude beyond the limits of the Web Mercator projection will
    make the map wrap. As an example, setting the longitude to 360 is effectively
    the same as setting it to 0.
*/
void Map::setLongitude(double longitude) {
    d_ptr->mapObj->jumpTo(
        mbgl::CameraOptions().withCenter(mbgl::LatLng{latitude(), longitude}).withPadding(d_ptr->margins));
}

/*!
    \property Map::scale
    \brief The map scale factor.
    \sa scale()
    \sa setScale()
*/

/*!
    \brief Get the map scale factor.
    \return The map scale factor.

    \sa zoom()
*/
double Map::scale() const {
    constexpr double zoomScaleBase{2.0};
    return std::pow(zoomScaleBase, zoom());
}

/*!
    \brief Set the map scale factor.
    \param scale The map scale factor.
    \param center The center pixel coordinate.

    This property is used to zoom the map. When \a center is defined, the map will
    scale in the direction of the center pixel coordinates. The \a center will remain
    at the same pixel coordinate after scaling as before calling this method.

    \note This function could be used for implementing a pinch gesture or zooming
    by using the mouse scroll wheel.

    \sa setZoom()
*/
void Map::setScale(double scale, const QPointF &center) {
    d_ptr->mapObj->jumpTo(
        mbgl::CameraOptions().withZoom(::log2(scale)).withAnchor(mbgl::ScreenCoordinate{center.x(), center.y()}));
}

/*!
    \property Map::zoom
    \brief the map zoom factor.
    \sa zoom()
    \sa setZoom()
*/

/*!
    \brief Get the map zoom factor.
    \return The map zoom factor.

    \sa scale()
*/
double Map::zoom() const {
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    return *d_ptr->mapObj->getCameraOptions().zoom;
}

/*!
    \brief Set the map zoom factor.
    \param zoom The map zoom factor.

    This property is used to zoom the map. When \a center is defined, the map will
    zoom in the direction of the center. This function could be used for implementing
    a pinch gesture or zooming by using the mouse scroll wheel.

    \sa setZoom()
*/
void Map::setZoom(double zoom) {
    d_ptr->mapObj->jumpTo(mbgl::CameraOptions().withZoom(zoom).withPadding(d_ptr->margins));
}

/*!
    \brief Get the minimum zoom level allowed for the map.
    \return The minimum zoom level allowed for the map.

    \sa maximumZoom()
*/
double Map::minimumZoom() const {
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    return *d_ptr->mapObj->getBounds().minZoom;
}

/*!
    \brief Get the maximum zoom level allowed for the map.
    \return The maximum zoom level allowed for the map.

    \sa minimumZoom()
*/
double Map::maximumZoom() const {
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    return *d_ptr->mapObj->getBounds().maxZoom;
}

/*!
    \property Map::coordinate
    \brief The map center coordinate.
    \sa coordinate()
    \sa setCoordinate()
*/

/*!
    \brief Get the map center coordinate.
    \return The map center coordinate.

    \sa margins()
*/
Coordinate Map::coordinate() const {
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    const mbgl::LatLng latLng = *d_ptr->mapObj->getCameraOptions(d_ptr->margins).center;
    return {latLng.latitude(), latLng.longitude()};
}

/*!
    \brief Set the map center coordinate.
    \param coordinate The map center coordinate.

    Centers the map at a geographic coordinate respecting the margins, if set.

    \sa setMargins()
*/
void Map::setCoordinate(const Coordinate &coordinate) {
    d_ptr->mapObj->jumpTo(mbgl::CameraOptions()
                              .withCenter(mbgl::LatLng{coordinate.first, coordinate.second})
                              .withPadding(d_ptr->margins));
}

/*!
    \brief Convenience method for setting the coordinate and zoom simultaneously.
    \param coordinate The map center coordinate.
    \param zoom The map zoom factor.

    \note Setting \a coordinate and \a zoom at once is more efficient than doing
    it in two steps.

    \sa zoom()
    \sa coordinate()
*/
void Map::setCoordinateZoom(const Coordinate &coordinate, double zoom) {
    d_ptr->mapObj->jumpTo(mbgl::CameraOptions()
                              .withCenter(mbgl::LatLng{coordinate.first, coordinate.second})
                              .withZoom(zoom)
                              .withPadding(d_ptr->margins));
}

/*!
    \brief Atomically jump to the camera options.
    \param camera The camera options.
*/
void Map::jumpTo(const CameraOptions &camera) {
    mbgl::CameraOptions mbglCamera;
    if (camera.center.isValid()) {
        const auto center = camera.center.value<Coordinate>();
        mbglCamera.center = mbgl::LatLng{center.first, center.second};
    }
    if (camera.anchor.isValid()) {
        const auto anchor = camera.anchor.value<QPointF>();
        mbglCamera.anchor = mbgl::ScreenCoordinate{anchor.x(), anchor.y()};
    }
    if (camera.zoom.isValid()) {
        mbglCamera.zoom = camera.zoom.value<double>();
    }
    if (camera.bearing.isValid()) {
        mbglCamera.bearing = camera.bearing.value<double>();
    }
    if (camera.pitch.isValid()) {
        mbglCamera.pitch = camera.pitch.value<double>();
    }

    mbglCamera.padding = d_ptr->margins;

    d_ptr->mapObj->jumpTo(mbglCamera);
}

/*!
    \brief Animate to the camera options with easing.
    \param camera The camera options.
    \param animation The animation options.
*/
void Map::easeTo(const CameraOptions &camera, const AnimationOptions &animation) {
    mbgl::CameraOptions mbglCamera;
    if (camera.center.isValid()) {
        const auto center = camera.center.value<Coordinate>();
        mbglCamera.center = mbgl::LatLng{center.first, center.second};
    }
    if (camera.anchor.isValid()) {
        const auto anchor = camera.anchor.value<QPointF>();
        mbglCamera.anchor = mbgl::ScreenCoordinate{anchor.x(), anchor.y()};
    }
    if (camera.zoom.isValid()) {
        mbglCamera.zoom = camera.zoom.value<double>();
    }
    if (camera.bearing.isValid()) {
        mbglCamera.bearing = camera.bearing.value<double>();
    }
    if (camera.pitch.isValid()) {
        mbglCamera.pitch = camera.pitch.value<double>();
    }
    mbglCamera.padding = d_ptr->margins;

    mbgl::AnimationOptions mbglAnimation;
    if (animation.duration.isValid()) {
        mbglAnimation.duration = std::chrono::milliseconds(animation.duration.value<qint64>());
    }
    if (animation.velocity.isValid()) {
        mbglAnimation.velocity = animation.velocity.value<double>();
    }
    if (animation.minZoom.isValid()) {
        mbglAnimation.minZoom = animation.minZoom.value<double>();
    }

    d_ptr->mapObj->easeTo(mbglCamera, mbglAnimation);
}

/*!
    \brief Animate to the camera options with a flight path.
    \param camera The camera options.
    \param animation The animation options.
*/
void Map::flyTo(const CameraOptions &camera, const AnimationOptions &animation) {
    mbgl::CameraOptions mbglCamera;
    if (camera.center.isValid()) {
        const auto center = camera.center.value<Coordinate>();
        mbglCamera.center = mbgl::LatLng{center.first, center.second};
    }
    if (camera.anchor.isValid()) {
        const auto anchor = camera.anchor.value<QPointF>();
        mbglCamera.anchor = mbgl::ScreenCoordinate{anchor.x(), anchor.y()};
    }
    if (camera.zoom.isValid()) {
        mbglCamera.zoom = camera.zoom.value<double>();
    }
    if (camera.bearing.isValid()) {
        mbglCamera.bearing = camera.bearing.value<double>();
    }
    if (camera.pitch.isValid()) {
        mbglCamera.pitch = camera.pitch.value<double>();
    }
    mbglCamera.padding = d_ptr->margins;

    mbgl::AnimationOptions mbglAnimation;
    if (animation.duration.isValid()) {
        mbglAnimation.duration = std::chrono::milliseconds(animation.duration.value<qint64>());
    }
    if (animation.velocity.isValid()) {
        mbglAnimation.velocity = animation.velocity.value<double>();
    }
    if (animation.minZoom.isValid()) {
        mbglAnimation.minZoom = animation.minZoom.value<double>();
    }

    d_ptr->mapObj->flyTo(mbglCamera, mbglAnimation);
}

/*!
    \property Map::bearing
    \brief the map bearing in degrees.
    \sa bearing()
    \sa setBearing()
*/

/*!
    \brief Get the bearing angle.
    \return The bearing angle in degrees.

    \sa margins()
*/
double Map::bearing() const {
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    return *d_ptr->mapObj->getCameraOptions().bearing;
}

/*!
    \brief Set the bearing angle.
    \param degrees The bearing angle in degrees.

    Negative values and values over 360 are valid and will wrap.
    The direction of the rotation is counterclockwise.

    \sa setMargins()
*/
void Map::setBearing(double degrees) {
    d_ptr->mapObj->jumpTo(mbgl::CameraOptions().withBearing(degrees).withPadding(d_ptr->margins));
}

/*!
    \brief Set the bearing angle.
    \param degrees The bearing angle in degrees.
    \param center The center pixel coordinate.

    Negative values and values over 360 are valid and will wrap.
    The direction of the rotation is counterclockwise.

    When \a center is defined, the map will rotate around the center pixel
    coordinate respecting the margins if defined.

    \sa setMargins()
*/
void Map::setBearing(double degrees, const QPointF &center) {
    d_ptr->mapObj->jumpTo(
        mbgl::CameraOptions().withBearing(degrees).withAnchor(mbgl::ScreenCoordinate{center.x(), center.y()}));
}

/*!
    \property Map::pitch
    \brief the map pitch in degrees.
    \sa pitch()
    \sa setPitch()
*/

/*!
    \brief Get the pitch angle.
    \return The pitch angle in degrees.

    \sa margins()
*/
double Map::pitch() const {
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    return *d_ptr->mapObj->getCameraOptions().pitch;
}

/*!
    \brief Set the pitch angle.
    \param pitch The pitch angle in degrees.

    Pitch toward the horizon measured in degrees, with 0 resulting in a
    two-dimensional map. It will be constrained at 60 degrees.

    \sa margins()
    \sa pitchBy()
*/
void Map::setPitch(double pitch) {
    d_ptr->mapObj->jumpTo(mbgl::CameraOptions().withPitch(pitch));
}

/*!
    \brief Pitch the map for an angle.
    \param pitch The pitch angle in degrees.

    \sa setPitch()
*/
void Map::pitchBy(double pitch) {
    d_ptr->mapObj->pitchBy(pitch);
}

/*!
    \brief Get the map north orientation mode.
    \return The map north orientation mode.
*/
Map::NorthOrientation Map::northOrientation() const {
    return static_cast<Map::NorthOrientation>(d_ptr->mapObj->getMapOptions().northOrientation());
}

/*!
    \brief Set the north orientation mode.
    \param orientation The north orientation mode.
*/
void Map::setNorthOrientation(NorthOrientation orientation) {
    d_ptr->mapObj->setNorthOrientation(static_cast<mbgl::NorthOrientation>(orientation));
}

/*!
    \brief Set gesture in progress status.
    \param progress The gesture in progress status.

    Tells the map rendering engine that there is currently a gesture in progress.
    This affects how the map renders labels, as it will use different textur
    filters if a gesture is ongoing.
*/
void Map::setGestureInProgress(bool progress) {
    d_ptr->mapObj->setGestureInProgress(progress);
}

/*!
    \brief Set transition options.
    \param duration The transition duration in milliseconds.
    \param delay The transition delay in milliseconds.

    Style paint property values transition to new values with animation when
    they are updated.
*/
void Map::setTransitionOptions(qint64 duration, qint64 delay) {
    static auto convert = [](qint64 value) -> std::optional<mbgl::Duration> {
        return std::chrono::duration_cast<mbgl::Duration>(mbgl::Milliseconds(value));
    };

    d_ptr->mapObj->getStyle().setTransitionOptions(mbgl::style::TransitionOptions{convert(duration), convert(delay)});
}

/*!
    \brief Add an annotation icon to the map.
    \param name The icon name.
    \param sprite The icon image.

    Adds an \a sprite to the annotation icon pool. This can be later used by the annotation
    functions to shown any drawing on the map by referencing its \a name.

    Unlike using addIcon() for runtime styling, annotations added with addAnnotation()
    will survive style changes.

    \sa addAnnotation()
*/
void Map::addAnnotationIcon(const QString &name, const QImage &sprite) {
    if (sprite.isNull()) {
        return;
    }

    d_ptr->mapObj->addAnnotationImage(toStyleImage(name, sprite));
}

/*!
    \brief Add an annotation to the map.
    \param annotation The annotation to add.
    \return The unique identifier for the new annotation.

    \sa addAnnotationIcon()
*/
AnnotationID Map::addAnnotation(const Annotation &annotation) {
    std::optional<mbgl::Annotation> a = asAnnotation(annotation);
    if (!a) {
        return 0;
    }

    return static_cast<AnnotationID>(d_ptr->mapObj->addAnnotation(*a));
}

/*!
    \brief Update an existing annotation.
    \param id The unique identifier for the annotation to update.
    \param annotation The updated annotation.

    \sa addAnnotationIcon()
*/
void Map::updateAnnotation(AnnotationID id, const Annotation &annotation) {
    std::optional<mbgl::Annotation> a = asAnnotation(annotation);
    if (!a) {
        return;
    }

    d_ptr->mapObj->updateAnnotation(id, *a);
}

/*!
    \brief Remove an existing annotation.
    \param id The unique identifier for the annotation to remove.
*/
void Map::removeAnnotation(AnnotationID id) {
    d_ptr->mapObj->removeAnnotation(id);
}

/*!
    \brief Set layer layout property.
    \param layerId The layer identifier.
    \param propertyName The layout property name.
    \param value The layout property value.
    \return \c true if the operation succeeds, \c false otherwise.

    Sets a layout property value to an existing layer. The property string can
    be any as defined by the <a href="https://maplibre.org/maplibre-style-spec/">
    MapLibre Style Spec</a> for layout properties.

    The implementation attempts to treat value as a JSON string, if the
    \c QVariant inner type is a string. If not a valid JSON string, then it'll
    proceed with the mapping described below.

    This example hides the layer \c route:

    \code
        map->setLayoutProperty("route", "visibility", "none");
    \endcode

    This table describes the mapping between <a href="https://maplibre.org/maplibre-style-spec/#types">
    style types</a> and Qt types accepted by setLayoutProperty():

    MapLibre style type  |  Qt type
    ---------------------|---------------------
    Enum                 | \c QString
    String               | \c QString
    Boolean              | \c bool
    Number               | \c int, \c double or \c float
    Array                | \c QVariantList
*/
bool Map::setLayoutProperty(const QString &layerId, const QString &propertyName, const QVariant &value) {
    return d_ptr->setProperty(&mbgl::style::Layer::setProperty, layerId, propertyName, value);
}

/*!
    \brief Set layer paint property.
    \param layerId The layer identifier.
    \param propertyName The paint property name.
    \param value The paint property value.
    \return \c true if the operation succeeds, \c false otherwise.

    Sets a layout property value to an existing layer. The property string can
    be any as defined by the <a href="https://maplibre.org/maplibre-style-spec/">
    MapLibre Style Spec</a> for paint properties.

    The implementation attempts to treat value as a JSON string, if the
    \c QVariant inner type is a string. If not a valid JSON string, then it'll
    proceed with the mapping described below.

    For paint properties that take a color as value, such as fill-color,
    a string such as \c blue can be passed or a \c QColor.

    \code
        map->setPaintProperty("route", "line-color", QColor("blue"));
    \endcode

    This table describes the mapping between <a href="https://maplibre.org/maplibre-style-spec/#types">
    style types</a> and Qt types accepted by setPaintProperty():

    MapLibre style type  |  Qt type
    ---------------------|---------------------
    Color                | \c QString or \c QColor
    Enum                 | \c QString
    String               | \c QString
    Boolean              | \c bool
    Number               | \c int, \c double or \c float
    Array                | \c QVariantList

    If the style specification defines the property's type as \b Array,
    use a \c QVariantList. For example, the following code sets a \c route
    layer's \c line-dasharray property:

    \code
        QVariantList lineDashArray;
        lineDashArray.append(1);
        lineDashArray.append(2);

        map->setPaintProperty("route","line-dasharray", lineDashArray);
    \endcode
*/
bool Map::setPaintProperty(const QString &layerId, const QString &propertyName, const QVariant &value) {
    return d_ptr->setProperty(&mbgl::style::Layer::setProperty, layerId, propertyName, value);
}

/*!
    \brief Get loaded state.
    \return \c true if the map is completely rendered, \c false otherwise.

    A partially rendered map ranges from nothing rendered at all to only labels missing.
*/
bool Map::isFullyLoaded() const {
    return d_ptr->mapObj->isFullyLoaded();
}

/*!
    \brief Pan the map by an offset.
    \param offset The offset in pixels.

    The pixel coordinate origin is located at the upper left corner of the map.
*/
void Map::moveBy(const QPointF &offset) {
    d_ptr->mapObj->moveBy(mbgl::ScreenCoordinate{offset.x(), offset.y()});
}

/*!
    \brief Scale the map.
    \param scale The scale factor.
    \param center The direction pixel coordinate.

    Scale the map by \a scale in the direction of the \a center.
    This function can be used for implementing a pinch gesture.
*/
void Map::scaleBy(double scale, const QPointF &center) {
    d_ptr->mapObj->scaleBy(scale, mbgl::ScreenCoordinate{center.x(), center.y()});
}

/*!
    \brief Rotate the map.
    \param first The first screen coordinate.
    \param second The second screen coordinate.

    Rotate the map from the \a first screen coordinate to the \a second screen coordinate.
    This method can be used for implementing rotating the map by clicking and dragging,
    being \a first the cursor coordinate at the last frame and \a second the cursor coordinate
    at the current frame.
*/
void Map::rotateBy(const QPointF &first, const QPointF &second) {
    d_ptr->mapObj->rotateBy(mbgl::ScreenCoordinate{first.x(), first.y()},
                            mbgl::ScreenCoordinate{second.x(), second.y()});
}

/*!
    \brief Resize the map.
    \param size The new size.
    \param pixelRatio The pixel ratio of the device.

    Resize the map to \a size and scale to fit at the framebuffer.
    For high DPI screens, the size will be smaller than the framebuffer.

    \a pixelRatio is optional and defaults to the currently set one.
*/
void Map::resize(const QSize &size, qreal pixelRatio) {
    const auto sanitizedSize = sanitizeSize(size);
    if (d_ptr->mapObj->getMapOptions().size() != sanitizedSize) {
        d_ptr->mapObj->setSize(sanitizedSize);
    }

    const qreal currentPixelRatio = pixelRatio > 0 ? pixelRatio : d_ptr->mapObj->getMapOptions().pixelRatio();

    // Always update the backend renderer size (works for all backends: OpenGL, Metal, Vulkan)
    updateRenderer(size, currentPixelRatio);
}

/*!
    \brief Get pixel for coordinate.
    \param coordinate The geographic coordinate.
    \return The pixel coordinate.

    Returns the offset in pixels for \a coordinate. The origin pixel coordinate is
    located at the top left corner of the map view.

    This method returns the correct value for any coordinate, even if the coordinate
    is not currently visible on the screen.

    \note The return value is affected by the current zoom level, bearing and pitch.
*/
QPointF Map::pixelForCoordinate(const Coordinate &coordinate) const {
    const mbgl::ScreenCoordinate pixel = d_ptr->mapObj->pixelForLatLng(
        mbgl::LatLng{coordinate.first, coordinate.second});

    return {pixel.x, pixel.y};
}

/*!
    \brief Get coordinate for pixel.
    \param pixel The pixel coordinate.
    \return The geographic coordinate.

    Returns the geographic coordinate for the \a pixel coordinate.
*/
Coordinate Map::coordinateForPixel(const QPointF &pixel) const {
    const mbgl::LatLng latLng = d_ptr->mapObj->latLngForPixel(mbgl::ScreenCoordinate{pixel.x(), pixel.y()});

    return {latLng.latitude(), latLng.longitude()};
}

/*!
    \brief Get the coordinate and zoom for required bounds.
    \param sw The southwest coordinate.
    \param ne The northeast coordinate.
    \return The coordinate and zoom combination.

    Returns the coordinate and zoom combination needed in order to make the coordinate
    bounding box \a sw and \a ne visible.
*/
CoordinateZoom Map::coordinateZoomForBounds(const Coordinate &sw, const Coordinate &ne) const {
    auto bounds = mbgl::LatLngBounds::hull(mbgl::LatLng{sw.first, sw.second}, mbgl::LatLng{ne.first, ne.second});
    mbgl::CameraOptions camera = d_ptr->mapObj->cameraForLatLngBounds(bounds, d_ptr->margins);
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    return {{(*camera.center).latitude(), (*camera.center).longitude()}, *camera.zoom};
}

/*!
    \brief Get the coordinate and zoom for required bounds with updated bearing and pitch.
    \param sw The southwest coordinate.
    \param ne The northeast coordinate.
    \param newBearing The updated bearing.
    \param newPitch The updated pitch.
    \return The coordinate and zoom combination.

    Returns the coordinate and zoom combination needed in order to make the coordinate
    bounding box \a sw and \a ne visible taking into account \a newBearing and \a newPitch.
*/
CoordinateZoom Map::coordinateZoomForBounds(const Coordinate &sw,
                                            const Coordinate &ne,
                                            double newBearing,
                                            double newPitch)

{
    auto bounds = mbgl::LatLngBounds::hull(mbgl::LatLng{sw.first, sw.second}, mbgl::LatLng{ne.first, ne.second});
    mbgl::CameraOptions camera = d_ptr->mapObj->cameraForLatLngBounds(bounds, d_ptr->margins, newBearing, newPitch);
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    return {{(*camera.center).latitude(), (*camera.center).longitude()}, *camera.zoom};
}

/*!
    \property Map::margins
    \brief the map margins in pixels from the corners of the map.
    \sa margins()
    \sa setMargins()
*/

/*!
    \brief Get the map margins.
    \return The map margins in pixels.
*/
QMargins Map::margins() const {
    return {static_cast<int>(d_ptr->margins.left()),
            static_cast<int>(d_ptr->margins.top()),
            static_cast<int>(d_ptr->margins.right()),
            static_cast<int>(d_ptr->margins.bottom())};
}

/*!
    \brief Set the map margins.
    \param margins The map margins in pixels.

    This sets a new reference center for the map.
*/
void Map::setMargins(const QMargins &margins) {
    d_ptr->margins = {static_cast<double>(margins.top()),
                      static_cast<double>(margins.left()),
                      static_cast<double>(margins.bottom()),
                      static_cast<double>(margins.right())};
}

/*!
    \brief Add a style source.
    \param id The source identifier.
    \param params The source parameters.

    Adds a source with \a id to the map as specified by the
    <a href="https://maplibre.org/maplibre-style-spec/#root-sources">MapLibre Style Specification</a>
    with parameters.

    This example reads a GeoJSON from the Qt resource system and adds it as source:

    \code
        QFile geojson(":source1.geojson");
        geojson.open(QIODevice::ReadOnly);

        QVariantMap routeSource;
        routeSource["type"] = "geojson";
        routeSource["data"] = geojson.readAll();

        map->addSource("routeSource", routeSource);
    \endcode
*/
void Map::addSource(const QString &id, const QVariantMap &params) {
    mbgl::style::conversion::Error error;
    std::optional<std::unique_ptr<mbgl::style::Source>> source =
        mbgl::style::conversion::convert<std::unique_ptr<mbgl::style::Source>>(
            QVariant(params), error, id.toStdString());
    if (!source) {
        qWarning() << "Unable to add source with id" << id << ":" << error.message.c_str();
        return;
    }

    d_ptr->mapObj->getStyle().addSource(std::move(*source));
}

/*!
    \brief Check if a style source exists.
    \param id The source identifier.
    \return \c true if the layer with given \a id exists, \c false otherwise.
*/
bool Map::sourceExists(const QString &id) {
    return d_ptr->mapObj->getStyle().getSource(id.toStdString()) != nullptr;
}

/*!
    \brief Update a style source.
    \param id The source identifier.
    \param params The source parameters.

    If the source does not exist, it will be added like in addSource(). Only
    image and GeoJSON sources can be updated.
*/
void Map::updateSource(const QString &id, const QVariantMap &params) {
    mbgl::style::Source *source = d_ptr->mapObj->getStyle().getSource(id.toStdString());
    if (source == nullptr) {
        addSource(id, params);
        return;
    }

    auto *sourceGeoJSON = source->as<mbgl::style::GeoJSONSource>();
    auto *sourceImage = source->as<mbgl::style::ImageSource>();
    if (sourceGeoJSON == nullptr && sourceImage == nullptr) {
        qWarning() << "Unable to update source: only GeoJSON and Image sources are mutable.";
        return;
    }

    if (sourceImage != nullptr) {
        if (params.contains("url")) {
            sourceImage->setURL(params["url"].toString().toStdString());
        }
        if (params.contains("coordinates") && params["coordinates"].toList().size() == 4) {
            mbgl::style::conversion::Error error;
            std::array<mbgl::LatLng, 4> coordinates;
            for (std::size_t i = 0; i < 4; i++) {
                auto latLng = mbgl::style::conversion::convert<mbgl::LatLng>(
                    params["coordinates"].toList()[static_cast<int>(i)], error);
                if (latLng) {
                    coordinates[i] = *latLng; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
                }
            }
            sourceImage->setCoordinates(coordinates);
        }
    } else if (sourceGeoJSON != nullptr && params.contains("data")) {
        mbgl::style::conversion::Error error;
        auto result = mbgl::style::conversion::convert<mbgl::GeoJSON>(params["data"], error);
        if (result) {
            sourceGeoJSON->setGeoJSON(*result);
        }
    }
}

/*!
    \brief Remove a style source.
    \param id The source identifier.

    This method has no effect if the style source does not exist.
*/
void Map::removeSource(const QString &id) {
    auto idStdString = id.toStdString();

    if (d_ptr->mapObj->getStyle().getSource(idStdString) != nullptr) {
        d_ptr->mapObj->getStyle().removeSource(idStdString);
    }
}

/*!
    \brief Add a custom style layer.
    \param id The layer identifier.
    \param host The custom layer host interface.
    \param before The layer identifier before which the new layer will be added.

    Adds a custom layer with \a id with the custom layer host interface \a host
    before the existing layer \a before.

    \warning This is used for delegating the rendering of a layer to the user of
    this API and is not officially supported. Use at your own risk.
*/
void Map::addCustomLayer(const QString &id, std::unique_ptr<CustomLayerHostInterface> host, const QString &before) {
    class HostWrapper final : public mbgl::style::CustomLayerHost {
    public:
        std::unique_ptr<CustomLayerHostInterface> ptr;
        explicit HostWrapper(std::unique_ptr<CustomLayerHostInterface> p)
            : ptr(std::move(p)) {}

        void initialize() final { ptr->initialize(); }

        void render(const mbgl::style::CustomLayerRenderParameters &params) final {
            CustomLayerRenderParameters renderParams{};
            renderParams.width = params.width;
            renderParams.height = params.height;
            renderParams.latitude = params.latitude;
            renderParams.longitude = params.longitude;
            renderParams.zoom = params.zoom;
            renderParams.bearing = params.bearing;
            renderParams.pitch = params.pitch;
            renderParams.fieldOfView = params.fieldOfView;
            ptr->render(renderParams);
        }

        void contextLost() final {}

        void deinitialize() final { ptr->deinitialize(); }
    };

    d_ptr->mapObj->getStyle().addLayer(
        std::make_unique<mbgl::style::CustomLayer>(id.toStdString(), std::make_unique<HostWrapper>(std::move(host))),
        before.isEmpty() ? std::optional<std::string>() : std::optional<std::string>(before.toStdString()));
}

/*!
    \brief Add a style layer.
    \param id The layer identifier.
    \param params The layer parameters.
    \param before The layer identifier before which the new layer will be added.

    Adds a style layer to the map as specified by the
    <a href="https://maplibre.org/maplibre-style-spec/#root-layers">MapLibre style specification</a>
    with parameters. The layer will be added under the layer specified by \a before,
    if specified. Otherwise it will be added as the topmost layer.

    This example shows how to add a layer that will be used to show a route line on the map. Note
    that nothing will be drawn until we set paint properties using setPaintProperty().

    \code
        QVariantMap route;
        route["type"] = "line";
        route["source"] = "routeSource";

        map->addLayer("route", route);
    \endcode

    \note The source must exist prior to adding a layer.
*/
void Map::addLayer(const QString &id, const QVariantMap &params, const QString &before) {
    QVariantMap parameters = params;
    parameters["id"] = id;

    mbgl::style::conversion::Error error;
    std::optional<std::unique_ptr<mbgl::style::Layer>> layer =
        mbgl::style::conversion::convert<std::unique_ptr<mbgl::style::Layer>>(QVariant(parameters), error);
    if (!layer) {
        qWarning() << "Unable to add layer with id" << id << ":" << error.message.c_str();
        return;
    }

    d_ptr->mapObj->getStyle().addLayer(
        std::move(*layer),
        before.isEmpty() ? std::optional<std::string>() : std::optional<std::string>(before.toStdString()));
}

/*!
    \brief Check if a style layer exists.
    \param id The layer identifier.
    \return \c true if the layer with given \a id exists, \c false otherwise.
*/
bool Map::layerExists(const QString &id) {
    return d_ptr->mapObj->getStyle().getLayer(id.toStdString()) != nullptr;
}

/*!
    \brief Remove a style layer.
    \param id The layer identifier.

    This method has no effect if the style layer does not exist.
*/
void Map::removeLayer(const QString &id) {
    d_ptr->mapObj->getStyle().removeLayer(id.toStdString());
}

/*!
    \brief List all existing layers.
    \return List of all existing layer IDs from the current style.
*/
QVector<QString> Map::layerIds() const {
    const auto &layers = d_ptr->mapObj->getStyle().getLayers();

    QVector<QString> layerIds;
    layerIds.reserve(static_cast<int>(layers.size()));

    for (const mbgl::style::Layer *layer : layers) {
        layerIds.append(QString::fromStdString(layer->getID()));
    }

    return layerIds;
}

/*!
    \brief Add a style image.
    \param id The image identifier.
    \param sprite The image.

    Adds the \a sprite with the identifier \a id that can be used
    later by a symbol layer.

    If the \a id was already added, it gets replaced by the new
    \a image only if the dimensions of the image are the same as
    the old image, otherwise it has no effect.

    \sa addLayer()
*/
void Map::addImage(const QString &id, const QImage &sprite) {
    if (sprite.isNull()) {
        return;
    }

    d_ptr->mapObj->getStyle().addImage(toStyleImage(id, sprite));
}

/*!
    \brief Remove a style image.
    \param id The image identifier.
*/
void Map::removeImage(const QString &id) {
    d_ptr->mapObj->getStyle().removeImage(id.toStdString());
}

/*!
    \brief Set a style filter to a layer.
    \param layerId The layer identifier.
    \param filter The filter expression.

    Adds a \a filter to a style \a layer using the format described in the
    <a href="https://maplibre.org/maplibre-style-spec/deprecations/#other-filter">MapLibre Style Spec</a>.

    Given a layer \c marker from an arbitrary GeoJSON source containing features
    of type \b "Point" and \b "LineString", this example shows how to make sure
    the layer will only tag features of type \b "Point".

    \code
        QVariantList filterExpression;
        filterExpression.push_back(QLatin1String("=="));
        filterExpression.push_back(QLatin1String("$type"));
        filterExpression.push_back(QLatin1String("Point"));

        QVariantList filter;
        filter.push_back(filterExpression);

        map->setFilter(QLatin1String("marker"), filter);
    \endcode
*/
void Map::setFilter(const QString &layerId, const QVariant &filter) {
    mbgl::style::Layer *layer = d_ptr->mapObj->getStyle().getLayer(layerId.toStdString());
    if (layer == nullptr) {
        qWarning() << "Layer not found:" << layerId;
        return;
    }

    if (filter.isNull() || filter.toList().isEmpty()) {
        layer->setFilter(mbgl::style::Filter());
        return;
    }

    mbgl::style::conversion::Error error;
    std::optional<mbgl::style::Filter> converted = mbgl::style::conversion::convert<mbgl::style::Filter>(filter, error);
    if (!converted) {
        qWarning() << "Error parsing filter:" << error.message.c_str();
        return;
    }

    layer->setFilter(*converted);
}

/*!
    \brief Get a style filter of a layer.
    \param layerId The layer identifier.
    \return The filter expression.

    Returns the current \a expression-based filter value applied to a layer
    with ID \a layerId, if any.

    Filter value types are described in the
    <a href="https://maplibre.org/maplibre-style-spec/types/">MapLibre Style Spec</a>.
*/
QVariant Map::getFilter(const QString &layerId) const {
    const mbgl::style::Layer *layer = d_ptr->mapObj->getStyle().getLayer(layerId.toStdString());
    if (layer == nullptr) {
        qWarning() << "Layer not found:" << layerId;
        return {};
    }

    auto serialized = layer->getFilter().serialize();
    return variantFromValue(serialized);
}

/*!
    \brief Create the renderer.
    \param nativeTargetPtr The pointer to the native layer/window/surface.

    Creates the infrastructure needed for rendering the map. It
    should be called before any call to render().

    // Metal-specific: create the renderer using a pre-existing CAMetalLayer.
    // Vulkan-specific: create the renderer using a Qt Quick window.

    Must be called on the render thread.
*/
void Map::createRenderer(void *nativeTargetPtr) {
    d_ptr->createRenderer(nativeTargetPtr);
}

#ifdef MLN_RENDER_BACKEND_VULKAN
void Map::createRendererWithQtVulkanDevice(void *windowPtr,
                                           void *physicalDevice,
                                           void *device,
                                           uint32_t graphicsQueueIndex) {
    d_ptr->createRendererWithQtVulkanDevice(windowPtr, physicalDevice, device, graphicsQueueIndex);
}
#endif

/*!
    \brief Destroy the renderer.

    Destroys the infrastructure needed for rendering the map,
    releasing resources.

    Must be called on the render thread.
*/
void Map::destroyRenderer() {
    d_ptr->destroyRenderer();
}

/*!
    \brief Start the static renderer.

    Start a static rendering of the current state of the map.
    This should only be called when the map is initialized in static mode.

    \sa Settings::MapMode
*/
void Map::startStaticRender() {
    d_ptr->mapObj->renderStill([this](const std::exception_ptr &err) {
        QString what;

        try {
            if (err) {
                std::rethrow_exception(err);
            }
        } catch (const std::exception &e) {
            what = e.what();
        }

        emit staticRenderFinished(what);
    });
}

/*!
    \brief Render.

    Renders the map using native draw calls.

    This function should be called only after the signal needsRendering() is
    emitted at least once.

    Must be called on the render thread.
*/
void Map::render() {
    d_ptr->render();
}

/*!
    \brief Trigger repaint.

    Trigger repaint so that render can be called.
*/
void Map::triggerRepaint() {
    d_ptr->mapObj->triggerRepaint();
}

/*!
    \brief Update the framebuffer size for the rendering backend.
    \param size The framebuffer size.
    \param pixelRatio The pixel ratio of the screen.
    \param fbo The framebuffer object ID (OpenGL only, ignored for Metal/Vulkan).

    The \a size is the size of the display surface, which on high DPI screens
    is usually smaller than the rendered map size.

    Updates the framebuffer configuration for the current rendering backend.
    For OpenGL: The \a fbo parameter specifies the framebuffer object ID to use.
    For Metal/Vulkan: The \a fbo parameter is ignored (pass 0).

    Must be called on the render thread.
*/
void Map::updateRenderer(const QSize &size, qreal pixelRatio, quint32 fbo) {
    d_ptr->updateRenderer(size, pixelRatio, fbo);
}

/*!
    \brief Set connection established.

    Informs the map that the network connection has been established, causing
    all network requests that previously timed out to be retried immediately.
*/
void Map::setConnectionEstablished() {
    mbgl::NetworkStatus::Reachable();
}

/*!
    \brief Returns the native color texture for backend-specific rendering.

    This method provides access to the underlying native texture used by Metal
    and Vulkan backends for Qt Quick integration. Returns nullptr if no texture
    is available or if the backend doesn't support native texture access.

    \return A pointer to the native texture, or nullptr if unavailable.
    \note This is backend-specific and primarily used for Qt Quick texture nodes.
*/
void *Map::nativeColorTexture() const {
#if defined(MLN_RENDER_BACKEND_METAL) || defined(MLN_RENDER_BACKEND_VULKAN)
    return d_ptr->currentDrawableTexture();
#else
    return nullptr;
#endif
}

/*!
    \brief Sets the current drawable texture for backend-specific rendering.

    This method allows external code to provide a drawable texture that the
    renderer can use. The texture pointer interpretation is backend-specific:
    - For Metal: CAMetalDrawable object
    - For Vulkan/OpenGL: Implementation-specific texture handle

    \param texturePtr Pointer to the backend-specific drawable texture.
    \note This is primarily used internally by the rendering backends.
*/
void Map::setCurrentDrawable(void *texturePtr) {
    d_ptr->setCurrentDrawable(texturePtr);
}

/*!
    \brief Sets the external drawable texture rendering on surfaces we do not own.

    This is primarily used for QRhiWidget integration.

    \param texturePtr Pointer to the backend-specific drawable texture.
    \param textureSize The size of the drawable texture.
    \note This is primarily used internally by the rendering backends.
*/
void Map::setExternalDrawable(void *texturePtr, const QSize &textureSize) {
    d_ptr->setExternalDrawable(texturePtr, textureSize);
}

#ifdef MLN_RENDER_BACKEND_VULKAN
mbgl::vulkan::Texture2D *Map::getVulkanTexture() const {
    return d_ptr->getVulkanTexture();
}
#endif

#ifdef MLN_RENDER_BACKEND_OPENGL
unsigned int Map::getFramebufferTextureId() const {
    return d_ptr->getFramebufferTextureId();
}
#endif

/*!
    \fn void Map::needsRendering()
    \brief Signal emitted when the rendering is needed.

    This signal is emitted when the visual contents of the map have changed
    and a redraw is needed in order to keep the map visually consistent
    with the current state.

    \sa render()
*/

/*!
    \fn void Map::staticRenderFinished(const QString &error)
    \brief Signal emitted when a static map is fully drawn.
    \param error The error message.

    This signal is emitted when a static map is fully drawn. Usually the next
    step is to extract the map from a framebuffer into a container like a
    \c QImage. \a error is set to a message when an error occurs.

    \sa startStaticRender()
*/

/*!
    \fn void Map::mapChanged(Map::MapChange change)
    \brief Signal emitted when the map has changed.
    \param change The map change mode.

    This signal is emitted when the state of the map has changed. This signal
    may be used for detecting errors when loading a style or detecting when
    a map is fully loaded by analyzing the parameter \a change.
*/

/*!
    \fn void Map::mapLoadingFailed(Map::MapLoadingFailure type, const QString &description)
    \brief Signal emitted when a map loading failure happens.
    \param type The type of failure.
    \param description The description of the failure.

    This signal is emitted when a map loading failure happens. Details of the
    failures are provided, including its \a type and textual \a description.
*/

/*!
    \fn void Map::copyrightsChanged(const QString &copyrightsHtml);
    \brief Signal emitted when the copyrights have changed.
    \param copyrightsHtml The HTML of the copyrights.

    This signal is emitted when the copyrights of the current content of the map
    have changed. This can be caused by a style change or adding a new source.
*/

/*! \cond PRIVATE */

MapPrivate::MapPrivate(Map *map, const Settings &settings, const QSize &size, qreal pixelRatio_)
    : QObject(map),
      m_mode(settings.contextMode()),
      m_pixelRatio(pixelRatio_),
      m_localFontFamily(settings.localFontFamily()) {
    // Setup MapObserver
    m_mapObserver = std::make_unique<MapObserver>(this);

    qRegisterMetaType<Map::MapChange>("Map::MapChange");

    connect(m_mapObserver.get(), &MapObserver::mapChanged, map, &Map::mapChanged);
    connect(m_mapObserver.get(), &MapObserver::mapLoadingFailed, map, &Map::mapLoadingFailed);
    connect(m_mapObserver.get(), &MapObserver::copyrightsChanged, map, &Map::copyrightsChanged);

    auto resourceOptions = resourceOptionsFromSettings(settings);
    auto clientOptions = clientOptionsFromSettings(settings);

    // Setup the Map object.
    mapObj = std::make_unique<mbgl::Map>(*this,
                                         *m_mapObserver,
                                         mapOptionsFromSettings(settings, size, m_pixelRatio),
                                         resourceOptions,
                                         clientOptionsFromSettings(settings));

    if (settings.resourceTransform()) {
        m_resourceTransform = std::make_unique<mbgl::Actor<mbgl::ResourceTransform::TransformCallback>>(
            *mbgl::Scheduler::GetCurrent(),
            [callback = settings.resourceTransform()](mbgl::Resource::Kind,
                                                      const std::string &url_,
                                                      const mbgl::ResourceTransform::FinishedCallback &onFinished) {
                onFinished(callback(url_));
            });

        mbgl::ResourceTransform transform{[actorRef = m_resourceTransform->self()](
                                              mbgl::Resource::Kind kind,
                                              const std::string &url,
                                              mbgl::ResourceTransform::FinishedCallback onFinished) {
            actorRef.invoke(&mbgl::ResourceTransform::TransformCallback::operator(), kind, url, std::move(onFinished));
        }};
        const std::shared_ptr<mbgl::FileSource> fs = mbgl::FileSourceManager::get()->getFileSource(
            mbgl::FileSourceType::Network, resourceOptions, clientOptions);
        fs->setResourceTransform(std::move(transform));
    }

    // Needs to be Queued to give time to discard redundant draw calls via the `renderQueued` flag.
    connect(this, &MapPrivate::needsRendering, map, &Map::needsRendering, Qt::QueuedConnection);
}

MapPrivate::~MapPrivate() = default;

void MapPrivate::update(std::shared_ptr<mbgl::UpdateParameters> parameters) {
    const std::scoped_lock lock(m_mapRendererMutex);

    m_updateParameters = std::move(parameters);

    if (m_mapRenderer == nullptr) {
        return;
    }

    m_mapRenderer->updateParameters(std::move(m_updateParameters));

    requestRendering();
}

void MapPrivate::setObserver(mbgl::RendererObserver &observer) {
    m_rendererObserver = std::make_unique<RendererObserver>(*mbgl::util::RunLoop::Get(), observer);

    const std::scoped_lock lock(m_mapRendererMutex);

    if (m_mapRenderer != nullptr) {
        m_mapRenderer->setObserver(m_rendererObserver.get());
    }
}

void MapPrivate::createRenderer(void *nativeTargetPtr) {
    const std::scoped_lock lock(m_mapRendererMutex);

    if (m_mapRenderer != nullptr) {
        return;
    }

    m_mapRenderer = std::make_unique<MapRenderer>(m_pixelRatio, m_mode, m_localFontFamily, nativeTargetPtr);

    connect(m_mapRenderer.get(), &MapRenderer::needsRendering, this, &MapPrivate::requestRendering);

    m_mapRenderer->setObserver(m_rendererObserver.get());

    // Propagate current map size to the renderer
    if (mapObj) {
        auto currentSize = mapObj->getMapOptions().size();
        auto currentPixelRatio = mapObj->getMapOptions().pixelRatio();
        m_mapRenderer->updateRenderer(currentSize, currentPixelRatio);
    }

    if (m_updateParameters != nullptr) {
        m_mapRenderer->updateParameters(m_updateParameters);
        requestRendering();
    }
}

#ifdef MLN_RENDER_BACKEND_VULKAN
void MapPrivate::createRendererWithQtVulkanDevice(void *windowPtr,
                                                  void *physicalDevice,
                                                  void *device,
                                                  uint32_t graphicsQueueIndex) {
    const std::scoped_lock lock(m_mapRendererMutex);

    if (m_mapRenderer != nullptr) {
        return; // already created
    }

    m_mapRenderer = std::make_unique<MapRenderer>(
        m_pixelRatio, m_mode, m_localFontFamily, windowPtr, physicalDevice, device, graphicsQueueIndex);

    connect(m_mapRenderer.get(), &MapRenderer::needsRendering, this, &MapPrivate::requestRendering);

    m_mapRenderer->setObserver(m_rendererObserver.get());

    if (mapObj) {
        auto currentSize = mapObj->getMapOptions().size();
        auto currentPixelRatio = mapObj->getMapOptions().pixelRatio();
        m_mapRenderer->updateRenderer(currentSize, currentPixelRatio);
    }

    if (m_updateParameters != nullptr) {
        m_mapRenderer->updateParameters(m_updateParameters);
        requestRendering();
    }
}
#endif

void MapPrivate::destroyRenderer() {
    const std::scoped_lock lock(m_mapRendererMutex);

    m_mapRenderer.reset();
}

void MapPrivate::render() {
    const std::scoped_lock lock(m_mapRendererMutex);

#ifdef MLN_RENDERER_DEBUGGING
    qDebug() << "MapPrivate::render() - Called";
#endif

    if (m_mapRenderer == nullptr) {
#ifdef MLN_RENDERER_DEBUGGING
        qDebug() << "MapPrivate::render() - MapRenderer is null, not rendering";
#endif
        return;
    }

#ifdef MLN_RENDERER_DEBUGGING
    qDebug() << "MapPrivate::render() - Clearing render queue and rendering";
#endif

    m_renderQueued.clear();
    m_mapRenderer->render();

#ifdef MLN_RENDERER_DEBUGGING
    qDebug() << "MapPrivate::render() - Completed";
#endif
}

void MapPrivate::updateRenderer(const QSize &size, qreal pixelRatio, quint32 fbo) {
    const std::scoped_lock lock(m_mapRendererMutex);

    if (m_mapRenderer == nullptr) {
#ifdef MLN_RENDERER_DEBUGGING
        qDebug() << "MapPrivate::updateRenderer() - MapRenderer is null, skipping renderer update";
#endif
        return;
    }

    // Need to add pixel ratio to the size, as the renderer expects the full size
    m_mapRenderer->updateRenderer(sanitizeSize(size), pixelRatio, fbo);
}

void MapPrivate::requestRendering() {
    if (!m_renderQueued.test_and_set()) {
        emit needsRendering();
    }
}

bool MapPrivate::setProperty(const PropertySetter &setter,
                             const QString &layerId,
                             const QString &name,
                             const QVariant &value) const {
    mbgl::style::Layer *layer = mapObj->getStyle().getLayer(layerId.toStdString());
    if (layer == nullptr) {
        qWarning() << "Layer not found:" << layerId;
        return false;
    }

    const std::string &propertyString = name.toStdString();

    std::optional<mbgl::style::conversion::Error> result;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (value.typeId() == QMetaType::QString) {
#else
    if (value.type() == QVariant::String) {
#endif
        mbgl::JSDocument document;
        document.Parse<0>(value.toString().toStdString());
        if (!document.HasParseError()) {
            // Treat value as a valid JSON.
            const mbgl::JSValue *jsonValue = &document;
            result = (layer->*setter)(propertyString, jsonValue);
        } else {
            result = (layer->*setter)(propertyString, value);
        }
    } else {
        result = (layer->*setter)(propertyString, value);
    }

    if (result) {
        qWarning() << "Error setting property" << name << "on layer" << layerId << ":"
                   << QString::fromStdString(result->message);
        return false;
    }

    return true;
}

void *MapPrivate::currentDrawableTexture() const {
    const std::scoped_lock lock(m_mapRendererMutex);
    return m_mapRenderer ? m_mapRenderer->currentDrawableTexture() : nullptr;
}

void MapPrivate::setCurrentDrawable(void *tex) {
    const std::scoped_lock lock(m_mapRendererMutex);
    if (m_mapRenderer) {
        m_mapRenderer->setCurrentDrawable(tex);
    }
}

void MapPrivate::setExternalDrawable(void *tex, const QSize &size) {
    const std::scoped_lock lock(m_mapRendererMutex);
    if (m_mapRenderer) {
        const mbgl::Size sanitizedSize = sanitizeSize(size);
        m_mapRenderer->setExternalDrawable(tex, sanitizedSize);
    }
}

#ifdef MLN_RENDER_BACKEND_VULKAN
mbgl::vulkan::Texture2D *MapPrivate::getVulkanTexture() const {
    const std::scoped_lock lock(m_mapRendererMutex);
    return m_mapRenderer ? m_mapRenderer->getVulkanTexture() : nullptr;
}
#endif

#ifdef MLN_RENDER_BACKEND_OPENGL
unsigned int MapPrivate::getFramebufferTextureId() const {
    const std::scoped_lock lock(m_mapRendererMutex);
    return m_mapRenderer ? m_mapRenderer->getFramebufferTextureId() : 0;
}
#endif

/*! \endcond */

} // namespace QMapLibre
