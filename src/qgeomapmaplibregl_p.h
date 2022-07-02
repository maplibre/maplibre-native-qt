// Copyright (C) 2022 MapLibre contributors
// Copyright (C) 2017 The Qt Company Ltd.
// Copyright (C) 2017 Mapbox, Inc.

// SPDX-License-Identifier: LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGEOMAPMAPLIBREGL_P_H
#define QGEOMAPMAPLIBREGL_P_H

#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QSharedPointer>
#include <QtCore/QTimer>
#include <QtCore/QVariant>
#include <QtCore/QRectF>
#include <QtLocation/private/qgeomap_p_p.h>
#include <QtLocation/private/qgeomapparameter_p.h>

class QMapboxGL;
class QMapLibreGLStyleChange;

class QGeoMapMapLibreGLPrivate : public QGeoMapPrivate
{
    Q_DECLARE_PUBLIC(QGeoMapMapLibreGL)

public:
    QGeoMapMapLibreGLPrivate(QGeoMappingManagerEngineMapLibreGL *engine);

    ~QGeoMapMapLibreGLPrivate();

    QSGNode *updateSceneGraph(QSGNode *oldNode, QQuickWindow *window);

    void addParameter(QGeoMapParameter *param) override;
    void removeParameter(QGeoMapParameter *param) override;

    QGeoMap::ItemTypes supportedMapItemTypes() const override;
    void addMapItem(QDeclarativeGeoMapItemBase *item) override;
    void removeMapItem(QDeclarativeGeoMapItemBase *item) override;

    /* Data members */
    enum SyncState : int {
        NoSync = 0,
        ViewportSync    = 1 << 0,
        CameraDataSync  = 1 << 1,
        MapTypeSync     = 1 << 2,
        VisibleAreaSync = 1 << 3
    };
    Q_DECLARE_FLAGS(SyncStates, SyncState);

    QMapboxGLSettings m_settings;
    bool m_useFBO = true;
    QString m_mapItemsBefore;

    QTimer m_refresh;
    bool m_shouldRefresh = true;
    bool m_warned = false;
    bool m_threadedRendering = false;
    bool m_styleLoaded = false;

    SyncStates m_syncState = NoSync;

    QList<QSharedPointer<QMapLibreGLStyleChange>> m_styleChanges;

protected:
    void changeViewportSize(const QSize &size) override;
    void changeCameraData(const QGeoCameraData &oldCameraData) override;
    void changeActiveMapType(const QGeoMapType mapType) override;

    void setVisibleArea(const QRectF &visibleArea) override;
    QRectF visibleArea() const override;

private:
    Q_DISABLE_COPY(QGeoMapMapLibreGLPrivate);

    void syncStyleChanges(QMapboxGL *map);
    void threadedRenderingHack(QQuickWindow *window, QMapboxGL *map);

    QRectF m_visibleArea;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QGeoMapMapLibreGLPrivate::SyncStates)

#endif // QGEOMAPMAPLIBREGL_P_H
