// Copyright (C) 2023 MapLibre contributors
// Copyright (C) 2017 The Qt Company Ltd.
// Copyright (C) 2017 Mapbox, Inc.

// SPDX-License-Identifier: LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include <QtOpenGL/QOpenGLFramebufferObject>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGRenderNode>
#include <QtQuick/QSGSimpleTextureNode>
#include <QtQuick/QSGTextureProvider>

#include <QMapLibre/Map>

namespace QMapLibre {

class QGeoMapMapLibre;

class RhiTextureNode; // forward

class TextureNode : public QSGSimpleTextureNode {
public:
    TextureNode(const Settings &setting, const QSize &size, qreal pixelRatio, QGeoMapMapLibre *geoMap);

    [[nodiscard]] Map *map() const;

    void resize(const QSize &size, qreal pixelRatio, QQuickWindow *window);
    void render(QQuickWindow *);

private:
    std::unique_ptr<Map> m_map{};
    std::unique_ptr<QOpenGLFramebufferObject> m_fbo{};

    RhiTextureNode *m_rhiNode{nullptr};
};

} // namespace QMapLibre
