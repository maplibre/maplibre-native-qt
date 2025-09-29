// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include "declarative_style_parameter.hpp"

#include <QMapLibre/ImageParameter>

#include <QtQml/QQmlEngine>
#include <QtQml/QQmlParserStatus>

namespace QMapLibre {

class DeclarativeImageParameter : public ImageParameter, public QQmlParserStatus {
    Q_OBJECT
    QML_NAMED_ELEMENT(ImageParameter)
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QML_ADDED_IN_VERSION(3, 0)
#endif
    Q_INTERFACES(QQmlParserStatus)
    // from base class
    Q_PROPERTY(QString styleId READ styleId WRITE setStyleId)
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceUpdated)
    // this type must not declare any additional properties
public:
    explicit DeclarativeImageParameter(QObject* parent = nullptr);
    ~DeclarativeImageParameter() override = default;

private:
    // QQmlParserStatus implementation
    MLN_DECLARATIVE_PARSER(DeclarativeImageParameter)
};

} // namespace QMapLibre
