// Copyright (C) 2023 MapLibre contributors
// Copyright (C) 2019 Mapbox, Inc.

// SPDX-License-Identifier: BSD-2-Clause

#include "types.hpp"

#include "mbgl/util/geometry.hpp"
#include "mbgl/util/traits.hpp"

// mbgl::FeatureType
static_assert(mbgl::underlying_type(QMapLibre::Feature::PointType) == mbgl::underlying_type(mbgl::FeatureType::Point),
              "error");
static_assert(mbgl::underlying_type(QMapLibre::Feature::LineStringType) ==
                  mbgl::underlying_type(mbgl::FeatureType::LineString),
              "error");
static_assert(mbgl::underlying_type(QMapLibre::Feature::PolygonType) ==
                  mbgl::underlying_type(mbgl::FeatureType::Polygon),
              "error");

namespace QMapLibre {

/*!
    \namespace QMapLibre

    Contains miscellaneous MapLibre types and utilities used throughout MapLibre Qt bindings.
*/

/*!
    \typedef Coordinate
    \brief Coordinate helper type.

    Alias for `QPair< double, double >`.
    Representation for geographical coordinates - latitude and longitude, respectively.
*/

/*!
    \typedef CoordinateZoom
    \brief Coordinate-zoom pair helper type.

    Alias for `QPair<` \ref Coordinate `, double>`.
    Used as return value in Map::coordinateZoomForBounds.
*/

/*!
    \typedef ProjectedMeters
    \brief Projected meters helper type.

    Alias for `QPair< double, double >`.
    Representation for projected meters - northing and easting, respectively.
*/

/*!
    \typedef Coordinates
    \brief \ref Coordinate vector.

    Alias for `QVector<` \ref Coordinate `>`.
    A list of \ref Coordinate objects.
*/

/*!
    \typedef CoordinatesCollection
    \brief \ref Coordinates vector.

    Alias for `QVector<` \ref Coordinates `>`.
    A list of \ref Coordinates objects.
*/

/*!
    \typedef CoordinatesCollections
    \brief \ref CoordinatesCollection vector.

    Alias for `QVector<` \ref CoordinatesCollection `>`.
    A list of \ref CoordinatesCollection objects.
*/

/*!
    \struct Style
    \brief %Map style helper type.
    \ingroup QMapLibre

    Represents map styles via its \a url, \a name,
    \a description (optional), \a night (optional) and \a type (optional).

    \enum Style::Type
    \brief %Map style type enumeration.

    Taken from Qt to be in sync with QtLocation.

    \fn Style::Style
    \brief Constructs a Style object with the given \a url_ and \a name_.

    \var Style::url
    \brief style URL

    \var Style::name
    \brief style name

    \var Style::description
    \brief style description

    \var Style::night
    \brief true if style is a dark/night variant, false otherwise

    \var Style::type
    \brief style type
*/

/*!
    \typedef Styles
    \brief \ref Style vector.

    Alias for `QVector<` \ref Style `>`.
    A list of \ref Style objects.
*/

/*!
    \struct Feature
    \brief %Map feature helper type.
    \ingroup QMapLibre

    Represents map features via its \a type (PointType, LineStringType or PolygonType),
    \a geometry, \a properties map and \a id (optional).

    \enum Feature::Type
    \brief %Map feature type.

    This enum is used as basis for geometry disambiguation in Feature.

    \var Feature::PointType
    A point geometry type. Means a single or a collection of points.

    \var Feature::LineStringType
    A line string geometry type. Means a single or a collection of line strings.

    \var Feature::PolygonType
    A polygon geometry type. Means a single or a collection of polygons.

    \var Feature::type
    \brief feature type

    \var Feature::geometry
    \brief feature geometry

    \var Feature::properties
    \brief feature properties

    \var Feature::id
    \brief feature identifier
*/

/*!
    \struct FeatureProperty
    \brief %Map feature property helper type.
    \ingroup QMapLibre

    Represents map Feature properties via its \a type (\ref LayoutProperty or \ref PaintProperty),
    \a name and \a value.

    \enum FeatureProperty::Type
    \brief %Map feature property type.

    This enum is used as basis for property type disambiguation in FeatureProperty.

    \var FeatureProperty::LayoutProperty
    A layout property type.

    \var FeatureProperty::PaintProperty
    A paint property type.

    \var FeatureProperty::type
    \brief property type

    \var FeatureProperty::name
    \brief property name

    \var FeatureProperty::value
    \brief property value
*/

/*!
    \typedef Annotation
    \brief Annotation helper type.

    Alias for \c QVariant.
    Container that encapsulates either a symbol, a line, a fill or a style sourced annotation.
*/

/*!
    \typedef AnnotationID
    \brief Annotation identifier helper type.

    Alias for \c quint32 representing an annotation identifier.
*/

/*!
    \typedef AnnotationIDs
    \brief A vector of annotation identifiers.

    Alias for `QVector< quint32 >` representing a container of annotation identifiers.
*/

/*!
    \struct ShapeAnnotationGeometry
    \brief Shape annotation geometry helper type.
    \ingroup QMapLibre

    Represents a shape annotation geometry via its \a type and \a geometry.

    \enum ShapeAnnotationGeometry::Type
    \brief Shape annotation geometry type.

    This enum is used as basis for shape annotation geometry disambiguation.

    \var ShapeAnnotationGeometry::PolygonType
    A polygon geometry type.

    \var ShapeAnnotationGeometry::LineStringType
    A line string geometry type.

    \var ShapeAnnotationGeometry::MultiPolygonType
    A polygon geometry collection type.

    \var ShapeAnnotationGeometry::MultiLineStringType
    A line string geometry collection type.

    \var ShapeAnnotationGeometry::type
    \brief annotation geometry type

    \var ShapeAnnotationGeometry::geometry
    \brief annotation geometry
*/

/*!
    \struct SymbolAnnotation
    \brief Symbol annotation helper type.
    \ingroup QMapLibre

    A symbol annotation comprises of its \a geometry and an \a icon identifier.

    \var SymbolAnnotation::geometry
    \brief annotation geometry

    \var SymbolAnnotation::icon
    \brief annotation icon identifier
*/

/*!
    \struct LineAnnotation
    \brief Line annotation helper type.
    \ingroup QMapLibre

    Represents a line annotation object, along with its properties.

    A line annotation comprises of its \a geometry and line properties
    such as \a opacity, \a width and \a color.

    \var LineAnnotation::geometry
    \brief annotation geometry

    \var LineAnnotation::opacity
    \brief annotation opacity

    \var LineAnnotation::width
    \brief annotation width

    \var LineAnnotation::color
    \brief annotation color
*/

/*!
    \struct FillAnnotation
    \brief Fill annotation helper type.
    \ingroup QMapLibre

    Represents a fill annotation object, along with its properties.

    A fill annotation comprises of its \a geometry and fill properties
    such as \a opacity, \a color and \a outlineColor.

    \var FillAnnotation::geometry
    \brief annotation geometry

    \var FillAnnotation::opacity
    \brief annotation opacity

    \var FillAnnotation::color
    \brief annotation color

    \var FillAnnotation::outlineColor
    \brief annotation outline color
*/

/*!
    \struct CameraOptions
    \brief Camera options helper type.
    \ingroup QMapLibre

    CameraOptions provides camera options to the renderer.

    \var CameraOptions::center
    \brief camera center coordinate (\ref Coordinate)

    \var CameraOptions::anchor
    \brief camera anchor point (\c QPointF)

    \var CameraOptions::zoom
    \brief camera zoom level (\c double)

    \var CameraOptions::bearing
    \brief camera bearing (\c double)

    \var CameraOptions::pitch
    \brief camera pitch (\c double)
*/

/*!
    \struct CustomLayerRenderParameters
    \ingroup QMapLibre

    CustomLayerRenderParameters provides the data passed on each render
    pass for a custom layer.

    \var CustomLayerRenderParameters::width
    \brief rendered width

    \var CustomLayerRenderParameters::height
    \brief rendered height

    \var CustomLayerRenderParameters::latitude
    \brief rendered latitude

    \var CustomLayerRenderParameters::longitude
    \brief rendered longitude

    \var CustomLayerRenderParameters::zoom
    \brief rendered zoom level

    \var CustomLayerRenderParameters::bearing
    \brief rendered bearing

    \var CustomLayerRenderParameters::pitch
    \brief rendered pitch

    \var CustomLayerRenderParameters::fieldOfView
    \brief rendered field of view
*/

/*!
    \class CustomLayerHostInterface
    \ingroup QMapLibre

    Represents a host interface to be implemented for rendering custom layers.

    \warning This is used for delegating the rendering of a layer to the user of
    this API and is not officially supported. Use at your own risk.

    \fn CustomLayerHostInterface::initialize
    \brief Initializes the custom layer.

    \fn CustomLayerHostInterface::render
    \brief Renders the custom layer with the given parameters.

    \fn CustomLayerHostInterface::deinitialize
    \brief Deinitializes the custom layer.
*/

} // namespace QMapLibre
