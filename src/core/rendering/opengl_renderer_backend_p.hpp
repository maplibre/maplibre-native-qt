// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include <mln/gfx/renderable.hpp>
#include <mln/gl/renderer_backend.hpp>
#include <mln/util/size.hpp>

namespace QMapLibre {

class QtOpenGLRenderableResource;

class OpenGLRendererBackend : public mln::gl::RendererBackend, public mln::gfx::Renderable {
    friend class QtOpenGLRenderableResource;

public:
    explicit OpenGLRendererBackend(mln::gfx::ContextMode mode = mln::gfx::ContextMode::Unique);
    ~OpenGLRendererBackend() override;

    // mln::gfx::RendererBackend -------------------------------------------------
    mln::gfx::Renderable &getDefaultRenderable() override { return *this; }

    // mln::gl::RendererBackend --------------------------------------------------
    void updateAssumedState() override;
    void activate() override;
    void deactivate() override;

protected:
    mln::gl::ProcAddress getExtensionFunctionPointer(const char *name) override;

public:
    // Qt integration helpers -----------------------------------------------------
    void restoreFramebufferBinding();
    void updateRenderer(const mln::Size &newSize, uint32_t fbo);

    // Get the current framebuffer texture ID for direct texture sharing
    [[nodiscard]] unsigned int getFramebufferTextureId() const;

    // Set OpenGL render target for zero-copy rendering
    void setExternalDrawable(unsigned int textureId, const mln::Size &textureSize);

private:
    bool m_usingExternalDrawable{};
    uint32_t m_fbo{};
    uint32_t m_colorTexture{};   // OpenGL texture ID for the framebuffer's color attachment
    uint32_t m_depthStencilRB{}; // OpenGL renderbuffer ID for depth-stencil attachment
};

} // namespace QMapLibre
