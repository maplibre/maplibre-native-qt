// Copyright (C) 2022 MapLibre contributors
// Copyright (C) 2017 The Qt Company Ltd.
// Copyright (C) 2017 Mapbox, Inc.

// SPDX-License-Identifier: LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgmaplibreglnode.h"
#include "qgeomapmaplibregl.h"

#if QT_HAS_INCLUDE(<QtQuick/private/qsgplaintexture_p.h>)
#include <QtQuick/private/qsgplaintexture_p.h>
#endif

#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QtQuick/QQuickOpenGLUtils>
#include <QtQuick/QSGRendererInterface>
#endif

// QSGMapLibreGLTextureNode

static const QSize minTextureSize = QSize(64, 64);

QSGMapLibreGLTextureNode::QSGMapLibreGLTextureNode(const QMapLibreGL::Settings &settings, const QSize &size, qreal pixelRatio, QGeoMapMapLibreGL *geoMap)
        : QSGSimpleTextureNode()
{
    setTextureCoordinatesTransform(QSGSimpleTextureNode::MirrorVertically);
    setFiltering(QSGTexture::Linear);

    m_map.reset(new QMapLibreGL::Map(nullptr, settings, size.expandedTo(minTextureSize), pixelRatio));

    QObject::connect(m_map.get(), &QMapLibreGL::Map::needsRendering, geoMap, &QGeoMap::sgNodeChanged);
    QObject::connect(m_map.get(), &QMapLibreGL::Map::copyrightsChanged, geoMap,
            static_cast<void (QGeoMap::*)(const QString &)>(&QGeoMapMapLibreGL::copyrightsChanged));
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void QSGMapLibreGLTextureNode::resize(const QSize &size, qreal pixelRatio, QQuickWindow *window)
#else
void QSGMapLibreGLTextureNode::resize(const QSize &size, qreal pixelRatio)
#endif
{
    const QSize& minSize = size.expandedTo(minTextureSize);
    const QSize fbSize = minSize * pixelRatio;

    m_map->resize(minSize);

    m_fbo.reset(new QOpenGLFramebufferObject(fbSize, QOpenGLFramebufferObject::CombinedDepthStencil));
    m_map->setFramebufferObject(m_fbo->handle(), fbSize);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // There is no elegant way to set new frame buffer object to already created texture.
    // So, a quick hack-solution is to re-create the texture as well, every time the size changes.
    setTexture(QNativeInterface::QSGOpenGLTexture::fromNative(m_fbo->texture(), window, fbSize, QQuickWindow::TextureHasAlphaChannel));
    setOwnsTexture(true);
#else
    auto *fboTexture = static_cast<QSGPlainTexture *>(texture());
    if (!fboTexture) {
        fboTexture = new QSGPlainTexture;
        fboTexture->setHasAlphaChannel(true);
    }

    fboTexture->setTextureId(m_fbo->texture());
    fboTexture->setTextureSize(fbSize);

    if (!texture()) {
        setTexture(fboTexture);
        setOwnsTexture(true);
    }
#endif

    setRect(QRectF(QPointF(), minSize));
    markDirty(QSGNode::DirtyGeometry);
}

void QSGMapLibreGLTextureNode::render(QQuickWindow *window)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QOpenGLFunctions *f = static_cast<QOpenGLContext*>(window->rendererInterface()->getResource(window, QSGRendererInterface::OpenGLContextResource))->functions();
#else
    QOpenGLFunctions *f = window->openglContext()->functions();
#endif

    f->glViewport(0, 0, m_fbo->width(), m_fbo->height());

    GLint alignment;
    f->glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QQuickOpenGLUtils::resetOpenGLState();
#endif

    m_fbo->bind();

    f->glClearColor(0.f, 0.f, 0.f, 0.f);
    f->glColorMask(true, true, true, true);
    f->glClear(GL_COLOR_BUFFER_BIT);

    m_map->render();
    m_fbo->release();

    // QTBUG-62861
    f->glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // makes sure the right depth is used
    f->glDepthRangef(0, 1);
#endif

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QQuickOpenGLUtils::resetOpenGLState();
#else
    window->resetOpenGLState();
#endif
    markDirty(QSGNode::DirtyMaterial);
}

QMapLibreGL::Map* QSGMapLibreGLTextureNode::map() const
{
    return m_map.get();
}

// QSGMapLibreGLRenderNode

QSGMapLibreGLRenderNode::QSGMapLibreGLRenderNode(const QMapLibreGL::Settings &settings, const QSize &size, qreal pixelRatio, QGeoMapMapLibreGL *geoMap)
        : QSGRenderNode()
{
    m_map.reset(new QMapLibreGL::Map(nullptr, settings, size, pixelRatio));
    QObject::connect(m_map.get(), &QMapLibreGL::Map::needsRendering, geoMap, &QGeoMap::sgNodeChanged);
    QObject::connect(m_map.get(), &QMapLibreGL::Map::copyrightsChanged, geoMap,
            static_cast<void (QGeoMap::*)(const QString &)>(&QGeoMapMapLibreGL::copyrightsChanged));
}

QMapLibreGL::Map* QSGMapLibreGLRenderNode::map() const
{
    return m_map.get();
}

void QSGMapLibreGLRenderNode::render(const RenderState *state)
{
    // QMapLibreGL::Map assumes we've prepared the viewport prior to render().
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
    f->glViewport(state->scissorRect().x(), state->scissorRect().y(), state->scissorRect().width(), state->scissorRect().height());
    f->glScissor(state->scissorRect().x(), state->scissorRect().y(), state->scissorRect().width(), state->scissorRect().height());
    f->glEnable(GL_SCISSOR_TEST);

    GLint alignment;
    f->glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);

    m_map->render();

    // QTBUG-62861
    f->glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // makes sure the right depth is used
    f->glDepthRangef(0, 1);
#endif
}

QSGRenderNode::StateFlags QSGMapLibreGLRenderNode::changedStates() const
{
    return QSGRenderNode::DepthState
         | QSGRenderNode::StencilState
         | QSGRenderNode::ScissorState
         | QSGRenderNode::ColorState
         | QSGRenderNode::BlendState
         | QSGRenderNode::ViewportState
         | QSGRenderNode::RenderTargetState;
}
