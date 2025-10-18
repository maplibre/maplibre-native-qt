// Copyright (C) 2023 MapLibre contributors
// Copyright (C) 2019 Mapbox, Inc.

// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include "settings.hpp"

#include "rendering/renderer_backend_p.hpp" // provides RendererBackend alias

#include <mbgl/renderer/renderer.hpp>
#include <mbgl/renderer/renderer_observer.hpp>
#include <mbgl/util/util.hpp>

#include <QtCore/QObject>

#include <memory>
#include <mutex>

namespace mbgl {
class Renderer;
class UpdateParameters;
namespace vulkan {
class Texture2D;
} // namespace vulkan
} // namespace mbgl

namespace QMapLibre {

class MapRenderer : public QObject {
    Q_OBJECT

public:
    // Metal: allow passing an existing CAMetalLayer supplied by the UI.
    // Vulkan: allow passing a Qt Quick window for Vulkan surface creation.
    // OpenGL: use the `GLContextMode` to determine the context sharing mode.
    MapRenderer(qreal pixelRatio,
                Settings::GLContextMode mode,
                const QString &localFontFamily,
                void *nativeTargetPtr = nullptr);
#ifdef MLN_RENDER_BACKEND_VULKAN
    // Vulkan: allow passing Qt's Vulkan device for proper resource sharing.
    MapRenderer(qreal pixelRatio,
                Settings::GLContextMode,
                const QString &localFontFamily,
                void *windowPtr,
                void *physicalDevice,
                void *device,
                uint32_t graphicsQueueIndex);
#endif
    ~MapRenderer() override;

    void render();
    void updateRenderer(const mbgl::Size &size, qreal pixelRatio, quint32 fbo = 0);
    void setObserver(mbgl::RendererObserver *observer);

    // Thread-safe, called by the Frontend
    void updateParameters(std::shared_ptr<mbgl::UpdateParameters> parameters);

    // Backend-specific helpers
#if defined(MLN_RENDER_BACKEND_METAL) || defined(MLN_RENDER_BACKEND_VULKAN)
    [[nodiscard]] void *currentDrawableTexture() const { return m_backend.currentDrawable(); }
    void setCurrentDrawable(void *tex) { m_backend.setCurrentDrawable(tex); }
    void setExternalDrawable(void *tex) { m_backend.setExternalDrawable(tex); }
#endif
#if defined(MLN_RENDER_BACKEND_VULKAN)
    // Helper method to get the texture object for pixel data extraction
    mbgl::vulkan::Texture2D *getVulkanTexture() const { return m_backend.getOffscreenTexture(); }
#endif
#if defined(MLN_RENDER_BACKEND_OPENGL)
    [[nodiscard]] void *currentDrawableTexture() const { return nullptr; }
    void setCurrentDrawable(void * /* tex */) { /* OpenGL doesn't use drawable textures */ }

    // Helper method to get the OpenGL framebuffer texture ID for direct texture sharing
    [[nodiscard]] unsigned int getFramebufferTextureId() const { return m_backend.getFramebufferTextureId(); }
#endif

signals:
    void needsRendering();

private:
    MBGL_STORE_THREAD(tid)

    Q_DISABLE_COPY(MapRenderer)

    std::mutex m_updateMutex;
    std::shared_ptr<mbgl::UpdateParameters> m_updateParameters;

    RendererBackend m_backend;
    std::unique_ptr<mbgl::Renderer> m_renderer;

    bool m_forceScheduler{};
};

} // namespace QMapLibre
