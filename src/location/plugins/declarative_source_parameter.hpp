// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include "declarative_style_parameter.hpp"

#include <QMapLibre/SourceParameter>

#include <QtQml/QQmlEngine>
#include <QtQml/QQmlParserStatus>

namespace QMapLibre {

class DeclarativeSourceParameter : public SourceParameter, public QQmlParserStatus {
    Q_OBJECT
    QML_NAMED_ELEMENT(SourceParameter)
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QML_ADDED_IN_VERSION(3, 0)
#endif
    Q_INTERFACES(QQmlParserStatus)
    // from base class
    Q_PROPERTY(QString styleId READ styleId WRITE setStyleId)
    Q_PROPERTY(QString type READ type WRITE setType)
    // this type must not declare any additional properties
public:
    explicit DeclarativeSourceParameter(QObject *parent = nullptr);
    ~DeclarativeSourceParameter() override = default;

    [[nodiscard]] QVariant parsedProperty(const char *propertyName) const override;

private:
    // QQmlParserStatus implementation
    MLN_DECLARATIVE_PARSER(DeclarativeSourceParameter)
};

} // namespace QMapLibre
