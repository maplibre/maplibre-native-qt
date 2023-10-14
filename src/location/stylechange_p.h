// Copyright (C) 2023 MapLibre contributors
// Copyright (C) 2017 Mapbox, Inc.

// SPDX-License-Identifier: LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include <QtLocation/private/qdeclarativecirclemapitem_p.h>
#include <QtLocation/private/qdeclarativegeomapitembase_p.h>
#include <QtLocation/private/qdeclarativepolygonmapitem_p.h>
#include <QtLocation/private/qdeclarativepolylinemapitem_p.h>
#include <QtLocation/private/qdeclarativerectanglemapitem_p.h>

#include <QtCore/QList>
#include <QtCore/QSharedPointer>
#include <QtCore/QString>
#include <QtCore/QVariant>
#include <QtCore/QVariantMap>
#include <QtGui/QImage>

#include <QMapLibre/Types>

namespace QMapLibre {
class Map;

class StyleChange {
public:
    virtual ~StyleChange() = default;

    static QList<QSharedPointer<StyleChange>> addMapItem(QDeclarativeGeoMapItemBase *, const QString &before);
    static QList<QSharedPointer<StyleChange>> removeMapItem(QDeclarativeGeoMapItemBase *);

    virtual void apply(Map *map) = 0;
};

class StyleSetLayoutProperty : public StyleChange {
public:
    static QList<QSharedPointer<StyleChange>> fromMapItem(QDeclarativeGeoMapItemBase *);

    void apply(Map *map) override;

private:
    static QList<QSharedPointer<StyleChange>> fromMapItem(QDeclarativePolylineMapItem *);

    StyleSetLayoutProperty() = default;
    StyleSetLayoutProperty(const QString &layer, const QString &property, const QVariant &value);

    QString m_layer;
    QString m_property;
    QVariant m_value;
};

class StyleSetPaintProperty : public StyleChange {
public:
    static QList<QSharedPointer<StyleChange>> fromMapItem(QDeclarativeGeoMapItemBase *);

    void apply(Map *map) override;

private:
    static QList<QSharedPointer<StyleChange>> fromMapItem(QDeclarativeRectangleMapItem *);
    static QList<QSharedPointer<StyleChange>> fromMapItem(QDeclarativeCircleMapItem *);
    static QList<QSharedPointer<StyleChange>> fromMapItem(QDeclarativePolygonMapItem *);
    static QList<QSharedPointer<StyleChange>> fromMapItem(QDeclarativePolylineMapItem *);

    StyleSetPaintProperty() = default;
    StyleSetPaintProperty(const QString &layer, const QString &property, const QVariant &value);

    QString m_layer;
    QString m_property;
    QVariant m_value;
};

class StyleAddLayer : public StyleChange {
public:
    static QSharedPointer<StyleChange> fromFeature(const Feature &feature, const QString &before);

    void apply(Map *map) override;

private:
    StyleAddLayer() = default;

    QVariantMap m_params;
    QString m_before;
};

class StyleRemoveLayer : public StyleChange {
public:
    explicit StyleRemoveLayer(const QString &id);

    void apply(Map *map) override;

private:
    StyleRemoveLayer() = default;

    QString m_id;
};

class StyleAddSource : public StyleChange {
public:
    static QSharedPointer<StyleChange> fromFeature(const Feature &feature);
    static QSharedPointer<StyleChange> fromMapItem(QDeclarativeGeoMapItemBase *);

    void apply(Map *map) override;

private:
    StyleAddSource() = default;

    QString m_id;
    QVariantMap m_params;
};

class StyleRemoveSource : public StyleChange {
public:
    explicit StyleRemoveSource(const QString &id);

    void apply(Map *map) override;

private:
    StyleRemoveSource() = default;

    QString m_id;
};

class StyleSetFilter : public StyleChange {
public:
    void apply(Map *map) override;

private:
    StyleSetFilter() = default;

    QString m_layer;
    QVariant m_filter;
};

class StyleAddImage : public StyleChange {
public:
    void apply(Map *map) override;

private:
    StyleAddImage() = default;

    QString m_name;
    QImage m_sprite;
};

} // namespace QMapLibre
