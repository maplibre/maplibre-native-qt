// Copyright (C) 2022 MapLibre contributors
// Copyright (C) 2017 The Qt Company Ltd.
// Copyright (C) 2017 Mapbox, Inc.

// SPDX-License-Identifier: LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGEOTILEDMAPPINGMANAGERENGINEMAPLIBREGL_H
#define QGEOTILEDMAPPINGMANAGERENGINEMAPLIBREGL_H

#include <QtLocation/QGeoServiceProvider>
#include <QtLocation/private/qgeomappingmanagerengine_p.h>

#include <QMapboxGL>

QT_BEGIN_NAMESPACE

class QGeoMappingManagerEngineMapLibreGL : public QGeoMappingManagerEngine
{
    Q_OBJECT

public:
    QGeoMappingManagerEngineMapLibreGL(const QVariantMap &parameters,
                                        QGeoServiceProvider::Error *error, QString *errorString);
    ~QGeoMappingManagerEngineMapLibreGL();

    QGeoMap *createMap() override;

private:
    QMapboxGLSettings m_settings;
    bool m_useFBO = true;
    QString m_mapItemsBefore;
};

QT_END_NAMESPACE

#endif // QGEOTILEDMAPPINGMANAGERENGINEMAPLIBREGL_H
