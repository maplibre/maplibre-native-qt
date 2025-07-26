// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#include "gl_widget.hpp"
#include "gl_widget_p.hpp"

#include <QtCore/QDebug>
#include <QtGui/QMouseEvent>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLExtraFunctions>
#include <QtGui/QSurfaceFormat>
#include <QtGui/QWheelEvent>
#include <QtOpenGL/QOpenGLShaderProgram>
#include <cstdio>

namespace QMapLibre {

/*!
    \defgroup QMapLibreWidgets QMapLibre Widgets
    \brief Qt Widgets for MapLibre.
*/

/*!
    \class GLWidget
    \brief A simple OpenGL widget that displays a \ref QMapLibre::Map.
    \ingroup QMapLibreWidgets

    \headerfile gl_widget.hpp <QMapLibreWidgets/GLWidget>

    The widget is intended as a standalone map viewer in a Qt Widgets application.
    It owns its own instance of \ref QMapLibre::Map that is rendered to the widget.

    \fn GLWidget::onMouseDoubleClickEvent
    \brief Emitted when the user double-clicks the mouse.

    \fn GLWidget::onMouseMoveEvent
    \brief Emitted when the user moves the mouse.

    \fn GLWidget::onMousePressEvent
    \brief Emitted when the user presses the mouse.

    \fn GLWidget::onMouseReleaseEvent
    \brief Emitted when the user releases the mouse.
*/

/*! Default constructor */
GLWidget::GLWidget(const Settings &settings)
    : d_ptr(std::make_unique<GLWidgetPrivate>(this, settings)) {
    // Ensure we get an OpenGL 3.2+ core profile to use MapLibre's modern
    // renderer. This does nothing on platforms where such a context cannot be
    // provided (e.g. WebAssembly), where Qt will fall back transparently.
#ifdef __EMSCRIPTEN__
    // WebAssembly: request a WebGL2 / OpenGL ES 3.0 context. Core profile is
    // implicit and should *not* be asked for explicitly (it would fall back to
    // WebGL 1). Qt will map this to the appropriate Emscripten attributes.
    QSurfaceFormat fmt;
    fmt.setRenderableType(QSurfaceFormat::OpenGLES);
    fmt.setVersion(3, 0);
#else
    // Desktop & mobile: request a 3.2 core profile context.
    QSurfaceFormat fmt;
    fmt.setRenderableType(QSurfaceFormat::OpenGL);
    fmt.setVersion(3, 2);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
#endif
    fmt.setDepthBufferSize(24);
    fmt.setStencilBufferSize(8);
    setFormat(fmt);
}

GLWidget::~GLWidget() {
    // Make sure we have a valid context so we
    // can delete the Map and OpenGL resources.
    makeCurrent();

    if (d_ptr->m_vao) {
        auto *f = QOpenGLContext::currentContext()->extraFunctions();
        f->glDeleteVertexArrays(1, &d_ptr->m_vao);
    }
    if (d_ptr->m_vertexBuffer) {
        auto *f = QOpenGLContext::currentContext()->extraFunctions();
        f->glDeleteBuffers(1, &d_ptr->m_vertexBuffer);
    }
    if (d_ptr->m_mapFramebuffer) {
        auto *f = QOpenGLContext::currentContext()->extraFunctions();
        f->glDeleteFramebuffers(1, &d_ptr->m_mapFramebuffer);
    }

    d_ptr.reset();
}

/*!
    \brief Get the QMapLibre::Map instance.
*/
Map *GLWidget::map() {
    return d_ptr->m_map.get();
}

/*!
    \brief Mouse press event handler.
*/
void GLWidget::mousePressEvent(QMouseEvent *event) {
    const QPointF &position = event->position();
    emit onMousePressEvent(d_ptr->m_map->coordinateForPixel(position));
    if (event->type() == QEvent::MouseButtonDblClick) {
        emit onMouseDoubleClickEvent(d_ptr->m_map->coordinateForPixel(position));
    }

    d_ptr->handleMousePressEvent(event);
}

/*!
    \brief Mouse release event handler.
*/
void GLWidget::mouseReleaseEvent(QMouseEvent *event) {
    const QPointF &position = event->position();
    emit onMouseReleaseEvent(d_ptr->m_map->coordinateForPixel(position));
}

/*!
    \brief Mouse move event handler.
*/
void GLWidget::mouseMoveEvent(QMouseEvent *event) {
    const QPointF &position = event->position();
    emit onMouseMoveEvent(d_ptr->m_map->coordinateForPixel(position));

    d_ptr->handleMouseMoveEvent(event);
}

/*!
    \brief Mouse wheel event handler.
*/
void GLWidget::wheelEvent(QWheelEvent *event) {
    d_ptr->handleWheelEvent(event);
}

/*!
    \brief Initializes the map and sets up default settings.

    This function is called internally by Qt when the widget is initialized.
*/
void GLWidget::initializeGL() {
    d_ptr->m_map = std::make_unique<Map>(nullptr, d_ptr->m_settings, size(), devicePixelRatioF());
    connect(d_ptr->m_map.get(), SIGNAL(needsRendering()), this, SLOT(update()));

    // Create the OpenGL renderer
    d_ptr->m_map->createRenderer();

    // Set default location
    d_ptr->m_map->setCoordinateZoom(d_ptr->m_settings.defaultCoordinate(), d_ptr->m_settings.defaultZoom());

    // Set default style
    if (!d_ptr->m_settings.styles().empty()) {
        d_ptr->m_map->setStyleUrl(d_ptr->m_settings.styles().front().url);
    } else if (!d_ptr->m_settings.providerStyles().empty()) {
        d_ptr->m_map->setStyleUrl(d_ptr->m_settings.providerStyles().front().url);
    }

    // Ensure the map is ready to render
    d_ptr->m_map->setConnectionEstablished();

    // Initialize shader program for zero-copy texture rendering
    auto *f = QOpenGLContext::currentContext()->extraFunctions();

    // Create shader program
    d_ptr->m_shaderProgram = std::make_unique<QOpenGLShaderProgram>();
    const char *vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec2 aPos;
        layout (location = 1) in vec2 aTexCoord;
        out vec2 TexCoord;
        void main() {
            gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
            TexCoord = aTexCoord;
        }
    )";
    const char *fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;
        in vec2 TexCoord;
        uniform sampler2D mapTexture;
        void main() {
            FragColor = texture(mapTexture, TexCoord);
        }
    )";

    d_ptr->m_shaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    d_ptr->m_shaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    if (!d_ptr->m_shaderProgram->link()) {
        qWarning() << "Failed to link shader program:" << d_ptr->m_shaderProgram->log();
    }

    // Create fullscreen quad with flipped texture coordinates (Y-axis flipped for FBO textures)
    float vertices[] = {
        // positions    // texture coords (Y flipped)
        -1.0f,
        1.0f,
        0.0f,
        1.0f, // top left
        -1.0f,
        -1.0f,
        0.0f,
        0.0f, // bottom left
        1.0f,
        -1.0f,
        1.0f,
        0.0f, // bottom right
        1.0f,
        1.0f,
        1.0f,
        1.0f // top right
    };
    unsigned int indices[] = {
        0,
        1,
        2, // first triangle
        0,
        2,
        3 // second triangle
    };

    f->glGenVertexArrays(1, &d_ptr->m_vao);
    f->glGenBuffers(1, &d_ptr->m_vertexBuffer);
    unsigned int ebo;
    f->glGenBuffers(1, &ebo);

    f->glBindVertexArray(d_ptr->m_vao);

    f->glBindBuffer(GL_ARRAY_BUFFER, d_ptr->m_vertexBuffer);
    f->glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    f->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    f->glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // position attribute
    f->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    f->glEnableVertexAttribArray(0);
    // texture coord attribute
    f->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    f->glEnableVertexAttribArray(1);

    f->glBindVertexArray(0);

    // Create framebuffer for MapLibre to render into
    f->glGenFramebuffers(1, &d_ptr->m_mapFramebuffer);
}

/*!
    \brief Renders the map.

    This function is called internally by Qt when the widget needs to be redrawn.
*/
void GLWidget::paintGL() {
    const qreal dpr = devicePixelRatioF();
    const QSize mapSize(static_cast<int>(width() * dpr), static_cast<int>(height() * dpr));

    // Resize map if needed
    d_ptr->m_map->resize(mapSize);

    // Direct rendering - using framebuffer 0 which works
    d_ptr->m_map->updateFramebuffer(0, mapSize);
    d_ptr->m_map->render();
}

/*! \cond PRIVATE */

// GLWidgetPrivate
GLWidgetPrivate::GLWidgetPrivate(QObject *parent, Settings settings)
    : QObject(parent),
      m_settings(std::move(settings)) {}

GLWidgetPrivate::~GLWidgetPrivate() = default;

void GLWidgetPrivate::handleMousePressEvent(QMouseEvent *event) {
    constexpr double zoomInScale{2.0};
    constexpr double zoomOutScale{0.5};

    m_lastPos = event->position();

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
    const QPointF &position = event->position();

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

    m_map->scaleBy(1 + factor, event->position());
    event->accept();
}

/*! \endcond */

} // namespace QMapLibre
