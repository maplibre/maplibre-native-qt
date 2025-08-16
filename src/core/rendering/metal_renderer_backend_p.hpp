// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_OSX || TARGET_OS_IPHONE

#include <QtCore/QtGlobal>

#import <QuartzCore/CAMetalLayer.hpp>

#include <mbgl/gfx/context.hpp>
#include <mbgl/gfx/renderable.hpp>
#include <mbgl/mtl/renderer_backend.hpp>

namespace QMapLibre {

class MetalRendererBackend final : public mbgl::mtl::RendererBackend, public mbgl::gfx::Renderable {
public:
    explicit MetalRendererBackend(CA::MetalLayer* layer);
    MetalRendererBackend(const MetalRendererBackend&) = delete;
    MetalRendererBackend& operator=(const MetalRendererBackend&) = delete;
    ~MetalRendererBackend() override;

    // mbgl::gfx::RendererBackend ------------------------------------------------
    mbgl::gfx::Renderable& getDefaultRenderable() override { return static_cast<mbgl::gfx::Renderable&>(*this); }
    void activate() override {}
    void deactivate() override {}
    void updateAssumedState() override {}

    // Qt-specific --------------------------------------------------------------
    void setSize(mbgl::Size size_);
    [[nodiscard]] mbgl::Size getSize() const;

    // Returns the color texture of the drawable rendered in the last frame.
    [[nodiscard]] void* currentDrawable() const { return m_currentDrawable; }
    void setCurrentDrawable(void* tex) { m_currentDrawable = tex; }

    void updateRenderer(const mbgl::Size& size, uint32_t /* fbo */) { setSize(size); };

private:
    void* m_currentDrawable{nullptr}; // id<MTLTexture>

    friend class QtMetalRenderableResource;
};

} // namespace QMapLibre

#endif // TARGET_OS_OSX || TARGET_OS_IPHONE
#endif // __APPLE__
