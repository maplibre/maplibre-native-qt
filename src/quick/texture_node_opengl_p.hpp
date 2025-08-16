// Copyright (C) 2023 MapLibre contributors
// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include "export_quick_p.hpp"
#include "texture_node_base_p.hpp"

#include <QtGui/qopengl.h>

namespace QMapLibre {

class Q_MAPLIBRE_QUICKPRIVATE_EXPORT TextureNodeOpenGL final : public TextureNodeBase {
public:
    using TextureNodeBase::TextureNodeBase;
    ~TextureNodeOpenGL() final;

    void resize(const QSize &size, qreal pixelRatio, QQuickWindow *window) final;
    void render(QQuickWindow *window) final;

private:
    bool m_rendererBound{};
    GLuint m_fbo{};
};

} // namespace QMapLibre
