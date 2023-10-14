// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#ifndef QMAPLIBREQMLTYPES_H
#define QMAPLIBREQMLTYPES_H

#include "../qgeomap.hpp"

#include <QtLocation/private/qdeclarativegeomap_p.h>
#include <QtQml/qqml.h>
#include <QtCore/QObject>

class MapLibreStyleAttached : public QObject {
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(QString style READ style WRITE setStyle NOTIFY styleChanged)
public:
    MapLibreStyleAttached(QObject *parent)
        : QObject(parent) {}

    QString style() const { return m_style; }
    void setStyle(const QString &style) {
        qDebug() << "Setting style to" << style;
        if (style == m_style) return;
        m_style = style;
        emit styleChanged(m_style);
    }

Q_SIGNALS:
    void styleChanged(QString style);

private:
    QString m_style;
};

class MapLibreStyleProperties : public QObject {
    Q_OBJECT
    QML_ATTACHED(MapLibreStyleAttached)
    QML_NAMED_ELEMENT(MapLibre)

public:
    static MapLibreStyleAttached *qmlAttachedProperties(QObject *object) {
        qDebug() << "Attaching to" << object;
        return new MapLibreStyleAttached(object);
    }
};

#endif // QMAPLIBREQMLTYPES_H
