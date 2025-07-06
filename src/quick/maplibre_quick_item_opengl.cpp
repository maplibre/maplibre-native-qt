// MapLibreQuickItem OpenGL backend implementation using QQuickFramebufferObject

#include "maplibre_quick_item_opengl.hpp"

#include <QQuickWindow>
#include <QOpenGLFramebufferObject>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QDebug>
#include <cmath>
#include <QMapLibre/Map>
#include <QMapLibre/Settings>
#include <QMapLibre/Types>

#include "utils/opengl_renderer_backend.hpp"

using namespace QMapLibreQuick;
using namespace QMapLibre;

namespace QMapLibreQuick {

/**
 * @brief Custom renderer for MapLibre using QQuickFramebufferObject::Renderer
 */
class MapLibreRenderer : public QQuickFramebufferObject::Renderer {
public:
    MapLibreRenderer(MapLibreQuickItemOpenGL* item) : m_item(item) {
        qDebug() << "Creating MapLibreRenderer";
    }

protected:
    void render() override {
        if (!m_item->m_map) {
            qDebug() << "No map available for rendering";
            return;
        }

        qDebug() << "MapLibreRenderer::render() called";
        
        // Get the current FBO from Qt
        QOpenGLFramebufferObject* fbo = framebufferObject();
        if (!fbo) {
            qWarning() << "No FBO available";
            return;
        }

        const QSize fboSize = fbo->size();
        qDebug() << "Rendering to FBO size:" << fboSize;

        // Update map size if needed
        const QSize itemSize(static_cast<int>(m_item->width()), 
                           static_cast<int>(m_item->height()));
        m_item->m_map->resize(itemSize);

        // Set the FBO for MapLibre to render into
        m_item->m_map->setOpenGLFramebufferObject(fbo->handle(), fboSize);

        // Get OpenGL context and functions
        QOpenGLContext* context = QOpenGLContext::currentContext();
        if (!context) {
            qWarning() << "No current OpenGL context";
            return;
        }

        QOpenGLFunctions* f = context->functions();
        if (!f) {
            qWarning() << "Failed to get OpenGL functions";
            return;
        }

        // Clear the FBO
        f->glViewport(0, 0, fboSize.width(), fboSize.height());
        f->glClearColor(0.2f, 0.2f, 0.3f, 1.0f);  // Dark blue background
        f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Render the map
        try {
            qDebug() << "Calling map render";
            m_item->m_map->render();
            qDebug() << "Map render completed successfully";
        } catch (const std::exception& e) {
            qWarning() << "MapLibre render failed:" << e.what();
        }
    }

    QOpenGLFramebufferObject* createFramebufferObject(const QSize& size) override {
        qDebug() << "Creating FBO of size:" << size;
        
        // Create FBO with depth and stencil buffers
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        format.setSamples(4); // Enable 4x MSAA
        
        auto* fbo = new QOpenGLFramebufferObject(size, format);
        
        // The FBO will be used by MapLibre which expects OpenGL coordinate system
        // (origin at bottom-left), but QML expects top-left origin.
        // This will be handled in the render() method.
        
        return fbo;
    }

private:
    MapLibreQuickItemOpenGL* m_item;
};

} // namespace QMapLibreQuick

MapLibreQuickItemOpenGL::MapLibreQuickItemOpenGL() {
    qDebug() << "Creating MapLibreQuickItemOpenGL";
    setFlag(ItemHasContents, true);
    setFlag(ItemAcceptsInputMethod, true);
    setAcceptedMouseButtons(Qt::AllButtons);
    setAcceptTouchEvents(true);
    setAcceptHoverEvents(true);
    
    // Enable focus so we can receive keyboard and mouse events
    setActiveFocusOnTab(true);
    setFlag(ItemIsFocusScope, true);
    
    // Important: Enable mouse tracking to receive move events even when not pressed
    setKeepMouseGrab(true);
    setKeepTouchGrab(true);
    
    // Set mirror vertically to correct OpenGL coordinate system vs QML coordinate system
    setMirrorVertically(true);
    
    qDebug() << "MapLibreQuickItemOpenGL constructor complete - accepted mouse buttons:" << acceptedMouseButtons();
}

