// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#include "style_change_utils_p.hpp"

#include <QtCore/QMetaProperty>
#include <QtCore/QRegularExpression>

/*! \cond PRIVATE */

namespace QMapLibre::StyleChangeUtils {

QList<QByteArray> allPropertyNamesList(const QObject* object) {
    const QMetaObject* metaObject = object->metaObject();
    QList<QByteArray> propertyNames(object->dynamicPropertyNames());
    for (int i = metaObject->propertyOffset(); i < metaObject->propertyCount(); ++i) {
        propertyNames.append(metaObject->property(i).name());
    }
    return propertyNames;
}

QByteArray formatPropertyName(const QString& name) {
    static const QRegularExpression camelCaseRegex(QStringLiteral("([a-z0-9])([A-Z])"));
    return QString(name).replace(camelCaseRegex, QStringLiteral("\\1-\\2")).toLower().toLatin1();
}

QByteArray formatPropertyName(const QByteArray& name) {
    const QString nameAsString = QString::fromLatin1(name);
    return formatPropertyName(nameAsString);
}

} // namespace QMapLibre::StyleChangeUtils

/*! \endcond */
