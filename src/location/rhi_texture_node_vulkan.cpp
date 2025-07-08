// Copyright (C) 2024 MapLibre contributors
// SPDX-License-Identifier: BSD-2-Clause

#include "rhi_texture_node.hpp"

#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGTexture>
#include <vulkan/vulkan.h>

namespace QMapLibre {

RhiTextureNode::RhiTextureNode(QQuickWindow* win) : m_window(win) {
    setTextureCoordinatesTransform(QSGSimpleTextureNode::MirrorVertically);
    setFiltering(QSGTexture::Linear);
}

void RhiTextureNode::syncWithNativeTexture(const void* nativeTex, int w, int h)
{
    VkImage vkImage = static_cast<VkImage>(reinterpret_cast<uintptr_t>(nativeTex));
    if (vkImage == VK_NULL_HANDLE)
        return;

    const QSize newSize(w, h);

    // Assume the image is in a layout that is readable by the scene graph. If
    // needed, MapLibre backend can track the actual layout and pass it here.
    constexpr VkImageLayout assumedLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    m_qsgTexture.reset(QNativeInterface::QSGVulkanTexture::fromNative(
        vkImage, assumedLayout, m_window, newSize, QQuickWindow::TextureHasAlphaChannel));

    m_size = {w, h};
    setTexture(m_qsgTexture.get());
    setOwnsTexture(false);
    markDirty(QSGNode::DirtyMaterial);
}

} // namespace QMapLibre
