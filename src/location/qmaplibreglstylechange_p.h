// Copyright (C) 2023 MapLibre contributors
// Copyright (C) 2017 Mapbox, Inc.

// SPDX-License-Identifier: LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMAPLIBREGLSTYLECHANGE_P_H
#define QQMAPLIBREGLSTYLECHANGE_P_H

#include <QtCore/QList>
#include <QtCore/QSharedPointer>
#include <QtCore/QString>
#include <QtCore/QVariant>
#include <QtCore/QVariantMap>
#include <QtGui/QImage>
#include <QtLocation/private/qdeclarativecirclemapitem_p.h>
#include <QtLocation/private/qdeclarativegeomapitembase_p.h>
#include <QtLocation/private/qdeclarativepolygonmapitem_p.h>
#include <QtLocation/private/qdeclarativepolylinemapitem_p.h>
#include <QtLocation/private/qdeclarativerectanglemapitem_p.h>

#include <QMapLibreGL/Types>

namespace QMapLibreGL {
    class Map;
}

class QMapLibreGLStyleChange
{
public:
    virtual ~QMapLibreGLStyleChange() = default;

    static QList<QSharedPointer<QMapLibreGLStyleChange>> addMapItem(QDeclarativeGeoMapItemBase *, const QString &before);
    static QList<QSharedPointer<QMapLibreGLStyleChange>> removeMapItem(QDeclarativeGeoMapItemBase *);

    virtual void apply(QMapLibreGL::Map *map) = 0;
};

class QMapLibreGLStyleSetLayoutProperty : public QMapLibreGLStyleChange
{
public:
    static QList<QSharedPointer<QMapLibreGLStyleChange>> fromMapItem(QDeclarativeGeoMapItemBase *);

    void apply(QMapLibreGL::Map *map) override;

private:
    static QList<QSharedPointer<QMapLibreGLStyleChange>> fromMapItem(QDeclarativePolylineMapItem *);

    QMapLibreGLStyleSetLayoutProperty() = default;
    QMapLibreGLStyleSetLayoutProperty(const QString &layer, const QString &property, const QVariant &value);

    QString m_layer;
    QString m_property;
    QVariant m_value;
};

class QMapLibreGLStyleSetPaintProperty : public QMapLibreGLStyleChange
{
public:
    static QList<QSharedPointer<QMapLibreGLStyleChange>> fromMapItem(QDeclarativeGeoMapItemBase *);

    void apply(QMapLibreGL::Map *map) override;

private:
    static QList<QSharedPointer<QMapLibreGLStyleChange>> fromMapItem(QDeclarativeRectangleMapItem *);
    static QList<QSharedPointer<QMapLibreGLStyleChange>> fromMapItem(QDeclarativeCircleMapItem *);
    static QList<QSharedPointer<QMapLibreGLStyleChange>> fromMapItem(QDeclarativePolygonMapItem *);
    static QList<QSharedPointer<QMapLibreGLStyleChange>> fromMapItem(QDeclarativePolylineMapItem *);

    QMapLibreGLStyleSetPaintProperty() = default;
    QMapLibreGLStyleSetPaintProperty(const QString &layer, const QString &property, const QVariant &value);

    QString m_layer;
    QString m_property;
    QVariant m_value;
};

class QMapLibreGLStyleAddLayer : public QMapLibreGLStyleChange
{
public:
    static QSharedPointer<QMapLibreGLStyleChange> fromFeature(const QMapLibreGL::Feature &feature, const QString &before);

    void apply(QMapLibreGL::Map *map) override;

private:
    QMapLibreGLStyleAddLayer() = default;

    QVariantMap m_params;
    QString m_before;
};

class QMapLibreGLStyleRemoveLayer : public QMapLibreGLStyleChange
{
public:
    explicit QMapLibreGLStyleRemoveLayer(const QString &id);

    void apply(QMapLibreGL::Map *map) override;

private:
    QMapLibreGLStyleRemoveLayer() = default;

    QString m_id;
};

class QMapLibreGLStyleAddSource : public QMapLibreGLStyleChange
{
public:
    static QSharedPointer<QMapLibreGLStyleChange> fromFeature(const QMapLibreGL::Feature &feature);
    static QSharedPointer<QMapLibreGLStyleChange> fromMapItem(QDeclarativeGeoMapItemBase *);

    void apply(QMapLibreGL::Map *map) override;

private:
    QMapLibreGLStyleAddSource() = default;

    QString m_id;
    QVariantMap m_params;
};

class QMapLibreGLStyleRemoveSource : public QMapLibreGLStyleChange
{
public:
    explicit QMapLibreGLStyleRemoveSource(const QString &id);

    void apply(QMapLibreGL::Map *map) override;

private:
    QMapLibreGLStyleRemoveSource() = default;

    QString m_id;
};

class QMapLibreGLStyleSetFilter : public QMapLibreGLStyleChange
{
public:
    void apply(QMapLibreGL::Map *map) override;

private:
    QMapLibreGLStyleSetFilter() = default;

    QString m_layer;
    QVariant m_filter;
};

class QMapLibreGLStyleAddImage : public QMapLibreGLStyleChange
{
public:
    void apply(QMapLibreGL::Map *map) override;

private:
    QMapLibreGLStyleAddImage() = default;

    QString m_name;
    QImage m_sprite;
};

#endif // QQMAPLIBREGLSTYLECHANGE_P_H
