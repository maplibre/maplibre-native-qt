// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#include "opengl_renderer_backend_p.hpp"

#include <mbgl/gfx/backend_scope.hpp>
#include <mbgl/gl/renderable_resource.hpp>

#include <QtGlobal>

#include <QtCore/QDebug>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>

namespace {
constexpr GLuint StencilMask{0xFF};
} // namespace

namespace QMapLibre {

class RenderableResource final : public mbgl::gl::RenderableResource {
public:
    explicit RenderableResource(OpenGLRendererBackend& backend_)
        : backend(backend_) {}

    void bind() override {
        assert(mbgl::gfx::BackendScope::exists());
        backend.restoreFramebufferBinding();
        backend.setViewport(0, 0, backend.getSize());

        // Ensure the framebuffer is properly cleared before rendering
        QOpenGLContext* context = QOpenGLContext::currentContext();
        if (context != nullptr) {
            QOpenGLFunctions* gl = context->functions();

            // Clear to opaque black background
            gl->glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

            // Enable stencil test for tile clipping
            gl->glEnable(GL_STENCIL_TEST);
            gl->glStencilFunc(GL_EQUAL, 0x00, 0x00); // Initially pass all - will be set by layers
            gl->glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
            gl->glStencilMask(StencilMask); // Enable writing to stencil buffer

            // Enable depth testing for proper layering
            gl->glEnable(GL_DEPTH_TEST);
            gl->glDepthFunc(GL_LEQUAL);
            gl->glDepthMask(GL_TRUE);

            // Set up blending for proper rendering
            gl->glEnable(GL_BLEND);
            // Always use premultiplied alpha for consistency with MapLibre's rendering
            gl->glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

            // Disable scissor test for full rendering
            gl->glDisable(GL_SCISSOR_TEST);
        }
    }

private:
    OpenGLRendererBackend& backend;
};

OpenGLRendererBackend::OpenGLRendererBackend(const mbgl::gfx::ContextMode mode)
    : mbgl::gl::RendererBackend(mode),
      mbgl::gfx::Renderable({0, 0}, std::make_unique<RenderableResource>(*this)) {}

OpenGLRendererBackend::~OpenGLRendererBackend() {
    QOpenGLContext* glContext = QOpenGLContext::currentContext();
    if (glContext != nullptr) {
        QOpenGLFunctions* gl = glContext->functions();

        // Clean up the color texture if we created one
        if (m_colorTexture != 0) {
            gl->glDeleteTextures(1, &m_colorTexture);
            m_colorTexture = 0;
        }

        // Clean up the depth-stencil renderbuffer if we created one
        if (m_depthStencilRB != 0) {
            gl->glDeleteRenderbuffers(1, &m_depthStencilRB);
            m_depthStencilRB = 0;
        }
    }
}

void OpenGLRendererBackend::activate() {
    // Qt has already made the context current for us (QOpenGLWidget / QSG).
    // Nothing to do here.
}

void OpenGLRendererBackend::deactivate() {
    // No explicit teardown necessary. If desired we could call makeCurrent on
    // nullptr to release the context, but Qt typically handles that.
}

void OpenGLRendererBackend::updateAssumedState() {
    assumeFramebufferBinding(ImplicitFramebufferBinding);
    assumeViewport(1, 1, size); // Use 1,1 to avoid zero-size viewport issues
}

void OpenGLRendererBackend::restoreFramebufferBinding() {
    setFramebufferBinding(m_fbo);
}

void OpenGLRendererBackend::updateRenderer(const mbgl::Size& newSize, uint32_t fbo) {
    size = newSize;
    const auto width = static_cast<GLsizei>(newSize.width);
    const auto height = static_cast<GLsizei>(newSize.height);

    qDebug() << "OpenGLRendererBackend::updateRenderer - size:" << newSize.width << "x" << newSize.height
             << "fbo:" << fbo << "current m_fbo:" << m_fbo << "current m_colorTexture:" << m_colorTexture;

    // Skip texture creation for default framebuffer
    if (fbo == 0) {
        // Skip texture creation for default framebuffer
        // Do NOT reset m_fbo here - keep the custom framebuffer state
        return;
    }

    // Only update m_fbo for non-default framebuffers
    m_fbo = fbo;

    // Create or recreate the color texture for the framebuffer
    QOpenGLContext* glContext = QOpenGLContext::currentContext();
    if (glContext != nullptr && newSize.width > 0 && newSize.height > 0) {
        QOpenGLFunctions* gl = glContext->functions();

        // Delete old texture if it exists
        if (m_colorTexture != 0) {
            gl->glDeleteTextures(1, &m_colorTexture);
        }

        // Create new texture for the framebuffer's color attachment
        gl->glGenTextures(1, &m_colorTexture);
        gl->glBindTexture(GL_TEXTURE_2D, m_colorTexture);

        // Set up texture parameters for framebuffer use with alpha
        // Use GL_RGBA8 for internal format to ensure full alpha support
        gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        // Use linear filtering for smooth rendering
        // This is especially important for overzooming/underzooming and SDF text
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Create and setup framebuffer if we don't have one
        if (fbo != 0) {
            gl->glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            gl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTexture, 0);

            // Create depth-stencil renderbuffer for proper tile clipping
            if (m_depthStencilRB == 0) {
                gl->glGenRenderbuffers(1, &m_depthStencilRB);
            }
            gl->glBindRenderbuffer(GL_RENDERBUFFER, m_depthStencilRB);
            gl->glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
            gl->glFramebufferRenderbuffer(
                GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_depthStencilRB);

            // Check framebuffer completeness
            GLenum status = gl->glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if (status != GL_FRAMEBUFFER_COMPLETE) {
                qWarning() << "OpenGLRendererBackend: Framebuffer not complete, status:" << status;
            } else {
                // Framebuffer is ready
            }

            // Clear the framebuffer to opaque black
            gl->glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

            // Set up rendering state optimized for tile rendering
            gl->glEnable(GL_BLEND);
            // Use premultiplied alpha blending to avoid edge artifacts
            gl->glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

            // Enable stencil test for tile clipping support
            gl->glEnable(GL_STENCIL_TEST);
            gl->glStencilFunc(GL_ALWAYS, 0, StencilMask);
            gl->glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
            gl->glStencilMask(StencilMask);

            // Enable depth testing for proper layering
            gl->glEnable(GL_DEPTH_TEST);
            gl->glDepthFunc(GL_LEQUAL);
            gl->glDepthMask(GL_TRUE);
        }

        // Restore default texture binding
        gl->glBindTexture(GL_TEXTURE_2D, 0);
    }
}

mbgl::gl::ProcAddress OpenGLRendererBackend::getExtensionFunctionPointer(const char* name) {
    QOpenGLContext* thisContext = QOpenGLContext::currentContext();
    return thisContext->getProcAddress(name);
}

unsigned int OpenGLRendererBackend::getFramebufferTextureId() const {
    // Return the actual color texture ID attached to the framebuffer
    // Only return texture if we're using a custom framebuffer (not the default)
    qDebug() << "OpenGLRendererBackend::getFramebufferTextureId called - m_fbo:" << m_fbo
             << "m_colorTexture:" << m_colorTexture;

    // Debug: Check if texture is still valid
    QOpenGLContext* glContext = QOpenGLContext::currentContext();
    if (glContext != nullptr && m_colorTexture != 0) {
        QOpenGLFunctions* gl = glContext->functions();
        GLboolean isTexture = gl->glIsTexture(m_colorTexture);
        qDebug() << "  Texture" << m_colorTexture << "is valid:" << (isTexture != 0 ? "YES" : "NO");
    }

    // Always return the texture if we have one, regardless of m_fbo
    return m_colorTexture;
}

} // namespace QMapLibre
