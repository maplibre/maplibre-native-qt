// Copyright (C) 2023 MapLibre contributors
// Copyright (C) 2019 Mapbox, Inc.

// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include "settings.hpp"

#include "rendering/renderer_backend_p.hpp" // provides RendererBackend alias

#include <mbgl/renderer/renderer.hpp>
#include <mbgl/renderer/renderer_observer.hpp>
#include <mbgl/util/util.hpp>

#include <QDebug>
#include <QtCore/QObject>

#include <QtGlobal>

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
    MapRenderer(qreal pixelRatio, Settings::GLContextMode, const QString &localFontFamily);
    // Metal: allow passing an existing CAMetalLayer supplied by the UI.
    MapRenderer(qreal pixelRatio, Settings::GLContextMode, const QString &localFontFamily, void *metalLayerPtr);
    // Vulkan: allow passing a Qt Quick window for Vulkan surface creation.
    MapRenderer(
        qreal pixelRatio, Settings::GLContextMode, const QString &localFontFamily, void *windowPtr, bool isVulkan);
    // Vulkan: allow passing Qt's Vulkan device for proper resource sharing.
    MapRenderer(qreal pixelRatio,
                Settings::GLContextMode,
                const QString &localFontFamily,
                void *windowPtr,
                void *physicalDevice,
                void *device,
                uint32_t graphicsQueueIndex);
    ~MapRenderer() override;

    // Debug: Check which backend is compiled
    void logBackendInfo() const {
#if defined(MLN_RENDER_BACKEND_METAL)
        qDebug() << "MapRenderer: Compiled with METAL backend";
#elif defined(MLN_RENDER_BACKEND_VULKAN)
        qDebug() << "MapRenderer: Compiled with VULKAN backend";
#else
        qDebug() << "MapRenderer: Compiled with OpenGL backend (fallback)";
#endif
    }

    void render();
    void updateFramebuffer(quint32 fbo, const mbgl::Size &size);
    void setObserver(mbgl::RendererObserver *observer);

    // Thread-safe, called by the Frontend
    void updateParameters(std::shared_ptr<mbgl::UpdateParameters> parameters);

    // Backend-specific helpers
#if defined(MLN_RENDER_BACKEND_METAL)
    void *currentMetalTexture() const { return m_backend.currentDrawable(); }
    void *currentVulkanTexture() const { return nullptr; }
    void setCurrentDrawable(void *tex) { m_backend._q_setCurrentDrawable(tex); }
#elif defined(MLN_RENDER_BACKEND_VULKAN)
    void *currentMetalTexture() const { return nullptr; }
    void *currentVulkanTexture() const {
        qDebug() << "MapRenderer::currentVulkanTexture() called - calling backend.currentDrawable()";
        qDebug() << "MLN_RENDER_BACKEND_VULKAN is defined - using Vulkan backend";
        void *result = m_backend.currentDrawable();
        qDebug() << "MapRenderer::currentVulkanTexture() got result:" << result;
        return result;
    }
    void setCurrentDrawable(void *tex) { m_backend._q_setCurrentDrawable(tex); }

    // Helper method to get the texture object for pixel data extraction
    mbgl::vulkan::Texture2D *getVulkanTexture() const { return m_backend.getOffscreenTexture(); }
#elif defined(MLN_RENDER_BACKEND_OPENGL)
    void *currentMetalTexture() const { return nullptr; }
    void *currentVulkanTexture() const { return nullptr; }
    void setCurrentDrawable(void *) { /* OpenGL doesn't use drawable textures */ }

    // Helper method to get the OpenGL framebuffer texture ID for direct texture sharing
    unsigned int getFramebufferTextureId() const { return m_backend.getFramebufferTextureId(); }
#else
    void *currentMetalTexture() const {
        qDebug() << "WARNING: No backend defined - using fallback (Metal/Vulkan not available)";
        return nullptr;
    }
    void *currentVulkanTexture() const {
        qDebug() << "WARNING: No backend defined - using fallback (Metal/Vulkan not available)";
        return nullptr;
    }
    void setCurrentDrawable(void *) { qDebug() << "WARNING: No backend defined - setCurrentDrawable is a no-op"; }
#endif

signals:
    void needsRendering();

private:
    MBGL_STORE_THREAD(tid)

    Q_DISABLE_COPY(MapRenderer)

    std::mutex m_updateMutex;
    std::shared_ptr<mbgl::UpdateParameters> m_updateParameters;

    RendererBackend m_backend;
    std::unique_ptr<mbgl::Renderer> m_renderer{};

    bool m_forceScheduler{};
};

} // namespace QMapLibre
