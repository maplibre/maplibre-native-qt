// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include <QMapLibre/StyleParameter>

#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickItem>

namespace QMapLibre {

class MapQuickItem;

class MapQuickStyle : public QQuickItem {
    Q_OBJECT
    QML_NAMED_ELEMENT(Style)
    QML_ADDED_IN_VERSION(4, 0)

public:
    explicit MapQuickStyle(QQuickItem *parent = nullptr);
    ~MapQuickStyle() override = default;

    Q_INVOKABLE void addParameter(StyleParameter *parameter);
    Q_INVOKABLE void removeParameter(StyleParameter *parameter);
    Q_INVOKABLE void clearParameters();
    QList<QObject *> parameters();

protected:
    void componentComplete() override;

private slots:
    void onParentChanged(QQuickItem *parent);

private:
    void populateParameters();

    MapQuickItem *m_map{};

    QList<StyleParameter *> m_parameters;
};

} // namespace QMapLibre

QML_DECLARE_TYPE(QMapLibre::MapQuickStyle)
