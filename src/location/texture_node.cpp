// Copyright (C) 2023 MapLibre contributors
// Copyright (C) 2017 The Qt Company Ltd.
// Copyright (C) 2017 Mapbox, Inc.

// SPDX-License-Identifier: LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "texture_node.hpp"
#include "qgeomap.hpp"
#include "rhi_texture_node.hpp"

#if __has_include(<QtQuick/private/qsgplaintexture_p.h>)
#include <QtQuick/private/qsgplaintexture_p.h>
#endif

#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>
#include <QtQuick/QQuickOpenGLUtils>
#include <QtQuick/QSGRendererInterface>

namespace QMapLibre {

static const QSize minTextureSize = QSize(64, 64);

TextureNode::TextureNode(const Settings &settings, const QSize &size, qreal pixelRatio, QGeoMapMapLibre *geoMap) {
    setTextureCoordinatesTransform(QSGSimpleTextureNode::MirrorVertically);
    setFiltering(QSGTexture::Linear);

    m_map = std::make_unique<Map>(nullptr, settings, size.expandedTo(minTextureSize), pixelRatio);

    QObject::connect(m_map.get(), &Map::needsRendering, geoMap, &QGeoMap::sgNodeChanged);
}

void TextureNode::resize(const QSize &size, qreal pixelRatio, QQuickWindow *window)
{
    const QSize &minSize = size.expandedTo(minTextureSize);
    const QSize fbSize = minSize * pixelRatio;

    m_map->resize(minSize);

    m_fbo = std::make_unique<QOpenGLFramebufferObject>(fbSize, QOpenGLFramebufferObject::CombinedDepthStencil);
    m_map->setOpenGLFramebufferObject(m_fbo->handle(), fbSize);

    setTexture(::QNativeInterface::QSGOpenGLTexture::fromNative(
        m_fbo->texture(), window, fbSize, QQuickWindow::TextureHasAlphaChannel));
    setOwnsTexture(true);

    setRect(QRectF(QPointF(), minSize));
    markDirty(QSGNode::DirtyGeometry);
}

void TextureNode::render(QQuickWindow *window) {
    if (window->rendererInterface()->graphicsApi() == QSGRendererInterface::MetalRhi) {
        if (!m_rhiNode) {
            m_rhiNode = new RhiTextureNode(window);
            appendChildNode(m_rhiNode);
        }

        m_map->render();
        if (void *tex = m_map->nativeColorTexture()) {
            const QRectF r = rect();
            m_rhiNode->syncWithNativeTexture(tex, static_cast<int>(r.width()), static_cast<int>(r.height()));
        }
        return;
    }

    QOpenGLFunctions *f = static_cast<QOpenGLContext *>(window->rendererInterface()->getResource(
                                                            window, QSGRendererInterface::OpenGLContextResource))
                              ->functions();

    f->glViewport(0, 0, m_fbo->width(), m_fbo->height());

    GLint alignment{};
    f->glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);

    // Ensure a clean GL state before binding FBO
    QQuickOpenGLUtils::resetOpenGLState();

    m_fbo->bind();

    const GLboolean on{1};
    f->glClearColor(0.f, 0.f, 0.f, 0.f);
    f->glColorMask(on, on, on, on);
    f->glClear(GL_COLOR_BUFFER_BIT);

    m_map->render();
    m_fbo->release();

    // QTBUG-62861
    f->glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);

    // Restore GL state modified by MapLibre rendering
    QQuickOpenGLUtils::resetOpenGLState();

    markDirty(QSGNode::DirtyMaterial);
}

Map *TextureNode::map() const {
    return m_map.get();
}

} // namespace QMapLibre
