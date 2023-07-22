// Copyright (C) 2023 MapLibre contributors
// Copyright (C) 2017 The Qt Company Ltd.
// Copyright (C) 2017 Mapbox, Inc.

// SPDX-License-Identifier: LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include <QMapLibre/Map>

#include <QtLocation/private/qgeomap_p.h>

namespace QMapLibre {

class QGeoMapMapLibrePrivate;

class QGeoMapMapLibre : public QGeoMap {
    Q_OBJECT
    Q_DECLARE_PRIVATE(QGeoMapMapLibre)

public:
    QGeoMapMapLibre(QGeoMappingManagerEngine *engine, QObject *parent = nullptr);
    virtual ~QGeoMapMapLibre();

    Capabilities capabilities() const override;

    void setSettings(const Settings &settings);
    void setMapItemsBefore(const QString &mapItemsBefore);

private Q_SLOTS:
    // QMapLibre
    void onMapChanged(Map::MapChange);

    // QDeclarativeGeoMapItemBase
    void onMapItemPropertyChanged();
    void onMapItemSubPropertyChanged();
    void onMapItemUnsupportedPropertyChanged();
    void onMapItemGeometryChanged();

private:
    QSGNode *updateSceneGraph(QSGNode *oldNode, QQuickWindow *window) override;
};

} // namespace QMapLibre
