// MapLibreQuickItem QRhi backend implementation for cross-platform compatibility

#include "maplibre_quick_item_opengl.hpp"

#include <QDebug>
#include <QMapLibre/Map>
#include <QMapLibre/Settings>
#include <QMapLibre/Types>
#include <QMouseEvent>
#include <QQuickWindow>
#include <QTimer>
#include <QWheelEvent>
#include <cmath>
#include <vector>

using namespace QMapLibreQuick;

// ===============================
// Simple Custom Render Node (minimal implementation)
// ===============================
class MapLibreDirectRenderNode : public QSGRenderNode {
public:
    explicit MapLibreDirectRenderNode(MapLibreQuickItemOpenGL *item)
        : m_item(item) {}

    void render(const RenderState *state) override {
        if (m_item) {
            m_item->performDirectRendering();
        }
    }

    QSGRenderNode::RenderingFlags flags() const override {
        return QSGRenderNode::BoundedRectRendering | QSGRenderNode::OpaqueRendering;
    }

    QRectF rect() const override { return m_item ? QRectF(0, 0, m_item->width(), m_item->height()) : QRectF(); }

private:
    MapLibreQuickItemOpenGL *m_item = nullptr;
};

MapLibreQuickItemOpenGL::MapLibreQuickItemOpenGL(QQuickItem *parent)
    : QQuickItem(parent) {
    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton | Qt::MiddleButton);
    setAcceptHoverEvents(true);

    // Enable custom paint node rendering
    setFlag(ItemHasContents, true);

    // Set a default size to ensure the item is visible
    setImplicitWidth(400);
    setImplicitHeight(300);

    qDebug() << "MapLibreQuickItemOpenGL: Initialized with QRhi backend for cross-platform compatibility";
}

MapLibreQuickItemOpenGL::~MapLibreQuickItemOpenGL() {
    qDebug() << "MapLibreQuickItemOpenGL: Destroying OpenGL item";
    m_map.reset();
}

void MapLibreQuickItemOpenGL::ensureMap(const int width, const int height, const float pixelRatio) {
    if (m_map || width <= 0 || height <= 0) {
        return;
    }

    qDebug() << "MapLibreQuickItemOpenGL: Creating map with size" << width << "x" << height << "DPR:" << pixelRatio;

    try {
        // Create with minimal settings optimized for OpenGL performance
        QMapLibre::Settings settings;
        settings.setContextMode(QMapLibre::Settings::SharedGLContext);

        m_map = std::make_unique<QMapLibre::Map>(nullptr, settings, QSize(width, height), pixelRatio);

        if (m_map) {
            // Connect to the map's rendering signals
            connect(
                m_map.get(), &QMapLibre::Map::needsRendering, this, &MapLibreQuickItemOpenGL::handleMapNeedsRendering);

            // Set up map for tile rendering
            qDebug() << "Setting map style and coordinates";
            m_map->setStyleUrl("https://demotiles.maplibre.org/style.json");

            // Set coordinates with good tile coverage (London) and reasonable zoom
            m_map->setCoordinateZoom(QMapLibre::Coordinate{51.505, -0.09}, 8.0); // London, zoom 8
            m_map->setPitch(0.0);                                                // No tilt for clearer tile visibility
            m_map->setBearing(0.0);                                              // No rotation
            m_map->setConnectionEstablished();

            qDebug() << "MapLibreQuickItemOpenGL: Map configured for London at zoom 8";
        }
    } catch (const std::exception &e) {
        qWarning() << "Failed to create MapLibre map:" << e.what();
        m_map.reset();
    }
}

