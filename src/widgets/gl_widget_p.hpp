// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include <QMapLibre/Map>
#include <QMapLibre/Settings>

#include <memory>

QT_BEGIN_NAMESPACE

class QKeyEvent;
class QMouseEvent;
class QWheelEvent;
class QOpenGLShaderProgram;

QT_END_NAMESPACE

namespace QMapLibre {

class GLWidgetPrivate : public QObject {
    Q_OBJECT

public:
    explicit GLWidgetPrivate(QObject *parent, Settings settings);
    ~GLWidgetPrivate() override;

    void handleMousePressEvent(QMouseEvent *event);
    void handleMouseMoveEvent(QMouseEvent *event);
    void handleWheelEvent(QWheelEvent *event) const;

    std::unique_ptr<Map> m_map{};
    Settings m_settings;
    
    // Zero-copy texture rendering members
    std::unique_ptr<QOpenGLShaderProgram> m_shaderProgram{};
    unsigned int m_vertexBuffer{0};
    unsigned int m_vao{0};
    unsigned int m_mapFramebuffer{0};

private:
    Q_DISABLE_COPY(GLWidgetPrivate);

    QPointF m_lastPos;
};

} // namespace QMapLibre
