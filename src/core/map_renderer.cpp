// Copyright (C) 2023 MapLibre contributors
// Copyright (C) 2020 Mapbox, Inc.

// SPDX-License-Identifier: BSD-2-Clause

#include "map_renderer_p.hpp"

#include "scheduler_p.hpp"

#include <mbgl/gfx/backend_scope.hpp>

#include <QtCore/QThreadStorage>

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

#if defined(MLN_RENDER_BACKEND_METAL)
#include <QuartzCore/CAMetalLayer.hpp>
#endif

#if defined(MLN_RENDER_BACKEND_VULKAN)
#include <vulkan/vulkan.h>
#include <QtGui/QWindow>
#endif

namespace {

bool needsToForceScheduler() {
    static QThreadStorage<bool> force;

    if (!force.hasLocalData()) {
        force.setLocalData(mbgl::Scheduler::GetCurrent() == nullptr);
    }

    return force.localData();
};

auto *getScheduler() {
    static QThreadStorage<std::shared_ptr<QMapLibre::Scheduler>> scheduler;

    if (!scheduler.hasLocalData()) {
        scheduler.setLocalData(std::make_shared<QMapLibre::Scheduler>());
    }

    return scheduler.localData().get();
};

} // namespace

namespace QMapLibre {

/*! \cond PRIVATE */

MapRenderer::MapRenderer(qreal pixelRatio,
                         Settings::GLContextMode mode,
                         const QString &localFontFamily,
                         void *nativeTargetPtr)
#if defined(MLN_RENDER_BACKEND_VULKAN)
    : m_backend(static_cast<QWindow *>(nativeTargetPtr)),
#elif defined(MLN_RENDER_BACKEND_METAL)
    : m_backend(static_cast<CA::MetalLayer *>(nativeTargetPtr)),
#else
    : m_backend(static_cast<mbgl::gfx::ContextMode>(mode)),
#endif
      m_renderer(std::make_unique<mbgl::Renderer>(
          m_backend,
          pixelRatio,
          localFontFamily.isEmpty() ? std::nullopt : std::optional<std::string>{localFontFamily.toStdString()})),
      m_forceScheduler(needsToForceScheduler()) {
    Q_UNUSED(mode);
    Q_UNUSED(nativeTargetPtr);
    // If we don't have a Scheduler on this thread, which
    // is usually the case for render threads, use a shared
    // dummy scheduler that needs to be explicitly forced to
    // process events.
    if (m_forceScheduler) {
        Scheduler *scheduler = getScheduler();

        if (mbgl::Scheduler::GetCurrent() == nullptr) {
            mbgl::Scheduler::SetCurrent(scheduler);
        }

        connect(scheduler, &Scheduler::needsProcessing, this, &MapRenderer::needsRendering);
    }
}

#if defined(MLN_RENDER_BACKEND_VULKAN)
// Constructor that uses Qt's Vulkan device for proper resource sharing
MapRenderer::MapRenderer(qreal pixelRatio,
                         Settings::GLContextMode /* mode */,
                         const QString &localFontFamily,
                         void *windowPtr,
                         void *physicalDevice,
                         void *device,
                         uint32_t graphicsQueueIndex)
    : m_backend(static_cast<QWindow *>(windowPtr),
                static_cast<VkPhysicalDevice>(physicalDevice),
                static_cast<VkDevice>(device),
                graphicsQueueIndex),
      m_renderer(std::make_unique<mbgl::Renderer>(
          m_backend,
          pixelRatio,
          localFontFamily.isEmpty() ? std::nullopt : std::optional<std::string>{localFontFamily.toStdString()})),
      m_forceScheduler(needsToForceScheduler()) {
    // If we don't have a Scheduler on this thread, which
    // is usually the case for render threads, use a shared
    // dummy scheduler that needs to be explicitly forced to
    // process events.
    if (m_forceScheduler) {
        Scheduler *scheduler = getScheduler();

        if (mbgl::Scheduler::GetCurrent() == nullptr) {
            mbgl::Scheduler::SetCurrent(scheduler);
        }

        connect(scheduler, &Scheduler::needsProcessing, this, &MapRenderer::needsRendering);
    }
}
#endif

MapRenderer::~MapRenderer() = default;
// MapRenderer may be destroyed from the GUI thread after the render thread is
// already shut down, so the thread identity might differ from creation
// time. Skip the thread guard here to avoid false assertion failures.

void MapRenderer::updateParameters(std::shared_ptr<mbgl::UpdateParameters> parameters) {
    const std::lock_guard<std::mutex> lock(m_updateMutex);
    m_updateParameters = std::move(parameters);
}

void MapRenderer::updateRenderer(const mbgl::Size &size, qreal pixelRatio, quint32 fbo) {
    MBGL_VERIFY_THREAD(tid);

    // Compute actual renderer size based on pixel ratio
    const mbgl::Size actualSize{static_cast<uint32_t>(size.width * pixelRatio),
                                static_cast<uint32_t>(size.height * pixelRatio)};

    m_backend.updateRenderer(actualSize, fbo);
}

void MapRenderer::render() {
    MBGL_VERIFY_THREAD(tid);

    std::shared_ptr<mbgl::UpdateParameters> params;
    {
        // Lock on the parameters
        const std::lock_guard<std::mutex> lock(m_updateMutex);

        // UpdateParameters should always be available when rendering.
        assert(m_updateParameters);

        // Hold on to the update parameters during render
        params = m_updateParameters;
    }

    // The OpenGL implementation automatically enables the OpenGL context for us.
    // For Vulkan, we need to ensure the backend is properly initialized
    const mbgl::gfx::BackendScope scope(m_backend, mbgl::gfx::BackendScope::ScopeType::Implicit);

    m_renderer->render(params);

    if (m_forceScheduler) {
        getScheduler()->processEvents();
    }
}

void MapRenderer::setObserver(mbgl::RendererObserver *observer) {
    m_renderer->setObserver(observer);
}

/*! \endcond */

} // namespace QMapLibre
