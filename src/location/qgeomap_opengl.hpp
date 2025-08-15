// Copyright (C) 2023 MapLibre contributors
// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include "qgeomap.hpp"

namespace QMapLibre {

class Q_MAPLIBRE_LOCATION_EXPORT QGeoMapMapLibreOpenGL : public QGeoMapMapLibre {
    Q_OBJECT

public:
    explicit QGeoMapMapLibreOpenGL(QGeoMappingManagerEngine *engine, QObject *parent = nullptr);
    ~QGeoMapMapLibreOpenGL() override;

private:
    QSGNode *updateSceneGraph(QSGNode *oldNode, QQuickWindow *window) override;
};

} // namespace QMapLibre
