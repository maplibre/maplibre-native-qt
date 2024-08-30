// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include "declarative_style_parameter.hpp"

#include <QMapLibre/FilterParameter>

#include <QtQml/QQmlEngine>
#include <QtQml/QQmlParserStatus>

namespace QMapLibre {

class DeclarativeFilterParameter : public FilterParameter, public QQmlParserStatus {
    Q_OBJECT
    QML_NAMED_ELEMENT(FilterParameter)
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QML_ADDED_IN_VERSION(3, 0)
#endif
    Q_INTERFACES(QQmlParserStatus)
    // from base class
    Q_PROPERTY(QString styleId READ styleId WRITE setStyleId)
    Q_PROPERTY(QVariantList expression READ expression WRITE setExpression NOTIFY expressionUpdated)
    // this type must not declare any additional properties
public:
    explicit DeclarativeFilterParameter(QObject *parent = nullptr);
    ~DeclarativeFilterParameter() override = default;

private:
    // QQmlParserStatus implementation
    MLN_DECLARATIVE_PARSER(DeclarativeFilterParameter)
};

} // namespace QMapLibre
