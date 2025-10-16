// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include <QSize>
#include <mbgl/gfx/renderable.hpp>
#include <mbgl/gl/renderer_backend.hpp>
#include <mbgl/util/size.hpp>

namespace QMapLibre {

class RenderableResource;

class OpenGLRendererBackend : public mbgl::gl::RendererBackend, public mbgl::gfx::Renderable {
    friend class RenderableResource;

public:
    explicit OpenGLRendererBackend(mbgl::gfx::ContextMode mode = mbgl::gfx::ContextMode::Unique);
    ~OpenGLRendererBackend() override;

    // mbgl::gfx::RendererBackend -------------------------------------------------
    mbgl::gfx::Renderable &getDefaultRenderable() override { return *this; }

    // mbgl::gl::RendererBackend --------------------------------------------------
    void updateAssumedState() override;
    void activate() override;
    void deactivate() override;

protected:
    mbgl::gl::ProcAddress getExtensionFunctionPointer(const char *name) override;

public:
    // Qt integration helpers -----------------------------------------------------
    void restoreFramebufferBinding();
    void updateRenderer(const mbgl::Size &newSize, uint32_t fbo);

    // Get the current framebuffer texture ID for direct texture sharing
    [[nodiscard]] unsigned int getFramebufferTextureId() const;

    // Set OpenGL render target for zero-copy rendering
    void setOpenGLRenderTarget(unsigned int textureId, const QSize &textureSize);

private:
    uint32_t m_fbo{};
    uint32_t m_colorTexture{};   // OpenGL texture ID for the framebuffer's color attachment
    uint32_t m_depthStencilRB{}; // OpenGL renderbuffer ID for depth-stencil attachment
    bool m_flipVertically{};     // Whether to flip rendering vertically (for QRhiWidget)
};

} // namespace QMapLibre
