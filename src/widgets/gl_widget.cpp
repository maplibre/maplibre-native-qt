// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#include "gl_widget.hpp"
#include "gl_widget_p.hpp"

#include <QtGui/QMouseEvent>
#include <QtGui/QWheelEvent>

namespace QMapLibre {

GLWidget::GLWidget(const Settings &settings)
    : d_ptr(std::make_unique<GLWidgetPrivate>(this, settings)) {}

GLWidget::~GLWidget() {
    // Make sure we have a valid context so we
    // can delete the Map.
    makeCurrent();
    d_ptr.reset();
}

Map *GLWidget::map() {
    return d_ptr->m_map.get();
}

void GLWidget::mousePressEvent(QMouseEvent *event) {
    d_ptr->handleMousePressEvent(event);
}

void GLWidget::mouseMoveEvent(QMouseEvent *event) {
    d_ptr->handleMouseMoveEvent(event);
}

void GLWidget::wheelEvent(QWheelEvent *event) {
    d_ptr->handleWheelEvent(event);
}

void GLWidget::initializeGL() {
    d_ptr->m_map = std::make_unique<Map>(nullptr, d_ptr->m_settings, size(), devicePixelRatioF());
    connect(d_ptr->m_map.get(), SIGNAL(needsRendering()), this, SLOT(update()));

    // Set default location
    d_ptr->m_map->setCoordinateZoom(d_ptr->m_settings.defaultCoordinate(), d_ptr->m_settings.defaultZoom());

    // Set default style
    if (!d_ptr->m_settings.styles().empty()) {
        d_ptr->m_map->setStyleUrl(d_ptr->m_settings.styles().front().url);
    } else if (!d_ptr->m_settings.providerStyles().empty()) {
        d_ptr->m_map->setStyleUrl(d_ptr->m_settings.providerStyles().front().url);
    }
}

void GLWidget::paintGL() {
    d_ptr->m_map->resize(size());
    d_ptr->m_map->setFramebufferObject(defaultFramebufferObject(), size() * devicePixelRatioF());
    d_ptr->m_map->render();
}

// GLWidgetPrivate

GLWidgetPrivate::GLWidgetPrivate(QObject *parent, Settings settings)
    : QObject(parent),
      m_settings(std::move(settings)) {}

GLWidgetPrivate::~GLWidgetPrivate() = default;

void GLWidgetPrivate::handleMousePressEvent(QMouseEvent *event) {
    constexpr double zoomInScale{2.0};
    constexpr double zoomOutScale{0.5};

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    m_lastPos = event->position();
#else
    m_lastPos = event->localPos();
#endif

    if (event->type() == QEvent::MouseButtonDblClick) {
        if (event->buttons() == Qt::LeftButton) {
            m_map->scaleBy(zoomInScale, m_lastPos);
        } else if (event->buttons() == Qt::RightButton) {
            m_map->scaleBy(zoomOutScale, m_lastPos);
        }
    }

    event->accept();
}

void GLWidgetPrivate::handleMouseMoveEvent(QMouseEvent *event) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    const QPointF &position = event->position();
#else
    const QPointF &position = event->localPos();
#endif

    const QPointF delta = position - m_lastPos;
    if (!delta.isNull()) {
        if (event->buttons() == Qt::LeftButton && (event->modifiers() & Qt::ShiftModifier) != 0) {
            m_map->pitchBy(delta.y());
        } else if (event->buttons() == Qt::LeftButton) {
            m_map->moveBy(delta);
        } else if (event->buttons() == Qt::RightButton) {
            m_map->rotateBy(m_lastPos, position);
        }
    }

    m_lastPos = position;
    event->accept();
}

void GLWidgetPrivate::handleWheelEvent(QWheelEvent *event) const {
    if (event->angleDelta().y() == 0) {
        return;
    }

    constexpr float wheelConstant = 1200.f;

    float factor = static_cast<float>(event->angleDelta().y()) / wheelConstant;
    if (event->angleDelta().y() < 0) {
        factor = factor > -1 ? factor : 1 / factor;
    }

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    m_map->scaleBy(1 + factor, event->position());
#else
    m_map->scaleBy(1 + factor, event->pos());
#endif
    event->accept();
}

} // namespace QMapLibre
