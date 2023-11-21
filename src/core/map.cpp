// Copyright (C) 2023 MapLibre contributors
// Copyright (C) 2020 Mapbox, Inc.

// SPDX-License-Identifier: BSD-2-Clause

#include "map.hpp"
#include "map_p.hpp"

#include "conversion_p.hpp"
#include "geojson_p.hpp"
#include "map_observer_p.hpp"
#include "renderer_observer_p.hpp"

#include <mbgl/actor/scheduler.hpp>
#include <mbgl/annotation/annotation.hpp>
#include <mbgl/gl/custom_layer.hpp>
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
#include <mbgl/util/projection.hpp>
#include <mbgl/util/rapidjson.hpp>
#include <mbgl/util/run_loop.hpp>
#include <mbgl/util/tile_server_options.hpp>
#include <mbgl/util/traits.hpp>

#include <QtCore/QDebug>
#include <QtCore/QThreadStorage>
#include <QtCore/QVariant>
#include <QtCore/QVariantList>
#include <QtCore/QVariantMap>
#include <QtGui/QColor>

#include <functional>
#include <memory>
#include <sstream>

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

mbgl::Size sanitizedSize(const QSize &size) {
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
                         .withSize(sanitizedSize(size))
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
    \class QMapLibre::Map
    \brief The QMapLibre::Map class is a Qt wrapper for the MapLibre Native engine.

    \inmodule MapLibre Maps SDK for Qt

    QMapLibre::Map is a Qt friendly version the MapLibre Native engine using Qt types
    and deep integration with Qt event loop. QMapLibre::Map relies as much as possible
    on Qt, trying to minimize the external dependencies. For instance it will use
    QNetworkAccessManager for HTTP requests and QString for UTF-8 manipulation.

    QMapLibre::Map is not thread-safe and it is assumed that it will be accessed from
    the same thread as the thread where the OpenGL context lives.
*/

/*!
    \enum QMapLibre::Map::MapChange

    This enum represents the last changed occurred to the map state.

    \value MapChangeRegionWillChange                      A region of the map will change, like
    when resizing the map.

    \value MapChangeRegionWillChangeAnimated              Not in use by QMapLibre::Map.

    \value MapChangeRegionIsChanging                      A region of the map is changing.

    \value MapChangeRegionDidChange                       A region of the map finished changing.

    \value MapChangeRegionDidChangeAnimated               Not in use by QMapLibre::Map.

    \value MapChangeWillStartLoadingMap                   The map is getting loaded. This state
    is set only once right after QMapLibre::Map is created and a style is set.

    \value MapChangeDidFinishLoadingMap                   All the resources were loaded and parsed
    and the map is fully rendered. After this state the mapChanged() signal won't fire again unless
    the is some client side interaction with the map or a tile expires, causing a new resource
    to be requested from the network.

    \value MapChangeDidFailLoadingMap                     An error occurred when loading the map.

    \value MapChangeWillStartRenderingFrame               Just before rendering the frame. This
    is the state of the map just after calling render() and might happened many times before
    the map is fully loaded.

    \value MapChangeDidFinishRenderingFrame               The current frame was rendered but was
    left in a partial state. Some parts of the map might be missing because have not arrived
    from the network or are being parsed.

    \value MapChangeDidFinishRenderingFrameFullyRendered  The current frame was fully rendered.

    \value MapChangeWillStartRenderingMap                 Set once when the map is about to get
    rendered for the first time.

    \value MapChangeDidFinishRenderingMap                 Not in use by QMapLibre::Map.

    \value MapChangeDidFinishRenderingMapFullyRendered    Map is fully loaded and rendered.

    \value MapChangeDidFinishLoadingStyle                 The style was loaded.

    \value MapChangeSourceDidChange                       A source has changed.

    \sa mapChanged()
*/

/*!
    \enum QMapLibre::Map::MapLoadingFailure

    This enum represents map loading failure type.

    \value StyleParseFailure                             Failure to parse the style.
    \value StyleLoadFailure                              Failure to load the style data.
    \value NotFoundFailure                               Failure to obtain style resource file.
    \value UnknownFailure                                Unknown map loading failure.

    \sa mapLoadingFailed()
*/

/*!
    \enum QMapLibre::Map::NorthOrientation

    This enum sets the orientation of the north bearing. It will directly affect bearing when
    resetting the north (i.e. setting bearing to 0).

    \value NorthUpwards     The north is pointing up in the map. This is usually how maps are oriented.

    \value NorthRightwards  The north is pointing right.

    \value NorthDownwards   The north is pointing down.

    \value NorthLeftwards   The north is pointing left.

    \sa northOrientation()
    \sa bearing()
*/

/*!
    Constructs a QMapLibre::Map object with \a settings and sets \a parent_ as the parent
    object. The \a settings cannot be changed after the object is constructed. The
    \a size represents the size of the viewport and the \a pixelRatio the initial pixel
    density of the screen.
*/
Map::Map(QObject *parent_, const Settings &settings, const QSize &size, qreal pixelRatio)
    : QObject(parent_) {
    assert(!size.isEmpty());

    // Multiple QMapLibre::Map running on the same thread
    // will share the same mbgl::util::RunLoop
    if (!loop.hasLocalData()) {
        loop.setLocalData(std::make_shared<mbgl::util::RunLoop>());
    }

    d_ptr = std::make_unique<MapPrivate>(this, settings, size, pixelRatio);
}

/*!
    Destroys this QMapLibre::Map.
*/
Map::~Map() = default;

/*!
    \property QMapLibre::Map::styleJson
    \brief the map style JSON.

    Sets a new \a style from a JSON that must conform to the
    \l {https://maplibre.org/maplibre-style-spec/}
    {MapLibre Style spec}.

    \note In case of a invalid style it will trigger a mapChanged
    signal with QMapLibre::Map::MapChangeDidFailLoadingMap as argument.
*/
QString Map::styleJson() const {
    return QString::fromStdString(d_ptr->mapObj->getStyle().getJSON());
}

void Map::setStyleJson(const QString &style) {
    d_ptr->mapObj->getStyle().loadJSON(style.toStdString());
}

/*!
    \property QMapLibre::Map::styleUrl
    \brief the map style URL.

    Sets a URL for fetching a JSON that will be later fed to
    setStyleJson. URLs using the Mapbox scheme (\a mapbox://) are
    also accepted and translated automatically to an actual HTTPS
    request.

    The Mapbox scheme is not enforced and a style can be fetched
    from anything that QNetworkAccessManager can handle.

    \note In case of a invalid style it will trigger a mapChanged
    signal with QMapLibre::Map::MapChangeDidFailLoadingMap as argument.
*/
QString Map::styleUrl() const {
    return QString::fromStdString(d_ptr->mapObj->getStyle().getURL());
}

void Map::setStyleUrl(const QString &url) {
    d_ptr->mapObj->getStyle().loadURL(url.toStdString());
}

/*!
    \property QMapLibre::Map::latitude
    \brief the map's current latitude in degrees.

    Setting a latitude doesn't necessarily mean it will be accepted since QMapLibre::Map
    might constrain it within the limits of the Web Mercator projection.
*/
double Map::latitude() const {
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    return d_ptr->mapObj->getCameraOptions(d_ptr->margins).center->latitude();
}

void Map::setLatitude(double latitude_) {
    d_ptr->mapObj->jumpTo(
        mbgl::CameraOptions().withCenter(mbgl::LatLng{latitude_, longitude()}).withPadding(d_ptr->margins));
}

/*!
    \property QMapLibre::Map::longitude
    \brief the map current longitude in degrees.

    Setting a longitude beyond the limits of the Web Mercator projection will make
    the map wrap. As an example, setting the longitude to 360 is effectively the same
    as setting it to 0.
*/
double Map::longitude() const {
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    return d_ptr->mapObj->getCameraOptions(d_ptr->margins).center->longitude();
}

void Map::setLongitude(double longitude_) {
    d_ptr->mapObj->jumpTo(
        mbgl::CameraOptions().withCenter(mbgl::LatLng{latitude(), longitude_}).withPadding(d_ptr->margins));
}

/*!
    \property QMapLibre::Map::scale
    \brief the map scale factor.

    This property is used to zoom the map. When \a center is defined, the map will
    scale in the direction of the center pixel coordinates. The \a center will remain
    at the same pixel coordinate after scaling as before calling this method.

    \note This function could be used for implementing a pinch gesture or zooming
    by using the mouse scroll wheel.

    \sa zoom()
*/
double Map::scale() const {
    constexpr double zoomScaleBase{2.0};
    return std::pow(zoomScaleBase, zoom());
}

void Map::setScale(double scale_, const QPointF &center) {
    d_ptr->mapObj->jumpTo(
        mbgl::CameraOptions().withZoom(::log2(scale_)).withAnchor(mbgl::ScreenCoordinate{center.x(), center.y()}));
}

/*!
    \property QMapLibre::Map::zoom
    \brief the map zoom factor.

    This property is used to zoom the map. When \a center is defined, the map will
    zoom in the direction of the center. This function could be used for implementing
    a pinch gesture or zooming by using the mouse scroll wheel.

    \sa scale()
*/
double Map::zoom() const {
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    return *d_ptr->mapObj->getCameraOptions().zoom;
}

void Map::setZoom(double zoom_) {
    d_ptr->mapObj->jumpTo(mbgl::CameraOptions().withZoom(zoom_).withPadding(d_ptr->margins));
}

/*!
    Returns the minimum zoom level allowed for the map.

    \sa maximumZoom()
*/
double Map::minimumZoom() const {
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    return *d_ptr->mapObj->getBounds().minZoom;
}

/*!
    Returns the maximum zoom level allowed for the map.

    \sa minimumZoom()
*/
double Map::maximumZoom() const {
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    return *d_ptr->mapObj->getBounds().maxZoom;
}

/*!
    \property QMapLibre::Map::coordinate
    \brief the map center \a coordinate.

    Centers the map at a geographic coordinate respecting the margins, if set.

    \sa margins()
*/
Coordinate Map::coordinate() const {
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    const mbgl::LatLng latLng = *d_ptr->mapObj->getCameraOptions(d_ptr->margins).center;
    return {latLng.latitude(), latLng.longitude()};
}

void Map::setCoordinate(const Coordinate &coordinate_) {
    d_ptr->mapObj->jumpTo(mbgl::CameraOptions()
                              .withCenter(mbgl::LatLng{coordinate_.first, coordinate_.second})
                              .withPadding(d_ptr->margins));
}

/*!
    \fn QMapLibre::Map::setCoordinateZoom(const Coordinate &coordinate, double zoom)

    Convenience method for setting the \a coordinate and \a zoom simultaneously.

    \note Setting \a coordinate and \a zoom at once is more efficient than doing
    it in two steps.

    \sa zoom()
    \sa coordinate()
*/
void Map::setCoordinateZoom(const Coordinate &coordinate_, double zoom_) {
    d_ptr->mapObj->jumpTo(mbgl::CameraOptions()
                              .withCenter(mbgl::LatLng{coordinate_.first, coordinate_.second})
                              .withZoom(zoom_)
                              .withPadding(d_ptr->margins));
}

/*!
    Atomically jumps to the \a camera options.
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
    \property QMapLibre::Map::bearing
    \brief the map bearing in degrees.

    Set the angle in degrees. Negative values and values over 360 are
    valid and will wrap. The direction of the rotation is counterclockwise.

    When \a center is defined, the map will rotate around the center pixel coordinate
    respecting the margins if defined.

    \sa margins()
*/
double Map::bearing() const {
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    return *d_ptr->mapObj->getCameraOptions().bearing;
}

void Map::setBearing(double degrees) {
    d_ptr->mapObj->jumpTo(mbgl::CameraOptions().withBearing(degrees).withPadding(d_ptr->margins));
}

void Map::setBearing(double degrees, const QPointF &center) {
    d_ptr->mapObj->jumpTo(
        mbgl::CameraOptions().withBearing(degrees).withAnchor(mbgl::ScreenCoordinate{center.x(), center.y()}));
}

/*!
    \property QMapLibre::Map::pitch
    \brief the map pitch in degrees.

    Pitch toward the horizon measured in degrees, with 0 resulting in a
    two-dimensional map. It will be constrained at 60 degrees.

    \sa margins()
*/
double Map::pitch() const {
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    return *d_ptr->mapObj->getCameraOptions().pitch;
}

void Map::setPitch(double pitch_) {
    d_ptr->mapObj->jumpTo(mbgl::CameraOptions().withPitch(pitch_));
}

void Map::pitchBy(double pitch_) {
    d_ptr->mapObj->pitchBy(pitch_);
}

/*!
    Returns the north orientation mode.
*/
Map::NorthOrientation Map::northOrientation() const {
    return static_cast<Map::NorthOrientation>(d_ptr->mapObj->getMapOptions().northOrientation());
}

/*!
    Sets the north orientation mode to \a orientation.
*/
void Map::setNorthOrientation(NorthOrientation orientation) {
    d_ptr->mapObj->setNorthOrientation(static_cast<mbgl::NorthOrientation>(orientation));
}

/*!
    Tells the map rendering engine that there is currently a gesture in \a progress. This
    affects how the map renders labels, as it will use different texture filters if a gesture
    is ongoing.
*/
void Map::setGestureInProgress(bool progress) {
    d_ptr->mapObj->setGestureInProgress(progress);
}

/*!
    Sets the \a duration and \a delay of style transitions. Style paint property
    values transition to new values with animation when they are updated.
*/
void Map::setTransitionOptions(qint64 duration, qint64 delay) {
    static auto convert = [](qint64 value) -> std::optional<mbgl::Duration> {
        return std::chrono::duration_cast<mbgl::Duration>(mbgl::Milliseconds(value));
    };

    d_ptr->mapObj->getStyle().setTransitionOptions(mbgl::style::TransitionOptions{convert(duration), convert(delay)});
}

/*!
    Adds an \a annotation to the map.

    Returns the unique identifier for the new annotation.

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
    Updates an existing \a annotation referred by \a id.

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
    Removes an existing annotation referred by \a id.
*/
void Map::removeAnnotation(AnnotationID id) {
    d_ptr->mapObj->removeAnnotation(id);
}

/*!
    Sets a layout \a property_ \a value to an existing \a layer. The \a property_ string can be any
    as defined by the \l {https://maplibre.org/maplibre-style-spec/} {MapLibre Style Spec}
    for layout properties. Returns true if the operation succeeds, and false otherwise.

    The implementation attempts to treat \a value as a JSON string, if the
    QVariant inner type is a string. If not a valid JSON string, then it'll
    proceed with the mapping described below.

    This example hides the layer \c route:

    \code
        map->setLayoutProperty("route", "visibility", "none");
    \endcode

    This table describes the mapping between \l {https://maplibre.org/maplibre-style-spec/#types}
    {style types} and Qt types accepted by setLayoutProperty():

    \table
    \header
        \li MapLibre style type
        \li Qt type
    \row
        \li Enum
        \li QString
    \row
        \li String
        \li QString
    \row
        \li Boolean
        \li \c bool
    \row
        \li Number
        \li \c int, \c double or \c float
    \row
        \li Array
        \li QVariantList
    \endtable
*/
bool Map::setLayoutProperty(const QString &layer, const QString &propertyName, const QVariant &value) {
    return d_ptr->setProperty(&mbgl::style::Layer::setProperty, layer, propertyName, value);
}

/*!
    Sets a paint \a property_ \a value to an existing \a layer. The \a property_ string can be any
    as defined by the \l {https://maplibre.org/maplibre-style-spec/} {MapLibre Style Spec}
    for paint properties. Returns true if the operation succeeds, and false otherwise.

    The implementation attempts to treat \a value as a JSON string, if the
    QVariant inner type is a string. If not a valid JSON string, then it'll
    proceed with the mapping described below.

    For paint properties that take a color as \a value, such as \c fill-color, a string such as
    \c blue can be passed or a QColor.

    \code
        map->setPaintProperty("route", "line-color", QColor("blue"));
    \endcode

    This table describes the mapping between \l {https://maplibre.org/maplibre-style-spec/#types}
    {style types} and Qt types accepted by setPaintProperty():

    \table
    \header
        \li MapLibre style type
        \li Qt type
    \row
        \li Color
        \li QString or QColor
    \row
        \li Enum
        \li QString
    \row
        \li String
        \li QString
    \row
        \li Boolean
        \li \c bool
    \row
        \li Number
        \li \c int, \c double or \c float
    \row
        \li Array
        \li QVariantList
    \endtable

    If the style specification defines the property's type as \b Array, use a QVariantList. For
    example, the following code sets a \c route layer's \c line-dasharray property:

    \code
        QVariantList lineDashArray;
        lineDashArray.append(1);
        lineDashArray.append(2);

        map->setPaintProperty("route","line-dasharray", lineDashArray);
    \endcode
*/

bool Map::setPaintProperty(const QString &layer, const QString &propertyName, const QVariant &value) {
    return d_ptr->setProperty(&mbgl::style::Layer::setProperty, layer, propertyName, value);
}

/*!
    Returns true when the map is completely rendered, false otherwise. A partially
    rendered map ranges from nothing rendered at all to only labels missing.
*/
bool Map::isFullyLoaded() const {
    return d_ptr->mapObj->isFullyLoaded();
}

/*!
    Pan the map by \a offset in pixels.

    The pixel coordinate origin is located at the upper left corner of the map.
*/
void Map::moveBy(const QPointF &offset) {
    d_ptr->mapObj->moveBy(mbgl::ScreenCoordinate{offset.x(), offset.y()});
}

/*!
    \fn QMapLibre::Map::scaleBy(double scale, const QPointF &center)

    Scale the map by \a scale in the direction of the \a center. This function
    can be used for implementing a pinch gesture.
*/
void Map::scaleBy(double scale_, const QPointF &center) {
    d_ptr->mapObj->scaleBy(scale_, mbgl::ScreenCoordinate{center.x(), center.y()});
}

/*!
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
    Resize the map to \a size_ and scale to fit at the framebuffer. For
    high DPI screens, the size will be smaller than the framebuffer.
*/
void Map::resize(const QSize &size_) {
    auto size = sanitizedSize(size_);

    if (d_ptr->mapObj->getMapOptions().size() == size) {
        return;
    }

    d_ptr->mapObj->setSize(size);
}

/*!
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
    \fn QMapLibre::Map::pixelForCoordinate(const Coordinate &coordinate) const

    Returns the offset in pixels for \a coordinate. The origin pixel coordinate is
    located at the top left corner of the map view.

    This method returns the correct value for any coordinate, even if the coordinate
    is not currently visible on the screen.

    /note The return value is affected by the current zoom level, bearing and pitch.
*/
QPointF Map::pixelForCoordinate(const Coordinate &coordinate_) const {
    const mbgl::ScreenCoordinate pixel = d_ptr->mapObj->pixelForLatLng(
        mbgl::LatLng{coordinate_.first, coordinate_.second});

    return {pixel.x, pixel.y};
}

/*!
    Returns the geographic coordinate for the \a pixel coordinate.
*/
Coordinate Map::coordinateForPixel(const QPointF &pixel) const {
    const mbgl::LatLng latLng = d_ptr->mapObj->latLngForPixel(mbgl::ScreenCoordinate{pixel.x(), pixel.y()});

    return {latLng.latitude(), latLng.longitude()};
}

/*!
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
    \property QMapLibre::Map::margins
    \brief the map margins in pixels from the corners of the map.

    This property sets a new reference center for the map.
*/
void Map::setMargins(const QMargins &margins_) {
    d_ptr->margins = {static_cast<double>(margins_.top()),
                      static_cast<double>(margins_.left()),
                      static_cast<double>(margins_.bottom()),
                      static_cast<double>(margins_.right())};
}

QMargins Map::margins() const {
    return {static_cast<int>(d_ptr->margins.left()),
            static_cast<int>(d_ptr->margins.top()),
            static_cast<int>(d_ptr->margins.right()),
            static_cast<int>(d_ptr->margins.bottom())};
}

/*!
    Adds a source \a id to the map as specified by the \l
    {https://maplibre.org/maplibre-style-spec/#root-sources}{MapLibre Style Specification} with
    \a params.

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
        qWarning() << "Unable to add source:" << error.message.c_str();
        return;
    }

    d_ptr->mapObj->getStyle().addSource(std::move(*source));
}

/*!
    Returns true if the layer with given \a sourceID exists, false otherwise.
*/
bool Map::sourceExists(const QString &sourceID) {
    return d_ptr->mapObj->getStyle().getSource(sourceID.toStdString()) != nullptr;
}

/*!
    Updates the source \a id with new \a params.

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

    if (sourceImage != nullptr && params.contains("url")) {
        sourceImage->setURL(params["url"].toString().toStdString());
    } else if (sourceGeoJSON != nullptr && params.contains("data")) {
        mbgl::style::conversion::Error error;
        auto result = mbgl::style::conversion::convert<mbgl::GeoJSON>(params["data"], error);
        if (result) {
            sourceGeoJSON->setGeoJSON(*result);
        }
    }
}

/*!
    Removes the source \a id.

    This method has no effect if the source does not exist.
*/
void Map::removeSource(const QString &id) {
    auto sourceIDStdString = id.toStdString();

    if (d_ptr->mapObj->getStyle().getSource(sourceIDStdString) != nullptr) {
        d_ptr->mapObj->getStyle().removeSource(sourceIDStdString);
    }
}

/*!
    Adds a custom layer \a id with the initialization function \a initFn, the
    render function \a renderFn and the deinitialization function \a deinitFn with
    the user data \a context before the existing layer \a before.

    \warning This is used for delegating the rendering of a layer to the user of
    this API and is not officially supported. Use at your own risk.
*/
void Map::addCustomLayer(const QString &id, std::unique_ptr<CustomLayerHostInterface> host, const QString &before) {
    class HostWrapper final : public mbgl::style::CustomLayerHost {
    public:
        std::unique_ptr<CustomLayerHostInterface> ptr{};
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
    Adds a style layer to the map as specified by the \l
    {https://maplibre.org/maplibre-style-spec/#root-layers}{MapLibre style specification} with
    \a params. The layer will be added under the layer specified by \a before, if specified.
    Otherwise it will be added as the topmost layer.

    This example shows how to add a layer that will be used to show a route line on the map. Note
    that nothing will be drawn until we set paint properties using setPaintProperty().

    \code
        QVariantMap route;
        route["id"] = "route";
        route["type"] = "line";
        route["source"] = "routeSource";

        map->addLayer(route);
    \endcode

    /note The source must exist prior to adding a layer.
*/
void Map::addLayer(const QVariantMap &params, const QString &before) {
    mbgl::style::conversion::Error error;
    std::optional<std::unique_ptr<mbgl::style::Layer>> layer =
        mbgl::style::conversion::convert<std::unique_ptr<mbgl::style::Layer>>(QVariant(params), error);
    if (!layer) {
        qWarning() << "Unable to add layer:" << error.message.c_str();
        return;
    }

    d_ptr->mapObj->getStyle().addLayer(
        std::move(*layer),
        before.isEmpty() ? std::optional<std::string>() : std::optional<std::string>(before.toStdString()));
}

/*!
    Returns true if the layer with given \a id exists, false otherwise.
*/
bool Map::layerExists(const QString &id) {
    return d_ptr->mapObj->getStyle().getLayer(id.toStdString()) != nullptr;
}

/*!
    Removes the layer with given \a id.
*/
void Map::removeLayer(const QString &id) {
    d_ptr->mapObj->getStyle().removeLayer(id.toStdString());
}

/*!
    List of all existing layer ids from the current style.
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
    Adds the \a sprite with the identifier \a id that can be used
    later by a symbol layer.

    If the \a id was already added, it gets replaced by the new
    \a image only if the dimensions of the image are the same as
    the old image, otherwise it has no effect.

    \sa addLayer()
*/
void Map::addImage(const QString &name, const QImage &sprite) {
    if (sprite.isNull()) {
        return;
    }

    d_ptr->mapObj->getStyle().addImage(toStyleImage(name, sprite));
}

/*!
    Removes the image \a id.
*/
void Map::removeImage(const QString &name) {
    d_ptr->mapObj->getStyle().removeImage(name.toStdString());
}

/*!
    Adds a \a filter to a style \a layer using the format described in the \l
    {https://maplibre.org/maplibre-style-spec/deprecations/#other-filter}{MapLibre Style Spec}.

    Given a layer \c marker from an arbitrary GeoJSON source containing features of type \b
    "Point" and \b "LineString", this example shows how to make sure the layer will only tag
    features of type \b "Point".

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
void Map::setFilter(const QString &layer, const QVariant &filter) {
    mbgl::style::Layer *layer_ = d_ptr->mapObj->getStyle().getLayer(layer.toStdString());
    if (layer_ == nullptr) {
        qWarning() << "Layer not found:" << layer;
        return;
    }

    mbgl::style::conversion::Error error;
    std::optional<mbgl::style::Filter> converted = mbgl::style::conversion::convert<mbgl::style::Filter>(filter, error);
    if (!converted) {
        qWarning() << "Error parsing filter:" << error.message.c_str();
        return;
    }

    layer_->setFilter(*converted);
}

QVariant QVariantFromValue(const mbgl::Value &value) {
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
                               list.push_back(QVariantFromValue(value_));
                           }
                           return list;
                       },
                       [&](const std::unordered_map<std::string, mbgl::Value> &map) {
                           QVariantMap varMap;
                           for (const auto &item : map) {
                               varMap.insert(item.first.c_str(), QVariantFromValue(item.second));
                           }
                           return varMap;
                       },
                       [](const auto &) { return QVariant(); });
}

/*!
    Returns the current \a expression-based filter value applied to a style
    \layer, if any.

    Filter value types are described in the {https://maplibre.org/maplibre-style-spec/types/}{MapLibre Style Spec}.
*/
QVariant Map::getFilter(const QString &layer) const {
    mbgl::style::Layer *layer_ = d_ptr->mapObj->getStyle().getLayer(layer.toStdString());
    if (layer_ == nullptr) {
        qWarning() << "Layer not found:" << layer;
        return {};
    }

    auto serialized = layer_->getFilter().serialize();
    return QVariantFromValue(serialized);
}

/*!
    Creates the infrastructure needed for rendering the map. It
    should be called before any call to render().

    Must be called on the render thread.
*/
void Map::createRenderer() {
    d_ptr->createRenderer();
}

/*!
    Destroys the infrastructure needed for rendering the map,
    releasing resources.

    Must be called on the render thread.
*/
void Map::destroyRenderer() {
    d_ptr->destroyRenderer();
}

/*!
    Start a static rendering of the current state of the map. This
    should only be called when the map is initialized in static mode.

    \sa QMapLibre::Settings::MapMode
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
    Renders the map using OpenGL draw calls. It will make sure to bind the
    framebuffer object before drawing; otherwise a valid OpenGL context is
    expected with an appropriate OpenGL viewport state set for the size of
    the canvas.

    This function should be called only after the signal needsRendering() is
    emitted at least once.

    Must be called on the render thread.
*/
void Map::render() {
    d_ptr->render();
}

/*!
    If MapLibre Native needs to rebind the default \a fbo, it will use the
    ID supplied here. \a size is the size of the framebuffer, which
    on high DPI screens is usually bigger than the map size.

    Must be called on the render thread.
*/
void Map::setFramebufferObject(quint32 fbo, const QSize &size) {
    d_ptr->setFramebufferObject(fbo, size);
}

/*!
    Informs the map that the network connection has been established, causing
    all network requests that previously timed out to be retried immediately.
*/
void Map::connectionEstablished() {
    mbgl::NetworkStatus::Reachable();
}

/*!
    \fn void QMapLibre::Map::needsRendering()

    This signal is emitted when the visual contents of the map have changed
    and a redraw is needed in order to keep the map visually consistent
    with the current state.

    \sa render()
*/

/*!
    \fn void QMapLibre::Map::staticRenderFinished(const QString &error)

    This signal is emitted when a static map is fully drawn. Usually the next
    step is to extract the map from a framebuffer into a container like a
    QImage. \a error is set to a message when an error occurs.

    \sa startStaticRender()
*/

/*!
    \fn void QMapLibre::Map::mapChanged(QMapLibre::Map::MapChange change)

    This signal is emitted when the state of the map has changed. This signal
    may be used for detecting errors when loading a style or detecting when
    a map is fully loaded by analyzing the parameter \a change.
*/

/*!
    \fn void QMapLibre::Map::mapLoadingFailed(QMapLibre::Map::MapLoadingFailure type, const QString &description)

    This signal is emitted when a map loading failure happens. Details of the
    failures are provided, including its \a type and textual \a description.
*/

/*!
    \fn void QMapLibre::Map::copyrightsChanged(const QString &copyrightsHtml);

    This signal is emitted when the copyrights of the current content of the map
    have changed. This can be caused by a style change or adding a new source.

    \a copyrightsHtml is a string with a HTML snippet.
*/

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
    const std::lock_guard<std::recursive_mutex> lock(m_mapRendererMutex);

    m_updateParameters = std::move(parameters);

    if (m_mapRenderer == nullptr) {
        return;
    }

    m_mapRenderer->updateParameters(std::move(m_updateParameters));

    requestRendering();
}

void MapPrivate::setObserver(mbgl::RendererObserver &observer) {
    m_rendererObserver = std::make_shared<RendererObserver>(*mbgl::util::RunLoop::Get(), observer);

    const std::lock_guard<std::recursive_mutex> lock(m_mapRendererMutex);

    if (m_mapRenderer != nullptr) {
        m_mapRenderer->setObserver(m_rendererObserver.get());
    }
}

void MapPrivate::createRenderer() {
    const std::lock_guard<std::recursive_mutex> lock(m_mapRendererMutex);

    if (m_mapRenderer != nullptr) {
        return;
    }

    m_mapRenderer = std::make_unique<MapRenderer>(m_pixelRatio, m_mode, m_localFontFamily);

    connect(m_mapRenderer.get(), &MapRenderer::needsRendering, this, &MapPrivate::requestRendering);

    m_mapRenderer->setObserver(m_rendererObserver.get());

    if (m_updateParameters != nullptr) {
        m_mapRenderer->updateParameters(m_updateParameters);
        requestRendering();
    }
}

void MapPrivate::destroyRenderer() {
    const std::lock_guard<std::recursive_mutex> lock(m_mapRendererMutex);

    m_mapRenderer.reset();
}

void MapPrivate::render() {
    const std::lock_guard<std::recursive_mutex> lock(m_mapRendererMutex);

    if (m_mapRenderer == nullptr) {
        createRenderer();
    }

    m_renderQueued.clear();
    m_mapRenderer->render();
}

void MapPrivate::setFramebufferObject(quint32 fbo, const QSize &size) {
    const std::lock_guard<std::recursive_mutex> lock(m_mapRendererMutex);

    if (m_mapRenderer == nullptr) {
        createRenderer();
    }

    m_mapRenderer->updateFramebuffer(fbo, sanitizedSize(size));
}

void MapPrivate::requestRendering() {
    if (!m_renderQueued.test_and_set()) {
        emit needsRendering();
    }
}

bool MapPrivate::setProperty(const PropertySetter &setter,
                             const QString &layer,
                             const QString &name,
                             const QVariant &value) const {
    mbgl::style::Layer *layerObject = mapObj->getStyle().getLayer(layer.toStdString());
    if (layerObject == nullptr) {
        qWarning() << "Layer not found:" << layer;
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
            result = (layerObject->*setter)(propertyString, jsonValue);
        } else {
            result = (layerObject->*setter)(propertyString, value);
        }
    } else {
        result = (layerObject->*setter)(propertyString, value);
    }

    if (result) {
        qWarning() << "Error setting property" << name << "on layer" << layer << ":"
                   << QString::fromStdString(result->message);
        return false;
    }

    return true;
}

} // namespace QMapLibre
