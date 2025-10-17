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

/*! \cond PRIVATE */
class QtOpenGLRenderableResource final : public mbgl::gl::RenderableResource {
public:
    explicit QtOpenGLRenderableResource(OpenGLRendererBackend &backend_)
        : backend(backend_) {}

    void bind() override {
        assert(mbgl::gfx::BackendScope::exists());
        backend.restoreFramebufferBinding();
        backend.setViewport(0, 0, backend.getSize());

        // Clear to prevent artifacts - scissor disable needed to ensure full clear
        QOpenGLContext *context = QOpenGLContext::currentContext();
        if (context != nullptr) {
            QOpenGLFunctions *gl = context->functions();

            // Disable scissor test to ensure full framebuffer is cleared
            gl->glDisable(GL_SCISSOR_TEST);

            // Clear the framebuffer
            gl->glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        }
    }

private:
    OpenGLRendererBackend &backend;
};
/*! \endcond PRIVATE */

/*! \cond PRIVATE */
OpenGLRendererBackend::OpenGLRendererBackend(const mbgl::gfx::ContextMode mode)
    : mbgl::gl::RendererBackend(mode),
      mbgl::gfx::Renderable({0, 0}, std::make_unique<QtOpenGLRenderableResource>(*this)) {}

OpenGLRendererBackend::~OpenGLRendererBackend() {
    QOpenGLContext *glContext = QOpenGLContext::currentContext();
    if (glContext != nullptr) {
        QOpenGLFunctions *gl = glContext->functions();

        // Don't delete textures we don't own (e.g., from QRhiWidget)
        // We only own textures if we created them ourselves
        // For now, we'll skip deleting m_colorTexture to be safe

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
    // Check if we have a valid context before trying to bind
    QOpenGLContext *glContext = QOpenGLContext::currentContext();
    if (!glContext) {
        // No context, can't bind
        return;
    }
    setFramebufferBinding(m_fbo);
}

void OpenGLRendererBackend::updateRenderer(const mbgl::Size &newSize, uint32_t fbo) {
    size = newSize;
    const auto width = static_cast<GLsizei>(newSize.width);
    const auto height = static_cast<GLsizei>(newSize.height);

    // Always update m_fbo to track the current FBO
    m_fbo = fbo;

    // If we're rendering to a non-default FBO (like QRhiWidget), we need to flip vertically
    m_flipVertically = (fbo != 0);

    // Skip texture creation for default framebuffer
    if (fbo == 0) {
        // For default framebuffer, no texture management needed
        return;
    }

    // Create or recreate the color texture for the framebuffer
    QOpenGLContext *glContext = QOpenGLContext::currentContext();
    if (glContext != nullptr && newSize.width > 0 && newSize.height > 0) {
        QOpenGLFunctions *gl = glContext->functions();

        // Check if the FBO already has a color attachment (e.g., from QRhiWidget)
        GLint existingColorAttachment = 0;
        gl->glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        gl->glGetFramebufferAttachmentParameteriv(
            GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &existingColorAttachment);

        // If there's already a color attachment, don't create our own texture
        if (existingColorAttachment != 0) {
            // Don't delete the previous texture if it's the same as the existing one
            if (m_colorTexture != 0 && m_colorTexture != static_cast<GLuint>(existingColorAttachment)) {
                gl->glDeleteTextures(1, &m_colorTexture);
            }
            m_colorTexture = existingColorAttachment;

            // Make sure depth/stencil buffer exists for the FBO
            if (m_depthStencilRB == 0) {
                gl->glGenRenderbuffers(1, &m_depthStencilRB);
            }
            gl->glBindRenderbuffer(GL_RENDERBUFFER, m_depthStencilRB);
            gl->glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
            gl->glFramebufferRenderbuffer(
                GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_depthStencilRB);

            return;
        }

        // Delete old texture if it exists and we own it
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

            // Initial clear when creating the framebuffer
            gl->glViewport(0, 0, width, height);
            gl->glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        }

        // Restore default texture binding
        gl->glBindTexture(GL_TEXTURE_2D, 0);
    }
}

