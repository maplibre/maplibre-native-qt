// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include "declarative_style_parameter.hpp"

#include <QMapLibre/LayerParameter>

#include <QtQml/QQmlEngine>
#include <QtQml/QQmlParserStatus>

namespace QMapLibre {

class DeclarativeLayerParameter : public LayerParameter, public QQmlParserStatus {
    Q_OBJECT
    QML_NAMED_ELEMENT(LayerParameter)
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QML_ADDED_IN_VERSION(3, 0)
#endif
    Q_INTERFACES(QQmlParserStatus)
    // from base class
    Q_PROPERTY(QString styleId READ styleId WRITE setStyleId)
    Q_PROPERTY(QString type READ type WRITE setType)
    Q_PROPERTY(QJsonObject layout READ layout WRITE setLayout NOTIFY layoutUpdated)
    Q_PROPERTY(QJsonObject paint READ paint WRITE setPaint NOTIFY paintUpdated)
    // this type must not declare any additional properties
public:
    explicit DeclarativeLayerParameter(QObject *parent = nullptr);
    ~DeclarativeLayerParameter() override = default;

private:
    // QQmlParserStatus implementation
    MLN_DECLARATIVE_PARSER(DeclarativeLayerParameter)
};

} // namespace QMapLibre
