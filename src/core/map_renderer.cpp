// Copyright (C) 2023 MapLibre contributors
// Copyright (C) 2020 Mapbox, Inc.

// SPDX-License-Identifier: BSD-2-Clause

#include "map_renderer_p.hpp"

#include "utils/scheduler.hpp"

#include <mbgl/gfx/backend_scope.hpp>

#include <QtCore/QThreadStorage>

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

#if defined(MLN_RENDER_BACKEND_METAL) && defined(__APPLE__) && TARGET_OS_OSX
#include <QuartzCore/CAMetalLayer.hpp>
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

MapRenderer::MapRenderer(qreal pixelRatio, Settings::GLContextMode mode, const QString &localFontFamily)
    : m_backend(static_cast<mbgl::gfx::ContextMode>(mode)),
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

#if defined(MLN_RENDER_BACKEND_METAL) && defined(__APPLE__) && TARGET_OS_OSX
// Constructor that takes an externally provided CAMetalLayer (Metal only).
MapRenderer::MapRenderer(qreal pixelRatio,
                         Settings::GLContextMode mode,
                         const QString &localFontFamily,
                         void *metalLayerPtr)
    : m_backend(static_cast<CA::MetalLayer *>(metalLayerPtr)),
      m_renderer(std::make_unique<mbgl::Renderer>(
          m_backend,
          pixelRatio,
          localFontFamily.isEmpty() ? std::nullopt : std::optional<std::string>{localFontFamily.toStdString()})),
      m_forceScheduler(needsToForceScheduler()) {
    if (m_forceScheduler) {
        Scheduler *scheduler = getScheduler();

        if (mbgl::Scheduler::GetCurrent() == nullptr) {
            mbgl::Scheduler::SetCurrent(scheduler);
        }

        connect(scheduler, &Scheduler::needsProcessing, this, &MapRenderer::needsRendering);
    }

    Q_UNUSED(mode);
}
#endif

#if defined(MLN_RENDER_BACKEND_VULKAN) || (defined(MLN_RENDER_BACKEND_METAL) && defined(__APPLE__) && TARGET_OS_OSX)
// Constructor that takes a window/layer pointer and determines the backend based on isVulkan flag
MapRenderer::MapRenderer(
    qreal pixelRatio, Settings::GLContextMode mode, const QString &localFontFamily, void *windowPtr, bool isVulkan)
#if defined(MLN_RENDER_BACKEND_VULKAN)
    : m_backend(isVulkan ? static_cast<QWindow *>(windowPtr) : nullptr),
#elif defined(MLN_RENDER_BACKEND_METAL)
    : m_backend(isVulkan ? nullptr : static_cast<CA::MetalLayer *>(windowPtr)),
#else
    : m_backend(static_cast<mbgl::gfx::ContextMode>(mode)),
#endif
      m_renderer(std::make_unique<mbgl::Renderer>(
          m_backend,
          pixelRatio,
          localFontFamily.isEmpty() ? std::nullopt : std::optional<std::string>{localFontFamily.toStdString()})),
      m_forceScheduler(needsToForceScheduler()) {

    // Debug: Log which backend we're using
    logBackendInfo();

    if (m_forceScheduler) {
        Scheduler *scheduler = getScheduler();

        if (mbgl::Scheduler::GetCurrent() == nullptr) {
            mbgl::Scheduler::SetCurrent(scheduler);
        }

        connect(scheduler, &Scheduler::needsProcessing, this, &MapRenderer::needsRendering);
    }

    Q_UNUSED(mode);
    Q_UNUSED(isVulkan);
    Q_UNUSED(windowPtr);
}
#else
// Fallback constructor for non-Metal/Vulkan backends; just delegate to default ctor and ignore pointer.
MapRenderer::MapRenderer(qreal pixelRatio,
                         Settings::GLContextMode mode,
                         const QString &localFontFamily,
                         void * /*unusedLayerPtr*/)
    : MapRenderer(pixelRatio, mode, localFontFamily) {}

// 5-parameter constructor for OpenGL compatibility - ignores both pointer and bool
MapRenderer::MapRenderer(qreal pixelRatio,
                         Settings::GLContextMode mode,
                         const QString &localFontFamily,
                         void * /*unusedPtr*/,
                         bool /*isVulkan*/)
    : MapRenderer(pixelRatio, mode, localFontFamily) {}
#endif

MapRenderer::~MapRenderer() {
    // MapRenderer may be destroyed from the GUI thread after the render thread is
    // already shut down, so the thread identity might differ from creation
    // time. Skip the thread guard here to avoid false assertion failures.
}

void MapRenderer::updateParameters(std::shared_ptr<mbgl::UpdateParameters> parameters) {
    const std::lock_guard<std::mutex> lock(m_updateMutex);
    m_updateParameters = std::move(parameters);
}

void MapRenderer::updateFramebuffer(quint32 fbo, const mbgl::Size &size) {
    MBGL_VERIFY_THREAD(tid);

    m_backend.updateFramebuffer(fbo, size);
}

void MapRenderer::render() {
    try {
        MBGL_VERIFY_THREAD(tid);
    } catch (const std::exception &e) {
        qWarning() << "Thread verification failed:" << e.what();
        return;
    } catch (...) {
        qWarning() << "Unknown exception in thread verification";
        return;
    }

    std::shared_ptr<mbgl::UpdateParameters> params;
    try {
        // Lock on the parameters
        const std::lock_guard<std::mutex> lock(m_updateMutex);

        // UpdateParameters should always be available when rendering.
        assert(m_updateParameters);

        // Hold on to the update parameters during render
        params = m_updateParameters;
    } catch (const std::exception &e) {
        qWarning() << "Exception getting update parameters:" << e.what();
        return;
    } catch (...) {
        qWarning() << "Unknown exception getting update parameters";
        return;
    }

    // The OpenGL implementation automatically enables the OpenGL context for us.
    // For Vulkan, we need to ensure the backend is properly initialized
    try {
        const mbgl::gfx::BackendScope scope(m_backend, mbgl::gfx::BackendScope::ScopeType::Implicit);

        // Add safety checks before calling render
        if (!m_renderer) {
            qWarning() << "MapRenderer::render() - m_renderer is null!";
            return;
        }

        if (!params) {
            qWarning() << "MapRenderer::render() - params is null!";
            return;
        }

        try {
            m_renderer->render(params);
        } catch (const std::exception &e) {
            qWarning() << "Exception in m_renderer->render():" << e.what();
        } catch (...) {
            qWarning() << "Unknown exception in m_renderer->render()";
        }
    } catch (const std::exception &e) {
        qWarning() << "Exception creating backend scope:" << e.what();
        return;
    } catch (...) {
        qWarning() << "Unknown exception creating backend scope";
        return;
    }

    if (m_forceScheduler) {
        getScheduler()->processEvents();
    }
}

void MapRenderer::setObserver(mbgl::RendererObserver *observer) {
    m_renderer->setObserver(observer);
}

/*! \endcond */

} // namespace QMapLibre
