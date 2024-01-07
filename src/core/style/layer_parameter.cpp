// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#include "layer_parameter.hpp"
#include "style_parameter.hpp"

namespace QMapLibre {

/*!
    \class LayerParameter
    \brief A helper utility to create and configure map layers in the Map
    \ingroup QMapLibre

    Needs to have a \ref type set to one of the supported types.
    Other properties are set dynamically, depending on the type.
*/

/*!
    \brief Default constructor
*/
LayerParameter::LayerParameter(QObject *parent)
    : StyleParameter(parent) {}

LayerParameter::~LayerParameter() = default;

/*!
    \fn void LayerParameter::layoutUpdated()
    \brief Signal emitted when the layer layout is updated.
*/

/*!
    \fn void LayerParameter::paintUpdated()
    \brief Signal emitted when the layer paint is updated.
*/

/*!
    \brief Get layer type.
    \return Layer type as \c QString.
*/
QString LayerParameter::type() const {
    return m_type;
}

/*!
    \brief Set layer type.
    \param type Layer type as \c QString.
*/
void LayerParameter::setType(const QString &type) {
    if (m_type.isEmpty()) {
        m_type = type;
    }
}

/*!
    \brief Layout properties of the layer.
    \return Layout properties as \c QJsonObject.
*/
QJsonObject LayerParameter::layout() const {
    return m_layout;
}

/*!
    \brief Set layout properties of the layer.
    \param layout Layout properties as \c QJsonObject.

    \ref layoutUpdated() signal is emitted when the layout is updated.
*/
void LayerParameter::setLayout(const QJsonObject &layout) {
    if (m_layout == layout) {
        return;
    }

    m_layout = layout;

    Q_EMIT layoutUpdated();
}

/*!
    \brief Set layout property.
    \param key Property name as \c QString.
    \param value Property value as \c QVariant.

    \ref layoutUpdated() signal is emitted when the layout is updated.
*/
void LayerParameter::setLayoutProperty(const QString &key, const QVariant &value) {
    m_layout[key] = value.toJsonValue();

    Q_EMIT layoutUpdated();
}

/*!
    \brief Paint properties of the layer.
    \return Paint properties as \c QJsonObject.
*/
QJsonObject LayerParameter::paint() const {
    return m_paint;
}

/*!
    \brief Set paint properties of the layer.
    \param paint Paint properties as \c QJsonObject.

    \ref paintUpdated() signal is emitted when the paint is updated.
*/
void LayerParameter::setPaint(const QJsonObject &paint) {
    if (m_paint == paint) {
        return;
    }

    m_paint = paint;

    Q_EMIT paintUpdated();
}

/*!
    \brief Set paint property.
    \param key Property name as \c QString.
    \param value Property value as \c QVariant.

    \ref paintUpdated() signal is emitted when the paint is updated.
*/
void LayerParameter::setPaintProperty(const QString &key, const QVariant &value) {
    m_paint[key] = value.toJsonValue();

    Q_EMIT paintUpdated();
}

/*!
    \var LayerParameter::m_type
    \brief Type of the source configured.
*/
/*!
    \var LayerParameter::m_layout
    \brief Layout properties of the layer.
*/
/*!
    \var LayerParameter::m_paint
    \brief Paint properties of the layer.
*/

} // namespace QMapLibre
