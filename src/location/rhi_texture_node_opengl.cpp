// Copyright (C) 2024 MapLibre contributors
// SPDX-License-Identifier: BSD-2-Clause

#include "rhi_texture_node.hpp"

#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGTexture>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>

namespace QMapLibre {

RhiTextureNode::RhiTextureNode(QQuickWindow* win) : m_window(win) {
    setTextureCoordinatesTransform(QSGSimpleTextureNode::MirrorVertically);
    setFiltering(QSGTexture::Linear);
}

void RhiTextureNode::syncWithNativeTexture(const void* nativeTex, int w, int h)
{
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
}

} // namespace QMapLibre
