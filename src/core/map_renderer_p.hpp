// Copyright (C) 2023 MapLibre contributors
// Copyright (C) 2019 Mapbox, Inc.

// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include "renderer_backend_p.hpp"
#include "settings.hpp"

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

class RendererBackend;

class MapRenderer : public QObject {
    Q_OBJECT

public:
    MapRenderer(qreal pixelRatio, Settings::GLContextMode, const QString &localFontFamily);
    virtual ~MapRenderer();

    void render();
    void updateFramebuffer(quint32 fbo, const mbgl::Size &size);
    void setObserver(std::shared_ptr<mbgl::RendererObserver>);

    // Thread-safe, called by the Frontend
    void updateParameters(std::shared_ptr<mbgl::UpdateParameters>);

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
