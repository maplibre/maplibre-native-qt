// Copyright (C) 2024 MapLibre contributors
// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include "texture_node_base.hpp"

namespace QMapLibre {

class TextureNodeMetal : public TextureNodeBase {
public:
    TextureNodeMetal(const Settings &settings, const QSize &size, qreal pixelRatio, QGeoMapMapLibre *geoMap);
    ~TextureNodeMetal();

    void resize(const QSize &size, qreal pixelRatio, QQuickWindow *window) override;
    void render(QQuickWindow *window) override;

private:
    bool m_rendererBound = false;
    void *m_layerPtr = nullptr;
    void *m_currentDrawable = nullptr;
    bool m_ownsLayer = false;
};

} // namespace QMapLibre
