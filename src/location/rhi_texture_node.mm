// Copyright (C) 2024 MapLibre contributors
// SPDX-License-Identifier: BSD-2-Clause

#include "rhi_texture_node.hpp"

#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGTexture>

// Need Metal imports for id<MTLTexture>
#if defined(MLN_RENDER_BACKEND_METAL)
#    include <Metal/Metal.h>
#elif defined(MLN_RENDER_BACKEND_OPENGL)
#    include <QtGui/QOpenGLContext>
#    include <QtGui/QOpenGLFunctions>
#elif defined(MLN_RENDER_BACKEND_VULKAN)
#    include <vulkan/vulkan.h>
#endif

namespace QMapLibre {

RhiTextureNode::RhiTextureNode(QQuickWindow* win) : m_window(win) {
    setTextureCoordinatesTransform(QSGSimpleTextureNode::MirrorVertically);
    setFiltering(QSGTexture::Linear);
}

void RhiTextureNode::syncWithNativeTexture(const void* nativeTex, int w, int h)
{
#if defined(MLN_RENDER_BACKEND_METAL) && (defined(Q_OS_MACOS) || defined(Q_OS_IOS))
    id<MTLTexture> mtlTex = static_cast<id<MTLTexture>>(const_cast<void*>(nativeTex));
    if (!mtlTex)
        return;

    const QSize newSize(w, h);
    m_qsgTexture.reset(QNativeInterface::QSGMetalTexture::fromNative(
        mtlTex, m_window, newSize, QQuickWindow::TextureHasAlphaChannel));

    m_size = {w, h};
    setTexture(m_qsgTexture.get());
    setOwnsTexture(false);
    markDirty(QSGNode::DirtyMaterial);
#elif defined(MLN_RENDER_BACKEND_OPENGL)
    GLuint glTex = static_cast<GLuint>(reinterpret_cast<uintptr_t>(nativeTex));
    if (!glTex)
        return;

    const QSize newSize(w, h);
    m_qsgTexture.reset(QNativeInterface::QSGOpenGLTexture::fromNative(
        glTex, m_window, newSize, QQuickWindow::TextureHasAlphaChannel));

    m_size = {w, h};
    setTexture(m_qsgTexture.get());
    setOwnsTexture(false);
    markDirty(QSGNode::DirtyMaterial);
#elif defined(MLN_RENDER_BACKEND_VULKAN)
    // TODO: integrate Vulkan swapchain image views when Qt is built with Vulkan support.
    Q_UNUSED(nativeTex);
    Q_UNUSED(w);
    Q_UNUSED(h);
#else
    Q_UNUSED(nativeTex);
    Q_UNUSED(w);
    Q_UNUSED(h);
#endif
}

} // namespace QMapLibre
