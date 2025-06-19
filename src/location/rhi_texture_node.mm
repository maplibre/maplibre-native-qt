// Copyright (C) 2024 MapLibre contributors
// SPDX-License-Identifier: BSD-2-Clause

#include "rhi_texture_node.hpp"

#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGTexture>

// Need Metal imports for id<MTLTexture>
#include <Metal/Metal.h>

namespace QMapLibre {

RhiTextureNode::RhiTextureNode(QQuickWindow* win) : m_window(win) {
    setTextureCoordinatesTransform(QSGSimpleTextureNode::MirrorVertically);
    setFiltering(QSGTexture::Linear);
}

void RhiTextureNode::syncWithNativeTexture(const void* nativeTex, int w, int h)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
    id<MTLTexture> mtlTex = static_cast<id<MTLTexture>>(const_cast<void*>(nativeTex));
    if (!mtlTex) {
        return;
    }

    const QSize newSize(w, h);

    // Re-create QSGTexture every frame for simplicity.
    m_qsgTexture.reset(::QNativeInterface::QSGMetalTexture::fromNative(
        mtlTex, m_window, newSize, QQuickWindow::TextureHasAlphaChannel));
    m_size = newSize;
    setTexture(m_qsgTexture.get());
    setOwnsTexture(false);
    markDirty(QSGNode::DirtyMaterial);
#else
    Q_UNUSED(nativeTex)
    Q_UNUSED(w)
    Q_UNUSED(h)
#endif
#else
    Q_UNUSED(nativeTex)
    Q_UNUSED(w)
    Q_UNUSED(h)
#endif
}

} // namespace QMapLibre 