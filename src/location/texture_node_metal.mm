// Copyright (C) 2024 MapLibre contributors
// SPDX-License-Identifier: BSD-2-Clause

#include "texture_node_metal.hpp"
#include <Metal/Metal.h>
#include <QtQuick/qsgtexture_platform.h>

namespace QMapLibre {

TextureNodeMetal::TextureNodeMetal(const Settings &settings,
                                   const QSize &size,
                                   qreal pixelRatio,
                                   QGeoMapMapLibre *geoMap)
    : TextureNodeBase(settings, size, pixelRatio, geoMap) {}

void TextureNodeMetal::resize(const QSize &size, qreal pixelRatio, QQuickWindow * /* window */) {
    m_size = size.expandedTo(QSize(64, 64));
    m_pixelRatio = pixelRatio;
    m_map->resize(m_size);
}

void TextureNodeMetal::render(QQuickWindow *window) {
    m_map->render();

    if (void *tex = m_map->nativeColorTexture()) {
        // Use Qt's native interface helper to wrap the Metal texture
        auto mtlTex = (__bridge id<MTLTexture>)tex;
        QSGTexture *qtTex = QNativeInterface::QSGMetalTexture::fromNative(
            mtlTex,
            window,
            QSize(m_size.width() * m_pixelRatio, m_size.height() * m_pixelRatio),
            QQuickWindow::TextureHasAlphaChannel);

        setTexture(qtTex);
        setOwnsTexture(true);
        markDirty(QSGNode::DirtyMaterial);
    }

    setRect(QRectF(QPointF(), m_size));
}

} // namespace QMapLibre
