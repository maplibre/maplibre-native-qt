// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#include "style_parameter.hpp"

#include <QtCore/QMetaProperty>
#include <QtCore/QVariant>

namespace QMapLibre {

/*!
    \class StyleParameter
    \brief A base class to pass style parameters to Map
    \ingroup QMapLibre

    Should usually not be used directly, but rather through one of its subclasses
    such as \ref SourceParameter, \ref LayerParameter, \ref ImageParameter
    or \ref FilterParameter.
*/

/*!
    \brief Default constructor
*/
StyleParameter::StyleParameter(QObject *parent)
    : QObject(parent) {}

StyleParameter::~StyleParameter() = default;

/*!
    \brief Comparison operator
*/
bool StyleParameter::operator==(const StyleParameter &other) const {
    return (other.toVariantMap() == toVariantMap());
}

/*!
    \fn void StyleParameter::ready(StyleParameter *parameter)
    \param parameter The style parameter that is ready.
    \brief Signal emitted when the style parameter is ready.
*/

/*!
    \fn void StyleParameter::updated(StyleParameter *parameter)
    \param parameter The style parameter that is updated.
    \brief Signal emitted when the style parameter is updated.
*/

/*!
    \fn bool StyleParameter::isReady()
    \brief Check if the style parameter is ready.
    \return \c true if the style parameter is ready, \c false otherwise.
*/

/*!
    \brief Get the property value
    \param propertyName Name of the property to get.
    \return Property value as \c QVariant or invalid \c QVariant if the property does not exist.

    By default this directly reads the property value from the object.
    It can be reimplemented from subclasses to provide custom parsing.
*/
QVariant StyleParameter::parsedProperty(const char *propertyName) const {
    return property(propertyName);
}

/*!
    \brief Check for property existence.
    \param propertyName Name of the property to check.
    \return \c true if the property exists, \c false otherwise.
*/
bool StyleParameter::hasProperty(const char *propertyName) const {
    return metaObject()->indexOfProperty(propertyName) != -1 ||
           dynamicPropertyNames().indexOf(QByteArray(propertyName)) != -1;
}

/*!
    \brief Update property value.
    \param propertyName Name of the property to update.
    \param value The new value of the property.
*/
void StyleParameter::updateProperty(const char *propertyName, const QVariant &value) {
    const QMetaProperty property = metaObject()->property(metaObject()->indexOfProperty(propertyName));
    property.write(this, value);
    updateNotify();
}

/*!
    \brief Return a map of all properties.
    \return A map of all properties as \c QVariantMap.
*/
QVariantMap StyleParameter::toVariantMap() const {
    QVariantMap res;
    const QMetaObject *metaObj = metaObject();
    for (int i = m_initialPropertyCount; i < metaObj->propertyCount(); ++i) {
        const char *propName = metaObj->property(i).name();
        res[QLatin1String(propName)] = property(propName);
    }
    return res;
}

/*!
    \brief Return style identifier.
    \return A style identifier as \c QString.
*/
QString StyleParameter::styleId() const {
    return m_styleId;
}

/*!
    \brief Set style identifier.
    \param id The style identifier to set.
*/
void StyleParameter::setStyleId(const QString &id) {
    if (m_styleId.isEmpty()) {
        m_styleId = id;
    }
}

/*!
    \brief Notify that the style parameter has been updated.
*/
void StyleParameter::updateNotify() {
    emit updated(this);
}

/*!
    \var StyleParameter::m_initialPropertyCount
    \brief Number of properties in the base class.
*/
/*!
    \var StyleParameter::m_ready
    \brief Ready status of the style parameter.
*/
/*!
    \var StyleParameter::m_styleId
    \brief %Style identifier of the parameter.
*/

} // namespace QMapLibre