QQuickFramebufferObject::Renderer* MapLibreQuickItemOpenGL::createRenderer() const {
    qDebug() << "MapLibreQuickItemOpenGL::createRenderer() called";
    
    // Ensure map is created
    if (!m_map) {
        const int w = static_cast<int>(width());
        const int h = static_cast<int>(height());
        const float dpr = window() ? window()->devicePixelRatio() : 1.0f;
        
        if (w > 0 && h > 0) {
            const_cast<MapLibreQuickItemOpenGL*>(this)->ensureMap(w, h, dpr);
        }
    }
    
    return new MapLibreRenderer(const_cast<MapLibreQuickItemOpenGL*>(this));
}

void MapLibreQuickItemOpenGL::ensureMap(int w, int h, float dpr) {
    if (m_map) {
        return;
    }

    qDebug() << "Creating MapLibre map with size:" << w << "x" << h << "dpr:" << dpr;

    // Create map settings
    Settings settings;
    settings.setApiKey("");  // No API key needed for OpenStreetMap styles
    settings.setApiBaseUrl("");
    settings.setLocalFontFamily("Arial");
    settings.setContextMode(Settings::GLContextMode::SharedGLContext);

    // Create the map using standard constructor (Map will handle renderer backend internally)
    m_map = std::make_unique<Map>(nullptr, settings, QSize(w, h), dpr);

    // Connect to needsRendering signal to trigger updates
    connect(m_map.get(), &Map::needsRendering, this, &QQuickFramebufferObject::update);

    // Explicitly create the renderer
    m_map->createRenderer();
    qDebug() << "Created map renderer";

    // Set default coordinate and zoom
    m_map->setCoordinateZoom(Coordinate(43.0, 21.0), 5.0);

    // Set a public style that doesn't require API key
    m_map->setStyleUrl("https://demotiles.maplibre.org/style.json");

    qDebug() << "MapLibre map created successfully";
}

// Mouse event handlers
void MapLibreQuickItemOpenGL::mousePressEvent(QMouseEvent* event) {
    qDebug() << "Mouse press at:" << event->position() << "button:" << event->button();
    if (event->button() == Qt::LeftButton) {
        m_lastMousePos = event->position();
        m_dragging = true;
        grabMouse(); // Grab mouse to ensure we get move events
        qDebug() << "Started dragging and grabbed mouse";
    }
    event->accept(); // Accept the event to ensure we get subsequent events
}

void MapLibreQuickItemOpenGL::mouseMoveEvent(QMouseEvent* event) {
    qDebug() << "Mouse move event - dragging:" << m_dragging << "position:" << event->position();
    if (m_dragging && m_map) {
        qDebug() << "Processing drag - delta:" << (event->position() - m_lastMousePos);
        const QPointF delta = event->position() - m_lastMousePos;
        
        // Use MapLibre's moveBy method which handles pixel offset directly
        // Fix the direction: move in the same direction as the mouse drag
        const QPointF offset(delta.x(), delta.y());
        m_map->moveBy(offset);
        
        m_lastMousePos = event->position();
        update(); // Trigger re-render
    }
    event->accept();
}

void MapLibreQuickItemOpenGL::mouseReleaseEvent(QMouseEvent* event) {
    qDebug() << "Mouse release, button:" << event->button();
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        ungrabMouse(); // Release mouse grab
        qDebug() << "Stopped dragging and released mouse grab";
    }
    event->accept();
}

void MapLibreQuickItemOpenGL::wheelEvent(QWheelEvent* event) {
    if (m_map) {
        const QPoint numDegrees = event->angleDelta() / 8;
        if (!numDegrees.isNull()) {
            const double currentZoom = m_map->zoom();
            const double zoomDelta = numDegrees.y() / 120.0; // Standard wheel delta
            const double newZoom = std::max(0.0, std::min(22.0, currentZoom + zoomDelta));
            
            m_map->setZoom(newZoom);
            update(); // Trigger re-render
        }
    }
    QQuickFramebufferObject::wheelEvent(event);
}
