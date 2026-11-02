// Copyright (C) 2023 MapLibre contributors
// Copyright (C) 2019 Mapbox, Inc.

// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include "map.hpp"
#include "map_observer_p.hpp"
#include "map_renderer_p.hpp"
#include "rendering/renderer_observer_p.hpp"

#include <mln/actor/actor.hpp>
#include <mln/actor/scheduler.hpp>
#include <mln/map/map.hpp>
#include <mln/renderer/renderer_frontend.hpp>
#include <mln/storage/resource_transform.hpp>
#include <mln/util/geo.hpp>

#include <QtCore/QObject>
#include <QtCore/QSize>

#include <atomic>
#include <memory>

namespace QMapLibre {

class MapPrivate : public QObject, public mln::RendererFrontend {
    Q_OBJECT

public:
    explicit MapPrivate(Map *map, const Settings &settings, const QSize &size, qreal pixelRatio);
    ~MapPrivate() override;

    // mln::RendererFrontend implementation.
    void reset() final {}
    void setObserver(mln::RendererObserver &observer) final;
    void update(std::shared_ptr<mln::UpdateParameters> parameters) final;
    const mln::TaggedScheduler &getThreadPool() const final { return m_threadPool; }

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

    using PropertySetter = std::optional<mln::style::conversion::Error> (mln::style::Layer::*)(
        const std::string &, const mln::style::conversion::Convertible &);
    [[nodiscard]] bool setProperty(const PropertySetter &setter,
                                   const QString &layerId,
                                   const QString &name,
                                   const QVariant &value) const;

    mln::EdgeInsets margins;
    std::unique_ptr<mln::Map> mapObj;

    // Backend-specific helpers to expose the most recent color texture
    // Safe to call from the GUI thread.
    void *currentDrawableTexture() const;

    // Backend-specific: push latest swap-chain texture into renderer
    void setCurrentDrawable(void *tex);
    void setExternalDrawable(void *tex, const QSize &size);

#if defined(MLN_RENDER_BACKEND_VULKAN)
    // Helper method to get the texture object for pixel data extraction
    mln::vulkan::Texture2D *getVulkanTexture() const;
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
    std::shared_ptr<mln::UpdateParameters> m_updateParameters;

    std::unique_ptr<MapObserver> m_mapObserver;
    std::unique_ptr<MapRenderer> m_mapRenderer;
    std::unique_ptr<mln::Actor<mln::ResourceTransform::TransformCallback>> m_resourceTransform;

    Settings::GLContextMode m_mode;
    qreal m_pixelRatio;

    QString m_localFontFamily;

    std::atomic_flag m_renderQueued = ATOMIC_FLAG_INIT;

    mln::TaggedScheduler m_threadPool{mln::Scheduler::GetBackground(), mln::util::SimpleIdentity{}};
};

} // namespace QMapLibre
