// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include "export_quick_p.hpp"
#include "texture_node_base_p.hpp"

#include <vulkan/vulkan.h>

namespace QMapLibre {

class Q_MAPLIBRE_QUICKPRIVATE_EXPORT TextureNodeVulkan final : public TextureNodeBase {
public:
    explicit TextureNodeVulkan(const Settings &settings, const QSize &size, qreal pixelRatio);
    explicit TextureNodeVulkan(std::shared_ptr<Map> map, const QSize &size, qreal pixelRatio);
    ~TextureNodeVulkan() final;

    void resize(const QSize &size, qreal pixelRatio, QQuickWindow *window) final;
    void render(QQuickWindow *window) final;

private:
    bool m_rendererBound{};
    QPointer<QSGTexture> m_qtTextureWrapper;
    VkImage m_lastVulkanImage{VK_NULL_HANDLE};
    QSize m_lastTextureSize;
};

} // namespace QMapLibre
