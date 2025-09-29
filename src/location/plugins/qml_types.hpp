// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#ifndef QMAPLIBREQMLTYPES_H
#define QMAPLIBREQMLTYPES_H

#include "declarative_style.hpp"

#include <QtLocation/private/qdeclarativegeomap_p.h>

#include <QtQml/QQmlEngine>

class MapLibreStyleAttached : public QObject {
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(QMapLibre::DeclarativeStyle* style READ style WRITE setStyle NOTIFY styleChanged)
public:
    explicit MapLibreStyleAttached(QObject* parent)
        : QObject(parent) {}

    [[nodiscard]] QMapLibre::DeclarativeStyle* style() const { return m_style; }

    void setStyle(QMapLibre::DeclarativeStyle* style) {
        m_style = style;
        Q_EMIT styleChanged(m_style);

        // Check for QQuickItem
        auto* quickItem = qobject_cast<QQuickItem*>(parent());
        if (quickItem == nullptr) {
            qWarning() << "Not a QQuickItem!";
            return;
        }

        // Check for MapView
        QDeclarativeGeoMap* declarativeMap{};
        if (QString(quickItem->metaObject()->className()).startsWith("MapView")) {
            declarativeMap = QQmlProperty::read(quickItem, "map").value<QDeclarativeGeoMap*>();
        } else {
            declarativeMap = qobject_cast<QDeclarativeGeoMap*>(parent());
        }

        if (declarativeMap == nullptr) {
            qWarning() << "Map object not found!";
            return;
        }

        style->setDeclarativeMap(declarativeMap);
    }

Q_SIGNALS:
    void styleChanged(QMapLibre::DeclarativeStyle* style);

private:
    QMapLibre::DeclarativeStyle* m_style{};
};

class MapLibreStyleProperties : public QObject {
    Q_OBJECT
    QML_ATTACHED(MapLibreStyleAttached)
    QML_NAMED_ELEMENT(MapLibre)

public:
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    static MapLibreStyleAttached* qmlAttachedProperties(QObject* object) { return new MapLibreStyleAttached(object); }
};

#endif // QMAPLIBREQMLTYPES_H
