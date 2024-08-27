// Copyright (C) 2023 MapLibre contributors
// Copyright (C) 2017 The Qt Company Ltd.
// Copyright (C) 2017 Mapbox, Inc.

// SPDX-License-Identifier: LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "texture_node.hpp"
#include "qgeomap.hpp"

#if __has_include(<QtQuick/private/qsgplaintexture_p.h>)
#include <QtQuick/private/qsgplaintexture_p.h>
#endif

#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QtQuick/QQuickOpenGLUtils>
#include <QtQuick/QSGRendererInterface>
#endif

namespace QMapLibre {

static const QSize minTextureSize = QSize(64, 64);

TextureNode::TextureNode(const Settings &settings, const QSize &size, qreal pixelRatio, QGeoMapMapLibre *geoMap) {
    setTextureCoordinatesTransform(QSGSimpleTextureNode::MirrorVertically);
    setFiltering(QSGTexture::Linear);

    m_map = std::make_unique<Map>(nullptr, settings, size.expandedTo(minTextureSize), pixelRatio);

    QObject::connect(m_map.get(), &Map::needsRendering, geoMap, &QGeoMap::sgNodeChanged);
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void TextureNode::resize(const QSize &size, qreal pixelRatio, QQuickWindow *window)
#else
void TextureNode::resize(const QSize &size, qreal pixelRatio)
#endif
{
    const QSize &minSize = size.expandedTo(minTextureSize);
    const QSize fbSize = minSize * pixelRatio;

    m_map->resize(minSize);

    m_fbo = std::make_unique<QOpenGLFramebufferObject>(fbSize, QOpenGLFramebufferObject::CombinedDepthStencil);
    m_map->setOpenGLFramebufferObject(m_fbo->handle(), fbSize);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    setTexture(QNativeInterface::QSGOpenGLTexture::fromNative(
        m_fbo->texture(), window, fbSize, QQuickWindow::TextureHasAlphaChannel));
    setOwnsTexture(true);
#else
    auto *fboTexture = static_cast<QSGPlainTexture *>(texture());
    if (fboTexture == nullptr) {
        fboTexture = new QSGPlainTexture;
        fboTexture->setHasAlphaChannel(true);
    }

    fboTexture->setTextureId(m_fbo->texture());
    fboTexture->setTextureSize(fbSize);

    if (texture() == nullptr) {
        setTexture(fboTexture);
        setOwnsTexture(true);
    }
#endif

    setRect(QRectF(QPointF(), minSize));
    markDirty(QSGNode::DirtyGeometry);
}

void TextureNode::render(QQuickWindow *window) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QOpenGLFunctions *f = static_cast<QOpenGLContext *>(window->rendererInterface()->getResource(
                                                            window, QSGRendererInterface::OpenGLContextResource))
                              ->functions();
#else
    QOpenGLFunctions *f = window->openglContext()->functions();
#endif

    f->glViewport(0, 0, m_fbo->width(), m_fbo->height());

    GLint alignment{};
    f->glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QQuickOpenGLUtils::resetOpenGLState();
#endif

    m_fbo->bind();

    const GLboolean on{1};
    f->glClearColor(0.f, 0.f, 0.f, 0.f);
    f->glColorMask(on, on, on, on);
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

Map *TextureNode::map() const {
    return m_map.get();
}

} // namespace QMapLibre
