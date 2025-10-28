// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#include <QtGui/qopenglcontext.h>
#include "texture_node_opengl_p.hpp"

#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>
#include <QtQuick/QQuickOpenGLUtils>

namespace {
constexpr int DefaultSize = 64;
} // namespace

namespace QMapLibre {

TextureNodeOpenGL::~TextureNodeOpenGL() {
    // Clean up framebuffer
    if (m_fbo != 0) {
        if (const QOpenGLContext *glContext = QOpenGLContext::currentContext()) {
            QOpenGLFunctions *gl = glContext->functions();
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

    const QOpenGLContext *glContext = QOpenGLContext::currentContext();
    if (glContext == nullptr) {
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
    QOpenGLFunctions *gl = glContext->functions();
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
    const GLuint maplibreTextureId = m_map->getFramebufferTextureId();
    if (maplibreTextureId > 0) {
        // Wrap it directly as QSGTexture (zero-copy!)
        const QSize physicalSize = m_size * m_pixelRatio;
        QSGTexture *qtTexture = QNativeInterface::QSGOpenGLTexture::fromNative(
            maplibreTextureId, window, physicalSize, QQuickWindow::TextureHasAlphaChannel);

        if (qtTexture != nullptr) {
            setTexture(qtTexture);
            setRect(QRectF(QPointF(), m_size));
            setFiltering(QSGTexture::Linear);
            setOwnsTexture(false); // Don't delete MapLibre's texture!
            markDirty(QSGNode::DirtyMaterial);
        }
    }
}

} // namespace QMapLibre
