// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_OSX || TARGET_OS_IPHONE

#include <QtCore/QtGlobal>

#import <QuartzCore/CAMetalLayer.hpp>

#include <mln/gfx/context.hpp>
#include <mln/gfx/renderable.hpp>
#include <mln/mtl/renderer_backend.hpp>

namespace QMapLibre {

class MetalRendererBackend final : public mln::mtl::RendererBackend, public mln::gfx::Renderable {
public:
    explicit MetalRendererBackend(CA::MetalLayer *layer);
    MetalRendererBackend(const MetalRendererBackend &) = delete;
    MetalRendererBackend &operator=(const MetalRendererBackend &) = delete;
    ~MetalRendererBackend() override;

    // mln::gfx::RendererBackend ------------------------------------------------
    mln::gfx::Renderable &getDefaultRenderable() override { return static_cast<mln::gfx::Renderable &>(*this); }
    void activate() override {}
    void deactivate() override {}

    // Qt-specific --------------------------------------------------------------
    void setSize(mln::Size size_);
    [[nodiscard]] mln::Size getSize() const;

    // Returns the color texture of the drawable rendered in the last frame.
    [[nodiscard]] void *currentDrawable() const { return m_currentDrawable; }
    void setCurrentDrawable(void *tex) { m_currentDrawable = tex; }

    // Set an external Metal texture to render to (for QRhiWidget integration)
    [[nodiscard]] void *externalDrawable() const { return m_externalDrawable; }
    void setExternalDrawable(void *tex, const mln::Size & /* size */) { m_externalDrawable = tex; }

    void updateRenderer(const mln::Size &size, uint32_t /* fbo */) { setSize(size); };

private:
    void *m_currentDrawable{nullptr};  // id<MTLTexture>
    void *m_externalDrawable{nullptr}; // external Metal texture from QRhiWidget

    friend class QtMetalRenderableResource;
};

} // namespace QMapLibre

#endif // TARGET_OS_OSX || TARGET_OS_IPHONE
#endif // __APPLE__
