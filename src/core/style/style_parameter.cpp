// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#include "style_parameter.hpp"

#include <QtCore/QMetaProperty>
#include <QtCore/QVariant>

namespace QMapLibre {

StyleParameter::StyleParameter(QObject *parent)
    : QObject(parent) {}

StyleParameter::~StyleParameter() = default;

bool StyleParameter::operator==(const StyleParameter &other) const {
    return (other.toVariantMap() == toVariantMap());
}

bool StyleParameter::hasProperty(const char *propertyName) const {
    return metaObject()->indexOfProperty(propertyName) != -1 ||
           dynamicPropertyNames().indexOf(QByteArray(propertyName)) != -1;
}

void StyleParameter::updateProperty(const char *propertyName, const QVariant &value) {
    const QMetaProperty property = metaObject()->property(metaObject()->indexOfProperty(propertyName));
    property.write(this, value);
    updateNotify();
}

QVariantMap StyleParameter::toVariantMap() const {
    QVariantMap res;
    const QMetaObject *metaObj = metaObject();
    for (int i = m_initialPropertyCount; i < metaObj->propertyCount(); ++i) {
        const char *propName = metaObj->property(i).name();
        res[QLatin1String(propName)] = property(propName);
    }
    return res;
}

QString StyleParameter::styleId() const {
    return m_styleId;
}

void StyleParameter::setStyleId(const QString &id) {
    if (m_styleId.isEmpty()) {
        m_styleId = id;
    }
}

void StyleParameter::updateNotify() {
    emit updated(this);
}

} // namespace QMapLibre
