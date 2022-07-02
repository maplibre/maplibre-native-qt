// Copyright (C) 2022 MapLibre contributors
// Copyright (C) 2017 The Qt Company Ltd.
// Copyright (C) 2017 Mapbox, Inc.

// SPDX-License-Identifier: LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGEOMAPMAPLIBREGL_H
#define QGEOMAPMAPLIBREGL_H

#include "qgeomappingmanagerenginemaplibregl.h"
#include <QtLocation/private/qgeomap_p.h>
#include <QtLocation/private/qgeomapparameter_p.h>

#include <QMapLibreGL/Map>

class QGeoMapMapLibreGLPrivate;

class QGeoMapMapLibreGL : public QGeoMap
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QGeoMapMapLibreGL)

public:
    QGeoMapMapLibreGL(QGeoMappingManagerEngineMapLibreGL *engine, QObject *parent);
    virtual ~QGeoMapMapLibreGL();

    QString copyrightsStyleSheet() const override;
    void setMapLibreGLSettings(const QMapLibreGL::Settings &);
    void setUseFBO(bool);
    void setMapItemsBefore(const QString &);
    Capabilities capabilities() const override;

private Q_SLOTS:
    // QMapLibreGL
    void onMapChanged(QMapLibreGL::Map::MapChange);

    // QDeclarativeGeoMapItemBase
    void onMapItemPropertyChanged();
    void onMapItemSubPropertyChanged();
    void onMapItemUnsupportedPropertyChanged();
    void onMapItemGeometryChanged();

    // QGeoMapParameter
    void onParameterPropertyUpdated(QGeoMapParameter *param, const char *propertyName);

public Q_SLOTS:
    void copyrightsChanged(const QString &copyrightsHtml);

private:
    QSGNode *updateSceneGraph(QSGNode *oldNode, QQuickWindow *window) override;

    QGeoMappingManagerEngineMapLibreGL *m_engine;
};

#endif // QGEOMAPMAPLIBREGL_H
