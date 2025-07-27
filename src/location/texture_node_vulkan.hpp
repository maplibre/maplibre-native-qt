// Copyright (C) 2024 MapLibre contributors
// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include "texture_node_base.hpp"

namespace QMapLibre {

class TextureNodeVulkan : public TextureNodeBase {
public:
    TextureNodeVulkan(const Settings &settings, const QSize &size, qreal pixelRatio, QGeoMapMapLibre *geoMap);

    void resize(const QSize &size, qreal pixelRatio, QQuickWindow *window) override;
    void render(QQuickWindow *window) override;
};

} // namespace QMapLibre
