// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

/*!
    \class LayerParameter
    \brief A QML type for setting parameters of a layer. Generated from \ref QMapLibre::LayerParameter and should be nested in a \ref Style.
    \ingroup QMapLibreLocation

    Additional layer-specific properties can be set directly as QML properties
    to the instance of the LayerParameter.

    \snippet{trimleft} snippets_Style.qml Layer parameter
*/

SourceParameter {
    property string type //!< layer type
    property object layout //!< layout properties as a dictionary
    property object paint //!< paint properties as a dictionary
}
