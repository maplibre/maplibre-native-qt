// Copyright (C) 2023 MapLibre contributors
// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include <vulkan/vulkan.h>
#include "texture_node_base.hpp"

namespace QMapLibre {

class TextureNodeVulkan : public TextureNodeBase {
public:
    TextureNodeVulkan(const Settings &settings, const QSize &size, qreal pixelRatio, QGeoMapMapLibre *geoMap);
    ~TextureNodeVulkan() override;

    void resize(const QSize &size, qreal pixelRatio, QQuickWindow *window) override;
    void render(QQuickWindow *window) override;

private:
    bool m_rendererBound = false;
    QPointer<QSGTexture> m_qtTextureWrapper;
    VkImage m_lastVkImage = VK_NULL_HANDLE;
    QSize m_lastTextureSize;
};

} // namespace QMapLibre
