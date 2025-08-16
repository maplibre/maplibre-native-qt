// Copyright (C) 2023 MapLibre contributors
// SPDX-License-Identifier: BSD-2-Clause

#include "texture_node_opengl.hpp"
#include <QtQuick/qsgtexture_platform.h>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>
#include <QtQuick/QQuickOpenGLUtils>

namespace {
constexpr int DefaultSize = 64;
} // namespace

namespace QMapLibre {

TextureNodeOpenGL::TextureNodeOpenGL(const Settings &settings,
                                     const QSize &size,
                                     qreal pixelRatio,
                                     QGeoMapMapLibre *geoMap)
    : TextureNodeBase(settings, size, pixelRatio, geoMap) {}

TextureNodeOpenGL::~TextureNodeOpenGL() {
    // Clean up framebuffer
    if (m_fbo != 0) {
        if (QOpenGLContext *context = QOpenGLContext::currentContext()) {
            QOpenGLFunctions *gl = context->functions();
            gl->glDeleteFramebuffers(1, &m_fbo);
        }
    }
}

void TextureNodeOpenGL::resize(const QSize &size, qreal pixelRatio, QQuickWindow * /* window */) {
    m_size = size.expandedTo(QSize(DefaultSize, DefaultSize));
    m_pixelRatio = pixelRatio;
    // Pass logical size; mbgl::Map handles DPI scaling internally via pixelRatio passed at construction
    m_map->resize(m_size, m_pixelRatio);
}

void TextureNodeOpenGL::render(QQuickWindow *window) {
    if (!m_map) {
        return;
    }

    // Check for valid size
    if (m_size.isEmpty()) {
        return;
    }

    auto *context = QOpenGLContext::currentContext();
    if (context == nullptr) {
        return;
    }

    // Ensure renderer is created first
    if (!m_rendererBound) {
        m_map->createRenderer(nullptr);
        m_rendererBound = true;
    }

    // Update map size if needed - pass logical size, mbgl::Map handles DPI internally
    m_map->resize(m_size, m_pixelRatio);

    // CRITICAL: Set up framebuffer for texture sharing before rendering
    QOpenGLFunctions *gl = context->functions();
    if (m_fbo == 0) {
        gl->glGenFramebuffers(1, &m_fbo);
    }

    // Update the size
    m_map->updateRenderer(m_size, m_pixelRatio, m_fbo);

    // Save and restore OpenGL state to prevent conflicts
    GLint prevFbo{};
    gl->glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFbo);

    // Begin external commands before MapLibre render
    window->beginExternalCommands();

    m_map->render();

    // Ensure we flush the OpenGL commands
    gl->glFlush();

    // End external commands after MapLibre render
    window->endExternalCommands();

    // Restore previous framebuffer
    gl->glBindFramebuffer(GL_FRAMEBUFFER, prevFbo);

    // Try to get MapLibre's OpenGL framebuffer texture ID for zero-copy sharing
    GLuint maplibreTextureId = m_map->getFramebufferTextureId();
    if (maplibreTextureId > 0) {
        // Wrap it directly as QSGTexture (zero-copy!)
        const QSize physicalSize = m_size * m_pixelRatio;
        QSGTexture *qtTexture = QNativeInterface::QSGOpenGLTexture::fromNative(
            maplibreTextureId, window, physicalSize, QQuickWindow::TextureHasAlphaChannel);

        if (qtTexture != nullptr) {
            setTexture(qtTexture);
            setRect(QRectF(QPointF(), m_size));
            setFiltering(QSGTexture::Linear);

            // Remove Y-flip - the framebuffer is already in the correct orientation
            // QSGGeometry *geometry = this->geometry();
            // if (geometry && geometry->vertexCount() == 4) {
            //     QSGGeometry::TexturedPoint2D *vertices = geometry->vertexDataAsTexturedPoint2D();

            //     // Y-flip for OpenGL framebuffer
            //     vertices[0].ty = 1.0f - vertices[0].ty;
            //     vertices[1].ty = 1.0f - vertices[1].ty;
            //     vertices[2].ty = 1.0f - vertices[2].ty;
            //     vertices[3].ty = 1.0f - vertices[3].ty;

            //     markDirty(QSGNode::DirtyGeometry);
            // }

            setOwnsTexture(false); // Don't delete MapLibre's texture!
            markDirty(QSGNode::DirtyMaterial);
        }
    }
}

} // namespace QMapLibre
