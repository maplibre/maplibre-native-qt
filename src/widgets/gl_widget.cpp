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

namespace {
constexpr int DepthBufferSize{24};
constexpr int StencilBufferSize{8};
} // namespace

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
    fmt.setDepthBufferSize(DepthBufferSize);
    fmt.setStencilBufferSize(StencilBufferSize);
    setFormat(fmt);
}

GLWidget::~GLWidget() {
    // Make sure we have a valid context so we
    // can delete the Map and OpenGL resources.
    makeCurrent();

    if (d_ptr->m_vao != 0) {
        auto *f = QOpenGLContext::currentContext()->extraFunctions();
        f->glDeleteVertexArrays(1, &d_ptr->m_vao);
    }
    if (d_ptr->m_vertexBuffer != 0) {
        auto *f = QOpenGLContext::currentContext()->extraFunctions();
        f->glDeleteBuffers(1, &d_ptr->m_vertexBuffer);
    }
    if (d_ptr->m_mapFramebuffer != 0) {
        auto *f = QOpenGLContext::currentContext()->extraFunctions();
        f->glDeleteFramebuffers(1, &d_ptr->m_mapFramebuffer);
    }
    if (d_ptr->m_mapTexture != 0) {
        auto *f = QOpenGLContext::currentContext()->extraFunctions();
        f->glDeleteTextures(1, &d_ptr->m_mapTexture);
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
    qDebug() << "GLWidget::initializeGL - size:" << size() << "DPR:" << devicePixelRatioF();

    d_ptr->m_map = std::make_unique<Map>(nullptr, d_ptr->m_settings, size(), devicePixelRatioF());
    connect(d_ptr->m_map.get(), SIGNAL(needsRendering()), this, SLOT(update()));

    // Create the OpenGL renderer
    d_ptr->m_map->createRenderer(nullptr);

    // Set default location
    d_ptr->m_map->setCoordinateZoom(d_ptr->m_settings.defaultCoordinate(), d_ptr->m_settings.defaultZoom());
    qDebug() << "Default coordinate:" << d_ptr->m_settings.defaultCoordinate()
             << "Default zoom:" << d_ptr->m_settings.defaultZoom();

    // Set default style
    if (!d_ptr->m_settings.styles().empty()) {
        d_ptr->m_map->setStyleUrl(d_ptr->m_settings.styles().front().url);
        qDebug() << "Setting style URL:" << d_ptr->m_settings.styles().front().url;
    } else if (!d_ptr->m_settings.providerStyles().empty()) {
        d_ptr->m_map->setStyleUrl(d_ptr->m_settings.providerStyles().front().url);
        qDebug() << "Setting provider style URL:" << d_ptr->m_settings.providerStyles().front().url;
    }

    // Ensure the map is ready to render
    d_ptr->m_map->setConnectionEstablished();

    // Add observers for debugging
    connect(d_ptr->m_map.get(), &Map::mapChanged, this, [this](Map::MapChange change) {
        if (change == Map::MapChangeDidFinishLoadingStyle) {
            qDebug() << "MapChange: Style finished loading";
            // Debug: Check min/max zoom after style loads
            qDebug() << "Map zoom bounds - Min:" << d_ptr->m_map->minimumZoom() << "Max:" << d_ptr->m_map->maximumZoom()
                     << "Current:" << d_ptr->m_map->zoom();
        } else if (change == Map::MapChangeDidFailLoadingMap) {
            qDebug() << "MapChange: Map failed to load";
        } else if (change == Map::MapChangeDidFinishLoadingMap) {
            qDebug() << "MapChange: Map finished loading";
        } else if (change == Map::MapChangeDidFinishRenderingFrameFullyRendered) {
            static int frameCount = 0;
            if (frameCount++ == 0) {
                qDebug() << "MapChange: First frame fully rendered";
            }
        } else if (change == Map::MapChangeSourceDidChange) {
            qDebug() << "MapChange: Source changed";
        }
    });

    connect(
        d_ptr->m_map.get(), &Map::mapLoadingFailed, this, [](Map::MapLoadingFailure type, const QString &description) {
            qWarning() << "Map loading failed:" << static_cast<int>(type) << description;
        });

    // Force a render to load initial tiles
    d_ptr->m_map->render();

    // Debug: Initial load state
    qDebug() << "Initial map state - fully loaded:" << d_ptr->m_map->isFullyLoaded();

    // Initialize shader program for zero-copy texture rendering
    auto *f = QOpenGLContext::currentContext()->extraFunctions();

    // Create shader program
    d_ptr->m_shaderProgram = std::make_unique<QOpenGLShaderProgram>();
    const char *vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec2 aPos;
        layout (location = 1) in vec2 aTexCoord;
        out vec2 TexCoord;
        uniform vec2 viewportSize;
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
            // Flip Y coordinate to correct upside-down rendering
            vec2 flippedTexCoord = vec2(TexCoord.x, 1.0 - TexCoord.y);
            vec4 texColor = texture(mapTexture, flippedTexCoord);

            // Pass through the color as-is
            // MapLibre already handles all the rendering correctly including premultiplied alpha
            FragColor = texColor;
        }
    )";

    d_ptr->m_shaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    d_ptr->m_shaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    if (!d_ptr->m_shaderProgram->link()) {
        qWarning() << "Failed to link shader program:" << d_ptr->m_shaderProgram->log();
    }

    // Create fullscreen quad
    float vertices[] = {
        // positions    // texture coords
        -1.0f,
        1.0f,
        0.0f,
        0.0f, // top left
        -1.0f,
        -1.0f,
        0.0f,
        1.0f, // bottom left
        1.0f,
        -1.0f,
        1.0f,
        1.0f, // bottom right
        1.0f,
        1.0f,
        1.0f,
        0.0f // top right
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
    GLuint ebo{};
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
    const QSize mapSize(width(), height());

    qDebug() << "GLWidget::paintGL - DPR:" << dpr << "Map size:" << mapSize << "Widget size:" << width() << "x"
             << height();

    // Resize map if needed
    d_ptr->m_map->resize(mapSize, dpr);

    auto *f = QOpenGLContext::currentContext()->extraFunctions();

    // Update framebuffer for MapLibre to render into
    d_ptr->m_map->updateRenderer(mapSize, dpr, d_ptr->m_mapFramebuffer);

    // Render the map to FBO
    d_ptr->m_map->render();

    // Update map size if needed

    // For zero-copy rendering, get the texture from OpenGL backend
    GLuint textureId = d_ptr->m_map->getFramebufferTextureId();

    // Fallback to hardcoded ID if backend method not available
    if (textureId == 0) {
        textureId = 3; // Based on debug output
    }

    // Only log key info to reduce debug spam
    static int paintCount = 0;
    if (paintCount++ % 60 == 0) { // Log every 60 frames
        qDebug() << "GLWidget::paintGL - Using texture ID:" << textureId << "FBO:" << d_ptr->m_mapFramebuffer;
    }

    if (d_ptr->m_mapFramebuffer > 0 && textureId > 0) {
        // Bind default framebuffer for rendering to screen
        f->glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());

        // Set viewport to match the actual framebuffer size
        // QOpenGLWidget handles DPR internally, so we need to use the actual framebuffer size
        const QSize fbSize = size() * devicePixelRatioF();
        f->glViewport(0, 0, fbSize.width(), fbSize.height());
        // Set viewport to match the framebuffer size

        // Clear the screen with black background
        f->glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        // Use shader to render the texture
        d_ptr->m_shaderProgram->bind();

        // Bind the MapLibre texture
        f->glActiveTexture(GL_TEXTURE0);
        f->glBindTexture(GL_TEXTURE_2D, textureId);

        // Don't set texture parameters - MapLibre has already configured them correctly
        // The texture is already set up for proper rendering including SDF fonts

        // Debug texture properties only once
        static bool textureLogged = false;
        if (!textureLogged && textureId > 0) {
#if !defined(Q_OS_IOS) && !defined(Q_OS_ANDROID)
            GLint texWidth{};
            GLint texHeight{};
            GLint texFormat{};
            f->glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &texWidth);
            f->glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &texHeight);
            f->glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &texFormat);
            qDebug() << "Texture info - Size:" << texWidth << "x" << texHeight
                     << "Format:" << QString::number(texFormat, 16);
#endif
            textureLogged = true;
        }

        d_ptr->m_shaderProgram->setUniformValue("mapTexture", 0);

        // Ensure proper pixel alignment for text rendering
        f->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        // Disable depth test for fullscreen quad
        f->glDisable(GL_DEPTH_TEST);

        // Set proper blending for rendering MapLibre's premultiplied alpha texture
        f->glEnable(GL_BLEND);
        // MapLibre uses premultiplied alpha, so we need to use the correct blend function
        f->glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

        // Draw fullscreen quad
        f->glBindVertexArray(d_ptr->m_vao);
        f->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        f->glBindVertexArray(0);

        d_ptr->m_shaderProgram->release();

        // Re-enable depth test
        f->glEnable(GL_DEPTH_TEST);
    }
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
