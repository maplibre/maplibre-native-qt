// Copyright (C) 2024 MapLibre contributors
// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include "qgeomap.hpp"

namespace QMapLibre {

class Q_MAPLIBRE_LOCATION_EXPORT QGeoMapMapLibreVulkan : public QGeoMapMapLibre {
    Q_OBJECT

public:
    explicit QGeoMapMapLibreVulkan(QGeoMappingManagerEngine *engine, QObject *parent = nullptr);
    ~QGeoMapMapLibreVulkan() override;

private:
    QSGNode *updateSceneGraph(QSGNode *oldNode, QQuickWindow *window) override;
};

} // namespace QMapLibre