// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#include "texture_node_vulkan_p.hpp"

#ifdef Q_OS_ANDROID
#define VK_USE_PLATFORM_ANDROID_KHR
#include <vulkan/vulkan_android.h>
#endif
#include <vulkan/vulkan.hpp>

#include <mbgl/vulkan/texture2d.hpp>

#include <QtCore/QDebug>
#include <QtCore/QPointer>
#include <QtQuick/QSGRendererInterface>

namespace {
constexpr int DefaultSize{64};
} // namespace

namespace QMapLibre {

TextureNodeVulkan::TextureNodeVulkan(const Settings &settings, const QSize &size, qreal pixelRatio)
    : TextureNodeBase(settings, size, pixelRatio) {
    // Vulkan has inverted Y-axis compared to OpenGL, so we don't need to mirror
    setTextureCoordinatesTransform(QSGSimpleTextureNode::NoTransform);
}

TextureNodeVulkan::TextureNodeVulkan(std::shared_ptr<Map> map, const QSize &size, qreal pixelRatio)
    : TextureNodeBase(std::move(map), size, pixelRatio) {
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
    m_size = size.expandedTo(QSize(DefaultSize, DefaultSize));
    m_pixelRatio = pixelRatio;
    m_map->resize(m_size, m_pixelRatio);
}

void TextureNodeVulkan::render(QQuickWindow *window) {
    if (!m_map) {
        return;
    }

    // Check for valid size
    if (m_size.isEmpty()) {
        return;
    }

    // Ensure renderer is created first
    if (!m_rendererBound) {
        // Try to get Qt's Vulkan info and use Qt's device
        auto *ri = window->rendererInterface();
        if (ri != nullptr) {
            // Qt returns pointers to the handles, not the handles themselves
            auto *qtPhysicalDevicePtr = reinterpret_cast<vk::PhysicalDevice *>(
                ri->getResource(window, QSGRendererInterface::PhysicalDeviceResource));
            auto *qtDevicePtr = reinterpret_cast<vk::Device *>(
                ri->getResource(window, QSGRendererInterface::DeviceResource));

            const vk::PhysicalDevice qtPhysicalDevice = qtPhysicalDevicePtr != nullptr ? *qtPhysicalDevicePtr : nullptr;
            const vk::Device qtDevice = qtDevicePtr != nullptr ? *qtDevicePtr : nullptr;
            if (qtPhysicalDevice != nullptr && qtDevice != nullptr) {
                // TODO: We need to get the graphics queue index from Qt
                // For now, assume it's 0 (common case)
                const uint32_t graphicsQueueIndex{};

                m_map->createRendererWithQtVulkanDevice(window, qtPhysicalDevice, qtDevice, graphicsQueueIndex);
            } else {
                m_map->createRenderer(window);
            }
        } else {
            m_map->createRenderer(window);
        }
        m_rendererBound = true;
    }

    // Update map size if needed - pass logical size, mbgl::Map handles DPI internally
    m_map->resize(m_size, m_pixelRatio);

    // Calculate physical size for texture
    const QSize physicalSize(static_cast<int>(m_size.width() * m_pixelRatio),
                             static_cast<int>(m_size.height() * m_pixelRatio));

    // Ensure rendering happens before getting texture
    if (m_rendererBound && m_map != nullptr) {
        // Begin external commands before MapLibre render
        window->beginExternalCommands();
        m_map->render();
        // End external commands after MapLibre render
        window->endExternalCommands();
    }

    // Get the Vulkan texture directly for zero-copy access
    auto *vulkanTexture = m_map->getVulkanTexture();
    if (vulkanTexture != nullptr) {
        // Get Vulkan image and layout
        VkImage vulkanImage{vulkanTexture->getVulkanImage()};
        const auto vulkanImageLayout = static_cast<VkImageLayout>(vulkanTexture->getVulkanImageLayout());

        // Check if we have a valid vk::Image
        if (vulkanImage != VK_NULL_HANDLE) {
            QSGTexture *qtTexture = nullptr;

            // Check if we can reuse existing texture wrapper
            if (m_lastVulkanImage == vulkanImage && m_qtTextureWrapper &&
                m_lastTextureSize.width() == physicalSize.width() &&
                m_lastTextureSize.height() == physicalSize.height()) {
                // Reuse existing wrapper for better performance
                qtTexture = m_qtTextureWrapper;
            } else {
                // Create new wrapper
                qtTexture = QNativeInterface::QSGVulkanTexture::fromNative(
                    vulkanImage, vulkanImageLayout, window, physicalSize, QQuickWindow::TextureHasAlphaChannel);
                if (qtTexture != nullptr) {
                    // Store for reuse
                    m_qtTextureWrapper = qtTexture;
                    m_lastVulkanImage = vulkanImage;
                    m_lastTextureSize = physicalSize;
                }
            }

            if (qtTexture != nullptr) {
                qtTexture->setFiltering(QSGTexture::Linear);
                qtTexture->setMipmapFiltering(QSGTexture::None);
                setTexture(qtTexture);
                setRect(QRectF(QPointF(), m_size));
                setOwnsTexture(false); // Don't delete - we manage it
                markDirty(QSGNode::DirtyMaterial | QSGNode::DirtyGeometry);
            }
        }
    }
}

} // namespace QMapLibre
