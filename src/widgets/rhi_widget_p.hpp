// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include <QMapLibre/Map>
#include <QMapLibre/Settings>

#if defined(MLN_RENDER_BACKEND_OPENGL)
#include <QtGui/QOpenGLContext>
typedef unsigned int GLuint;
#endif

#include <memory>

QT_BEGIN_NAMESPACE
class QKeyEvent;
class QMouseEvent;
class QWheelEvent;
QT_END_NAMESPACE

namespace QMapLibre {

class RhiWidgetPrivate : public QObject {
    Q_OBJECT

public:
    explicit RhiWidgetPrivate(QObject *parent, const Settings &settings);
    ~RhiWidgetPrivate() override;

    void handleMousePressEvent(QMouseEvent *event);
    void handleMouseMoveEvent(QMouseEvent *event);
    void handleWheelEvent(QWheelEvent *event) const;
    void updateMapSize(const QSize &size, qreal dpr);

    std::unique_ptr<Map> m_map;
    Settings m_settings;
    bool m_initialized = false;
    QSize m_currentSize;
    qreal m_currentDpr = 1.0;
#if defined(MLN_RENDER_BACKEND_VULKAN)
    void* m_vulkanCommandBuffer = nullptr;  // Track the current Vulkan command buffer
#endif
#if defined(MLN_RENDER_BACKEND_METAL) && defined(__APPLE__)
    void *m_metalLayer = nullptr;  // CAMetalLayer for rendering
    bool m_metalRendererCreated = false;
    bool m_needsTextureUpdate = false;
    QImage m_renderedImage;  // Store the rendered map image
#endif

private:
    Q_DISABLE_COPY(RhiWidgetPrivate);

    QPointF m_lastPos;
};

} // namespace QMapLibre