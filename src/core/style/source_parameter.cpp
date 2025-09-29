// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#include "source_parameter.hpp"

namespace QMapLibre {

/*!
    \class SourceParameter
    \brief A helper utility to create an additional map data source in the Map
    \ingroup QMapLibre

    \headerfile source_parameter.hpp <QMapLibre/SourceParameter>

    Needs to have a \ref type set to one of the supported types.
    Other properties are set dynamically, depending on the type.
*/

/*!
    \brief Default constructor
*/
SourceParameter::SourceParameter(QObject* parent)
    : StyleParameter(parent) {}

SourceParameter::~SourceParameter() = default;

/*!
    \brief Get source type.
    \return Source type as \c QString.
*/
QString SourceParameter::type() const {
    return m_type;
}

/*!
    \brief Set source type.
    \param type Source type as \c QString.
*/
void SourceParameter::setType(const QString& type) {
    if (m_type.isEmpty()) {
        m_type = type;
    }
}

/*!
    \var SourceParameter::m_type
    \brief Type of the source configured.
*/

} // namespace QMapLibre
