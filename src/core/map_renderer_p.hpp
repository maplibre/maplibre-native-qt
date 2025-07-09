// Copyright (C) 2023 MapLibre contributors
// Copyright (C) 2019 Mapbox, Inc.

// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include "settings.hpp"

#include <utils/renderer_backend.hpp> // provides RendererBackend alias

#include <mbgl/renderer/renderer.hpp>
#include <mbgl/renderer/renderer_observer.hpp>
#include <mbgl/util/util.hpp>

#include <QtCore/QObject>

#include <QtGlobal>

#include <memory>
#include <mutex>

namespace mbgl {
class Renderer;
class UpdateParameters;
} // namespace mbgl

namespace QMapLibre {

class MapRenderer : public QObject {
    Q_OBJECT

public:
    MapRenderer(qreal pixelRatio, Settings::GLContextMode, const QString &localFontFamily);
    // Metal: allow passing an existing CAMetalLayer supplied by the UI.
    MapRenderer(qreal pixelRatio, Settings::GLContextMode, const QString &localFontFamily, void *metalLayerPtr);
    ~MapRenderer() override;

    void render();
    void updateFramebuffer(quint32 fbo, const mbgl::Size &size);
    void setObserver(mbgl::RendererObserver *observer);

    // Thread-safe, called by the Frontend
    void updateParameters(std::shared_ptr<mbgl::UpdateParameters> parameters);

    // Backend-specific helpers (only meaningful for Metal).
#if defined(MLN_RENDER_BACKEND_METAL)
    void *currentMetalTexture() const { return m_backend.currentDrawable(); }
    void setCurrentDrawable(void *tex) { m_backend._q_setCurrentDrawable(tex); }
#else
    void *currentMetalTexture() const { return nullptr; }
    void setCurrentDrawable(void *) {}
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
