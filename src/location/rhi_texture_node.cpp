// Copyright (C) 2024 MapLibre contributors
// SPDX-License-Identifier: BSD-2-Clause

#include "rhi_texture_node.hpp"

#include <QtQuick/QQuickWindow>
#include <QtGui/QRhiTexture>
#include <QtQuick/QSGTexture>

namespace QMapLibre {

RhiTextureNode::RhiTextureNode(QQuickWindow* win) : m_window(win) {
    setTextureCoordinatesTransform(QSGSimpleTextureNode::MirrorVertically);
    setFiltering(QSGTexture::Linear);
}

void RhiTextureNode::syncWithNativeTexture(const void* /*nativeTex*/, int /*w*/, int /*h*/) {
    // Placeholder – implemented in a later step.
    Q_UNUSED(nativeTex);
    Q_UNUSED(w);
    Q_UNUSED(h);
}

} // namespace QMapLibre 