// Copyright (C) 2023 MapLibre contributors
// Copyright (C) 2019 Mapbox, Inc.

// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include "map.hpp"
#include "map_observer_p.hpp"
#include "map_renderer_p.hpp"
#include "rendering/renderer_observer_p.hpp"

#include <mbgl/actor/actor.hpp>
#include <mbgl/actor/scheduler.hpp>
#include <mbgl/map/map.hpp>
#include <mbgl/renderer/renderer_frontend.hpp>
#include <mbgl/storage/resource_transform.hpp>
#include <mbgl/util/geo.hpp>

#include <QtCore/QObject>
#include <QtCore/QSize>

#include <atomic>
#include <memory>

namespace QMapLibre {

class MapPrivate : public QObject, public mbgl::RendererFrontend {
    Q_OBJECT

public:
    explicit MapPrivate(Map *map, const Settings &settings, const QSize &size, qreal pixelRatio);
    ~MapPrivate() override;

    // mbgl::RendererFrontend implementation.
    void reset() final {}
    void setObserver(mbgl::RendererObserver &observer) final;
    void update(std::shared_ptr<mbgl::UpdateParameters> parameters) final;
    const mbgl::TaggedScheduler &getThreadPool() const final { return m_threadPool; }

    // These need to be called on the same thread.
    void createRenderer(void *nativeTargetPtr);
#ifdef MLN_RENDER_BACKEND_VULKAN
    void createRendererWithQtVulkanDevice(void *windowPtr,
                                          void *physicalDevice,
                                          void *device,
                                          uint32_t graphicsQueueIndex);
#endif
    void updateRenderer(const QSize &size, qreal pixelRatio, quint32 fbo = 0);
    void destroyRenderer();
    void render();

    using PropertySetter = std::optional<mbgl::style::conversion::Error> (mbgl::style::Layer::*)(
        const std::string &, const mbgl::style::conversion::Convertible &);
    [[nodiscard]] bool setProperty(const PropertySetter &setter,
                                   const QString &layerId,
                                   const QString &name,
                                   const QVariant &value) const;

    mbgl::EdgeInsets margins;
    std::unique_ptr<mbgl::Map> mapObj;

    // Backend-specific helpers to expose the most recent color texture
    // Safe to call from the GUI thread.
    void *currentDrawableTexture() const;

    // Backend-specific: push latest swap-chain texture into renderer
    void setCurrentDrawable(void *tex);

#if defined(MLN_RENDER_BACKEND_VULKAN)
    // Helper method to get the texture object for pixel data extraction
    mbgl::vulkan::Texture2D *getVulkanTexture() const;
#endif

#if defined(MLN_RENDER_BACKEND_OPENGL)
    // Helper method to get the OpenGL framebuffer texture ID for direct texture sharing
    unsigned int getFramebufferTextureId() const;
#endif

public slots:
    void requestRendering();

signals:
    void needsRendering();

private:
    Q_DISABLE_COPY(MapPrivate)

    mutable std::recursive_mutex m_mapRendererMutex;
    std::unique_ptr<RendererObserver> m_rendererObserver;
    std::shared_ptr<mbgl::UpdateParameters> m_updateParameters;

    std::unique_ptr<MapObserver> m_mapObserver;
    std::unique_ptr<MapRenderer> m_mapRenderer;
    std::unique_ptr<mbgl::Actor<mbgl::ResourceTransform::TransformCallback>> m_resourceTransform;

    Settings::GLContextMode m_mode;
    qreal m_pixelRatio;

    QString m_localFontFamily;

    std::atomic_flag m_renderQueued = ATOMIC_FLAG_INIT;

    mbgl::TaggedScheduler m_threadPool{mbgl::Scheduler::GetBackground(), mbgl::util::SimpleIdentity{}};
};

inline void *MapPrivate::currentDrawableTexture() const {
    std::scoped_lock lock(m_mapRendererMutex);
    return m_mapRenderer ? m_mapRenderer->currentDrawableTexture() : nullptr;
}

inline void MapPrivate::setCurrentDrawable(void *tex) {
    std::scoped_lock lock(m_mapRendererMutex);
    if (m_mapRenderer) {
        m_mapRenderer->setCurrentDrawable(tex);
    }
}

#if defined(MLN_RENDER_BACKEND_VULKAN)
inline mbgl::vulkan::Texture2D *MapPrivate::getVulkanTexture() const {
    std::scoped_lock lock(m_mapRendererMutex);
    return m_mapRenderer ? m_mapRenderer->getVulkanTexture() : nullptr;
}
#endif

#if defined(MLN_RENDER_BACKEND_OPENGL)
inline unsigned int MapPrivate::getFramebufferTextureId() const {
    std::scoped_lock lock(m_mapRendererMutex);
    return m_mapRenderer ? m_mapRenderer->getFramebufferTextureId() : 0;
}
#endif

} // namespace QMapLibre
