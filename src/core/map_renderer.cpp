// Copyright (C) 2023 MapLibre contributors
// Copyright (C) 2020 Mapbox, Inc.

// SPDX-License-Identifier: BSD-2-Clause

#include "map_renderer_p.hpp"

#include "utils/scheduler.hpp"

#include <mbgl/gfx/backend_scope.hpp>

#include <QtCore/QThreadStorage>

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

MapRenderer::~MapRenderer() {
    MBGL_VERIFY_THREAD(tid);
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
    const mbgl::gfx::BackendScope scope(m_backend, mbgl::gfx::BackendScope::ScopeType::Implicit);

    m_renderer->render(params);

    if (m_forceScheduler) {
        getScheduler()->processEvents();
    }
}

void MapRenderer::setObserver(mbgl::RendererObserver *observer) {
    m_renderer->setObserver(observer);
}

} // namespace QMapLibre
