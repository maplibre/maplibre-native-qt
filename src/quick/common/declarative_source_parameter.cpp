// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#include "declarative_source_parameter.hpp"

#include <QtCore/QJsonDocument>

namespace QMapLibre {

DeclarativeSourceParameter::DeclarativeSourceParameter(QObject *parent)
    : SourceParameter(parent) {}

QVariant DeclarativeSourceParameter::parsedProperty(const char *propertyName) const {
    if (!hasProperty(propertyName)) {
        return StyleParameter::parsedProperty(propertyName);
    }

    if (property(propertyName).typeId() < QMetaType::User) {
        return StyleParameter::parsedProperty(propertyName);
    }

    // explicitly handle only "data" for now
    if (qstrcmp(propertyName, "data") != 0) {
        return StyleParameter::parsedProperty(propertyName);
    }

    // convert QJSValue to JSON string
    return QJsonDocument::fromVariant(property(propertyName).value<QJSValue>().toVariant()).toJson();
}

} // namespace QMapLibre
