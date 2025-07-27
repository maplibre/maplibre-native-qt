// Copyright (C) 2024 MapLibre contributors
// SPDX-License-Identifier: BSD-2-Clause

#include "texture_node_vulkan.hpp"

#include <QtQuick/qsgtexture_platform.h>
#include <mbgl/vulkan/texture2d.hpp>

namespace QMapLibre {

TextureNodeVulkan::TextureNodeVulkan(const Settings &settings, const QSize &size, qreal pixelRatio, QGeoMapMapLibre *geoMap)
    : TextureNodeBase(settings, size, pixelRatio, geoMap) {
}

void TextureNodeVulkan::resize(const QSize &size, qreal pixelRatio, QQuickWindow * /* window */) {
    m_size = size.expandedTo(QSize(64, 64));
    m_pixelRatio = pixelRatio;
    m_map->resize(m_size);
}

void TextureNodeVulkan::render(QQuickWindow *window) {
    m_map->render();
    
    // Get the Vulkan texture directly for zero-copy access
    auto *vulkanTexture = m_map->getVulkanTexture();
    
    if (vulkanTexture) {
        // Get Vulkan image and layout
        VkImage vkImage = vulkanTexture->getVulkanImage();
        VkImageLayout imageLayout = static_cast<VkImageLayout>(vulkanTexture->getVulkanImageLayout());
        
        // Check if we have a valid VkImage
        if (vkImage != VK_NULL_HANDLE) {
            // Create QSGTexture from native Vulkan image (zero-copy)
            QSGTexture *qtTexture = QNativeInterface::QSGVulkanTexture::fromNative(
                vkImage,
                imageLayout,
                window,
                QSize(m_size.width() * m_pixelRatio, m_size.height() * m_pixelRatio),
                QQuickWindow::TextureHasAlphaChannel);
            
            if (qtTexture) {
                qtTexture->setFiltering(QSGTexture::Linear);
                qtTexture->setMipmapFiltering(QSGTexture::None);
                setTexture(qtTexture);
                setOwnsTexture(true);
                markDirty(QSGNode::DirtyMaterial);
            }
        }
    }
}

} // namespace QMapLibre