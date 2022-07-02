// Copyright (C) 2022 MapLibre contributors
// Copyright (C) 2017 The Qt Company Ltd.
// Copyright (C) 2017 Mapbox, Inc.

// SPDX-License-Identifier: LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGMAPLIBREGLNODE_H
#define QSGMAPLIBREGLNODE_H

#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGRenderNode>
#include <QtQuick/QSGSimpleTextureNode>
#include <QtQuick/private/qsgtexture_p.h>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QtOpenGL/QOpenGLFramebufferObject>
#else
#include <QtGui/QOpenGLFramebufferObject>
#endif

#include <QMapLibreGL/Map>

class QGeoMapMapLibreGL;

class QSGMapLibreGLTextureNode : public QSGSimpleTextureNode
{
public:
    QSGMapLibreGLTextureNode(const QMapLibreGL::Settings &, const QSize &, qreal pixelRatio, QGeoMapMapLibreGL *geoMap);

    QMapLibreGL::Map* map() const;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    void resize(const QSize &size, qreal pixelRatio, QQuickWindow *window);
#else
    void resize(const QSize &size, qreal pixelRatio);
#endif
    void render(QQuickWindow *);

private:
    std::unique_ptr<QMapLibreGL::Map> m_map{};
    std::unique_ptr<QOpenGLFramebufferObject> m_fbo{};
};

class QSGMapLibreGLRenderNode : public QSGRenderNode
{
public:
    QSGMapLibreGLRenderNode(const QMapLibreGL::Settings &, const QSize &, qreal pixelRatio, QGeoMapMapLibreGL *geoMap);

    QMapLibreGL::Map* map() const;

    // QSGRenderNode
    void render(const RenderState *state) override;
    StateFlags changedStates() const override;

private:
    std::unique_ptr<QMapLibreGL::Map> m_map{};
};

#endif // QSGMAPLIBREGLNODE_H
