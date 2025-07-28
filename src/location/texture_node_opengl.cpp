// Copyright (C) 2024 MapLibre contributors
// SPDX-License-Identifier: BSD-2-Clause

#include "texture_node_opengl.hpp"
#include <QtQuick/qsgtexture_platform.h>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>
#include <QtQuick/QQuickOpenGLUtils>

namespace QMapLibre {

TextureNodeOpenGL::TextureNodeOpenGL(const Settings &settings,
                                     const QSize &size,
                                     qreal pixelRatio,
                                     QGeoMapMapLibre *geoMap)
    : TextureNodeBase(settings, size, pixelRatio, geoMap) {}

void TextureNodeOpenGL::resize(const QSize &size, qreal pixelRatio, QQuickWindow * /* window */) {
    m_size = size.expandedTo(QSize(64, 64));
    m_pixelRatio = pixelRatio;
    m_map->resize(m_size);
}

void TextureNodeOpenGL::render(QQuickWindow *window) {
    auto *context = static_cast<QOpenGLContext *>(
        window->rendererInterface()->getResource(window, QSGRendererInterface::OpenGLContextResource));

    if (!context) return;

    // Ensure renderer is created
    if (!m_rendererBound) {
        m_map->createRenderer();
        m_rendererBound = true;
    }

    QOpenGLFunctions *f = context->functions();

    // Save current GL state
    GLint viewport[4];
    f->glGetIntegerv(GL_VIEWPORT, viewport);

    // Render to texture
    m_map->render();

    // Get the native OpenGL texture from the map
    if (void *tex = m_map->nativeColorTexture()) {
        GLuint textureId = reinterpret_cast<intptr_t>(tex);

        QSGTexture *qtTex = QNativeInterface::QSGOpenGLTexture::fromNative(
            textureId,
            window,
            QSize(m_size.width() * m_pixelRatio, m_size.height() * m_pixelRatio),
            QQuickWindow::TextureHasAlphaChannel);

        setTexture(qtTex);
        setOwnsTexture(true);
        markDirty(QSGNode::DirtyMaterial);
    }

    // Restore viewport
    f->glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

    setRect(QRectF(QPointF(), m_size));
}

} // namespace QMapLibre