QSGNode *MapLibreQuickItemOpenGL::updatePaintNode(QSGNode *node, UpdatePaintNodeData *) {
    qDebug() << "=== DIRECT OPENGL RENDERING: updatePaintNode called ===";
    if (!window()) {
        return node;
    }

    // Check for valid size
    if (width() <= 0 || height() <= 0) {
        return node;
    }

    // Ensure we have a map
    if (!m_map) {
        ensureMap(width(), height(), window()->devicePixelRatio());
        if (!m_map) {
            return node;
        }
    }

    // This is the OpenGL-specific Quick item

    // DIRECT APPROACH: Use simple custom render node
    qDebug() << "DIRECT OPENGL RENDERING: Using direct OpenGL rendering";

    // Create or reuse direct render node
    MapLibreDirectRenderNode *renderNode = static_cast<MapLibreDirectRenderNode *>(node);
    if (!renderNode) {
        qDebug() << "DIRECT OPENGL RENDERING: Creating new direct render node";
        renderNode = new MapLibreDirectRenderNode(this);
    }

    qDebug() << "DIRECT OPENGL RENDERING: Render node ready";
    qDebug() << "DIRECT OPENGL RENDERING: Size:" << width() << "x" << height()
             << "DPR:" << window()->devicePixelRatio();

    return renderNode;
}

QMapLibre::Map *MapLibreQuickItemOpenGL::getMap() const {
    return m_map.get();
}

void MapLibreQuickItemOpenGL::performDirectRendering() {
    if (!m_map) {
        qDebug() << "MapLibreQuickItemOpenGL: No map available for rendering";
        return;
    }

    qDebug() << "MapLibreQuickItemOpenGL: Executing direct OpenGL rendering";

    // Update map size if needed
    if (window()) {
        const qreal dpr = window()->devicePixelRatio();
        const QSize mapSize(static_cast<int>(width() * dpr), static_cast<int>(height() * dpr));
        m_map->resize(mapSize);
    }

    // Direct map rendering (OpenGL-only Quick item)
    qDebug() << "MapLibreQuickItemOpenGL: Direct map rendering";
    m_map->render();
    qDebug() << "MapLibreQuickItemOpenGL: Map rendering completed";
}

void MapLibreQuickItemOpenGL::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) {
    QQuickItem::geometryChange(newGeometry, oldGeometry);

    if (m_map && newGeometry.size() != oldGeometry.size()) {
        const float dpr = window() ? window()->devicePixelRatio() : 1.0f;
        m_map->resize(QSize(static_cast<int>(newGeometry.width() * dpr), static_cast<int>(newGeometry.height() * dpr)));
        update();
    }
}

void MapLibreQuickItemOpenGL::releaseResources() {
    // Clean up OpenGL resources
    qDebug() << "MapLibreQuickItemOpenGL: Releasing OpenGL resources";
    m_map.reset();
}

void MapLibreQuickItemOpenGL::handleMapNeedsRendering() {
    qDebug() << "TILES DEBUG: needsRendering signal received - triggering update()";
    update();
}

// Mouse interaction (same as other backends)
void MapLibreQuickItemOpenGL::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_lastMousePos = event->position();
        m_dragging = true;
        qDebug() << "Mouse press at" << m_lastMousePos;
    }
    event->accept();
}

void MapLibreQuickItemOpenGL::mouseMoveEvent(QMouseEvent *event) {
    if (m_dragging && m_map) {
        const QPointF delta = event->position() - m_lastMousePos;
        m_map->moveBy(delta);
        m_lastMousePos = event->position();
        update();
        qDebug() << "Mouse move delta" << delta;
    }
    event->accept();
}

void MapLibreQuickItemOpenGL::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        qDebug() << "Mouse released";
    }
    event->accept();
}

void MapLibreQuickItemOpenGL::wheelEvent(QWheelEvent *event) {
    if (m_map) {
        const QPoint numDegrees = event->angleDelta() / 8;
        if (!numDegrees.isNull()) {
            const double currentZoom = m_map->zoom();
            const double zoomDelta = numDegrees.y() / 120.0;
            const double newZoom = std::max(0.0, std::min(22.0, currentZoom + zoomDelta));
            m_map->setZoom(newZoom);
            update();
            qDebug() << "Zoom changed to" << newZoom;
        }
    }
    QQuickItem::wheelEvent(event);
}
