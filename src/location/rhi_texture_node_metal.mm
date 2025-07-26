// Copyright (C) 2024 MapLibre contributors
// SPDX-License-Identifier: BSD-2-Clause

#include "rhi_texture_node.hpp"

#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGTexture>
#include <Metal/Metal.h>

namespace QMapLibre {

RhiTextureNode::RhiTextureNode(QQuickWindow* win) : m_window(win) {
    setTextureCoordinatesTransform(QSGSimpleTextureNode::MirrorVertically);
    setFiltering(QSGTexture::Linear);
}

void RhiTextureNode::syncWithNativeTexture(const void* nativeTex, int w, int h)
{
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
}

} // namespace QMapLibre
