// Copyright (C) 2024 MapLibre contributors
// SPDX-License-Identifier: BSD-2-Clause

#include "texture_node_vulkan.hpp"

#include <QtQuick/qsgtexture_platform.h>
#include <vulkan/vulkan.h>
#include <QtCore/QDebug>
#include <QtCore/QPointer>
#include <QtQuick/QSGRendererInterface>
#include <mbgl/vulkan/texture2d.hpp>

namespace QMapLibre {

TextureNodeVulkan::TextureNodeVulkan(const Settings &settings,
                                     const QSize &size,
                                     qreal pixelRatio,
                                     QGeoMapMapLibre *geoMap)
    : TextureNodeBase(settings, size, pixelRatio, geoMap) {
    // Vulkan has inverted Y-axis compared to OpenGL, so we don't need to mirror
    setTextureCoordinatesTransform(QSGSimpleTextureNode::NoTransform);
}

TextureNodeVulkan::~TextureNodeVulkan() {
    // Clean up texture wrapper if needed
    if (m_qtTextureWrapper) {
        delete m_qtTextureWrapper;
        m_qtTextureWrapper = nullptr;
    }
}

void TextureNodeVulkan::resize(const QSize &size, qreal pixelRatio, QQuickWindow * /* window */) {
    m_size = size.expandedTo(QSize(64, 64));
    m_pixelRatio = pixelRatio;
    m_map->resize(m_size);
}

void TextureNodeVulkan::render(QQuickWindow *window) {
    if (!m_map) {
        return;
    }

    // Check for valid size
    if (m_size.isEmpty()) {
        return;
    }

    try {
        // Ensure renderer is created first
        if (!m_rendererBound) {
            // Try to get Qt's Vulkan info and use Qt's device
            auto *ri = window->rendererInterface();
            if (ri && ri->graphicsApi() == QSGRendererInterface::Vulkan) {
                // Qt returns pointers to the handles, not the handles themselves
                auto *qtPhysicalDevicePtr = reinterpret_cast<VkPhysicalDevice *>(
                    ri->getResource(window, QSGRendererInterface::PhysicalDeviceResource));
                auto *qtDevicePtr = reinterpret_cast<VkDevice *>(
                    ri->getResource(window, QSGRendererInterface::DeviceResource));

                VkPhysicalDevice qtPhysicalDevice = qtPhysicalDevicePtr ? *qtPhysicalDevicePtr : VK_NULL_HANDLE;
                VkDevice qtDevice = qtDevicePtr ? *qtDevicePtr : VK_NULL_HANDLE;

                if (qtPhysicalDevice && qtDevice) {
                    // TODO: We need to get the graphics queue index from Qt
                    // For now, assume it's 0 (common case)
                    uint32_t graphicsQueueIndex = 0;

                    m_map->createRendererWithQtVulkanDevice(window, qtPhysicalDevice, qtDevice, graphicsQueueIndex);
                } else {
                    m_map->createRendererWithVulkanWindow(window);
                }
            } else {
                m_map->createRendererWithVulkanWindow(window);
            }
            m_rendererBound = true;
        }

        // Update map size if needed
        const QSize mapSize(static_cast<int>(m_size.width() * m_pixelRatio),
                            static_cast<int>(m_size.height() * m_pixelRatio));
        m_map->resize(mapSize);

        // Ensure rendering happens before getting texture
        if (m_rendererBound && m_map) {
            try {
                // Begin external commands before MapLibre render
                window->beginExternalCommands();
                m_map->render();
                // End external commands after MapLibre render
                window->endExternalCommands();
            } catch (const std::exception &e) {
                qWarning() << "TextureNodeVulkan::render - Exception during render:" << e.what();
                return;
            }
        }

        // Get the Vulkan texture directly for zero-copy access
        auto *vulkanTexture = m_map->getVulkanTexture();

        if (vulkanTexture) {
            // Get Vulkan image and layout
            VkImage vkImage = vulkanTexture->getVulkanImage();
            VkImageLayout imageLayout = static_cast<VkImageLayout>(vulkanTexture->getVulkanImageLayout());

            // Check if we have a valid VkImage
            if (vkImage != VK_NULL_HANDLE) {
                QSGTexture *qtTexture = nullptr;

                // Check if we can reuse existing texture wrapper
                if (m_lastVkImage == vkImage && m_qtTextureWrapper && m_lastTextureSize.width() == mapSize.width() &&
                    m_lastTextureSize.height() == mapSize.height()) {
                    // Reuse existing wrapper for better performance
                    qtTexture = m_qtTextureWrapper;
                } else {
                    // Create new wrapper
                    qtTexture = QNativeInterface::QSGVulkanTexture::fromNative(
                        vkImage, imageLayout, window, mapSize, QQuickWindow::TextureHasAlphaChannel);

                    if (qtTexture) {
                        // Store for reuse
                        m_qtTextureWrapper = qtTexture;
                        m_lastVkImage = vkImage;
                        m_lastTextureSize = mapSize;
                    }
                }

                if (qtTexture) {
                    qtTexture->setFiltering(QSGTexture::Linear);
                    qtTexture->setMipmapFiltering(QSGTexture::None);
                    setTexture(qtTexture);
                    setRect(QRectF(QPointF(), m_size));
                    setOwnsTexture(false); // Don't delete - we manage it
                    markDirty(QSGNode::DirtyMaterial | QSGNode::DirtyGeometry);
                }
            }
        }
    } catch (const std::exception &e) {
        qWarning() << "TextureNodeVulkan::render - Exception:" << e.what();
    }
}

} // namespace QMapLibre