mbgl::gl::ProcAddress OpenGLRendererBackend::getExtensionFunctionPointer(const char *name) {
    QOpenGLContext *thisContext = QOpenGLContext::currentContext();
    return thisContext->getProcAddress(name);
}

unsigned int OpenGLRendererBackend::getFramebufferTextureId() const {
    // Return the actual color texture ID attached to the framebuffer
    // Only return texture if we're using a custom framebuffer (not the default)

    // Always return the texture if we have one
    return m_colorTexture;
}

void OpenGLRendererBackend::setOpenGLRenderTarget(unsigned int textureId, const QSize &textureSize) {
    // Handle reset case (textureId == 0 means reset to default)
    if (textureId == 0) {
        QOpenGLContext *glContext = QOpenGLContext::currentContext();
        if (glContext && m_fbo != 0) {
            QOpenGLFunctions *gl = glContext->functions();
            gl->glDeleteFramebuffers(1, &m_fbo);
            m_fbo = 0;
            if (m_depthStencilRB != 0) {
                gl->glDeleteRenderbuffers(1, &m_depthStencilRB);
                m_depthStencilRB = 0;
            }
        }
        m_colorTexture = 0;
        assumeFramebufferBinding(0);
        return;
    }

    QOpenGLContext *glContext = QOpenGLContext::currentContext();
    if (!glContext) {
        qWarning() << "OpenGLRendererBackend::setOpenGLRenderTarget - No OpenGL context";
        return;
    }

    QOpenGLFunctions *gl = glContext->functions();

    const auto width = static_cast<GLsizei>(textureSize.width());
    const auto height = static_cast<GLsizei>(textureSize.height());

    // Skip debug output for production
    // qDebug() << "OpenGLRendererBackend::setOpenGLRenderTarget - textureId:" << textureId
    //          << "size:" << width << "x" << height;

    // Update the size
    size = mbgl::Size(textureSize.width(), textureSize.height());

    // Check if we need to recreate the FBO (texture changed or no FBO yet)
    if (m_colorTexture != textureId || m_fbo == 0) {
        // If we already have an FBO, delete it to create a fresh one
        if (m_fbo != 0) {
            gl->glDeleteFramebuffers(1, &m_fbo);
            m_fbo = 0;
        }

        // Create new framebuffer
        gl->glGenFramebuffers(1, &m_fbo);
        gl->glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

        // Attach the external texture as color attachment
        gl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, 0);

        // Update our stored texture ID (but we don't own it)
        m_colorTexture = textureId;

        // Delete old depth-stencil buffer if exists
        if (m_depthStencilRB != 0) {
            gl->glDeleteRenderbuffers(1, &m_depthStencilRB);
            m_depthStencilRB = 0;
        }

        // Create new depth-stencil renderbuffer
        gl->glGenRenderbuffers(1, &m_depthStencilRB);
        gl->glBindRenderbuffer(GL_RENDERBUFFER, m_depthStencilRB);
        gl->glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        gl->glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_depthStencilRB);

        // Check framebuffer completeness
        GLenum status = gl->glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            qWarning() << "OpenGLRendererBackend::setOpenGLRenderTarget - Framebuffer incomplete:" << status;
            // Clean up on failure
            gl->glDeleteFramebuffers(1, &m_fbo);
            m_fbo = 0;
            gl->glDeleteRenderbuffers(1, &m_depthStencilRB);
            m_depthStencilRB = 0;
            m_colorTexture = 0;
            return;
        }
    } else {
        // Just bind the existing FBO
        gl->glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    }

    // Update assumed state so MapLibre knows to use our FBO
    assumeFramebufferBinding(m_fbo);
    setFramebufferBinding(m_fbo);

    // Set the viewport
    setViewport(0, 0, size);
}
/*! \endcond PRIVATE */

} // namespace QMapLibre
