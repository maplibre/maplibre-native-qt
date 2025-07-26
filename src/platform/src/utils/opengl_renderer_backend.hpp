#pragma once

#if defined(MLN_RENDER_BACKEND_OPENGL)

#include <mbgl/gfx/renderable.hpp>
#include <mbgl/gl/renderer_backend.hpp>
#include <mbgl/util/size.hpp>

namespace QMapLibre {

class RenderableResource;

class OpenGLRendererBackend : public mbgl::gl::RendererBackend, public mbgl::gfx::Renderable {
    friend class RenderableResource;
public:
    explicit OpenGLRendererBackend(const mbgl::gfx::ContextMode mode = mbgl::gfx::ContextMode::Unique);
    ~OpenGLRendererBackend() override;

    // mbgl::gfx::RendererBackend -------------------------------------------------
    mbgl::gfx::Renderable& getDefaultRenderable() override { return *this; }

    // mbgl::gl::RendererBackend --------------------------------------------------
    void updateAssumedState() override;
    void activate() override;
    void deactivate() override;

protected:
    mbgl::gl::ProcAddress getExtensionFunctionPointer(const char*) override;

public:
    // Qt integration helpers -----------------------------------------------------
    void restoreFramebufferBinding();
    void updateFramebuffer(uint32_t fbo, const mbgl::Size& newSize);

    // Get the current framebuffer texture ID for direct texture sharing
    unsigned int getFramebufferTextureId() const;

private:
    uint32_t m_fbo{0};
    uint32_t m_colorTexture{0}; // OpenGL texture ID for the framebuffer's color attachment
    uint32_t m_depthStencilRB{0}; // OpenGL renderbuffer ID for depth-stencil attachment
};

} // namespace QMapLibre

#endif // MLN_RENDER_BACKEND_OPENGL
