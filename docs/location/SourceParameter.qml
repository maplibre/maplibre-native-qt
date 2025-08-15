// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

/*!
    \class SourceParameter
    \brief A QML type for setting parameters of a source. Generated from \ref QMapLibre::SourceParameter and should be nested in a \ref Style.
    \ingroup QMapLibreLocation

    Additional source-specific properties can be set directly as QML properties
    to the instance of the SourceParameter.

    \snippet{trimleft} snippets_Style.qml Source parameter
*/

Item {
    property string m_type //!< source type
}
