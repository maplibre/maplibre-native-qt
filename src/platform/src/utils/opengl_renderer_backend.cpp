#if defined(MLN_RENDER_BACKEND_OPENGL)

#include "opengl_renderer_backend.hpp"

#include <mbgl/gfx/backend_scope.hpp>
#include <mbgl/gl/renderable_resource.hpp>

#include <QOpenGLContext>
#include <QOpenGLFunctions>

#include <QtGlobal>

namespace QMapLibre {

class RenderableResource final : public mbgl::gl::RenderableResource {
public:
    explicit RenderableResource(OpenGLRendererBackend& backend_)
        : backend(backend_) {}

    void bind() override {
        assert(mbgl::gfx::BackendScope::exists());
        backend.restoreFramebufferBinding();
        backend.setViewport(0, 0, backend.getSize());
    }

private:
    OpenGLRendererBackend& backend;
};

OpenGLRendererBackend::OpenGLRendererBackend(const mbgl::gfx::ContextMode mode)
    : mbgl::gl::RendererBackend(mode),
      mbgl::gfx::Renderable({0, 0}, std::make_unique<RenderableResource>(*this)) {}

OpenGLRendererBackend::~OpenGLRendererBackend() {
    // Clean up the color texture if we created one
    if (m_colorTexture != 0) {
        QOpenGLContext* glContext = QOpenGLContext::currentContext();
        if (glContext) {
            QOpenGLFunctions* gl = glContext->functions();
            gl->glDeleteTextures(1, &m_colorTexture);
        }
        m_colorTexture = 0;
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
    assumeViewport(0, 0, size);
}

void OpenGLRendererBackend::restoreFramebufferBinding() {
    setFramebufferBinding(m_fbo);
}

void OpenGLRendererBackend::updateFramebuffer(uint32_t fbo, const mbgl::Size& newSize) {
    m_fbo = fbo;
    size = newSize;
    
    // Create or recreate the color texture for the framebuffer
    QOpenGLContext* glContext = QOpenGLContext::currentContext();
    if (glContext && newSize.width > 0 && newSize.height > 0) {
        QOpenGLFunctions* gl = glContext->functions();
        
        // Delete old texture if it exists
        if (m_colorTexture != 0) {
            gl->glDeleteTextures(1, &m_colorTexture);
        }
        
        // Create new texture for the framebuffer's color attachment
        gl->glGenTextures(1, &m_colorTexture);
        gl->glBindTexture(GL_TEXTURE_2D, m_colorTexture);
        
        // Set up texture parameters for framebuffer use
        gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, newSize.width, newSize.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        // Create and setup framebuffer if we don't have one
        if (fbo != 0) {
            gl->glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            gl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTexture, 0);
            
            // Check framebuffer completeness
            GLenum status = gl->glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if (status != GL_FRAMEBUFFER_COMPLETE) {
                qWarning() << "OpenGLRendererBackend: Framebuffer not complete, status:" << status;
            } else {
                qDebug() << "OpenGLRendererBackend: Framebuffer" << fbo << "setup complete with texture" << m_colorTexture << "size" << newSize.width << "x" << newSize.height;
            }
            
            // Don't clear the framebuffer - let MapLibre handle rendering
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
    return m_colorTexture;
}

} // namespace QMapLibre

#endif // MLN_RENDER_BACKEND_OPENGL
