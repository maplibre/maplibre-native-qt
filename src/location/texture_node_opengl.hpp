// Copyright (C) 2024 MapLibre contributors
// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include "texture_node_base.hpp"
#include "qgeomap.hpp"
#include <qopengl.h>

namespace QMapLibre {

class TextureNodeOpenGL : public TextureNodeBase {
public:
    TextureNodeOpenGL(const Settings &settings, const QSize &size, qreal pixelRatio, QGeoMapMapLibre *geoMap);
    ~TextureNodeOpenGL() override;

    void resize(const QSize &size, qreal pixelRatio, QQuickWindow *window) override;
    void render(QQuickWindow *window) override;

private:
    bool m_rendererBound = false;
    GLuint m_fbo = 0;
};

} // namespace QMapLibre
