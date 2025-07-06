// MapLibreQuickItem base implementation

#include "maplibre_quick_item_base.hpp"

#include <QDebug>
#include <QMapLibre/Map>
#include <QMapLibre/Settings>
#include <QMouseEvent>
#include <QQuickWindow>
#include <QWheelEvent>

using namespace QMapLibreQuick;
using namespace QMapLibre;

MapLibreQuickItemBase::MapLibreQuickItemBase() {
    setFlag(ItemHasContents, true);
    // Handle mouse interactions (press/move/release) on the left button.
    setAcceptedMouseButtons(Qt::LeftButton);
}

QSGNode* MapLibreQuickItemBase::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* data) {
    if (!m_map) {
        const int w = static_cast<int>(width());
        const int h = static_cast<int>(height());
        const float dpr = window() ? window()->devicePixelRatio() : 1.0f;

        if (w > 0 && h > 0) {
            qDebug() << "Creating map in updatePaintNode";
            ensureMap(w, h, dpr);
            initializeBackend();
        }
    }

    if (!m_map || !window()) {
        return oldNode;
    }

    return renderFrame(oldNode);
}

void MapLibreQuickItemBase::geometryChange(const QRectF& newG, const QRectF& oldG) {
    QQuickItem::geometryChange(newG, oldG);

    const QSize newSize(newG.width(), newG.height());
    if (newSize != m_size) {
        m_size = newSize;
        // Trigger re-rendering
        update();
    }
}

void MapLibreQuickItemBase::releaseResources() {
    cleanupBackend();
    m_map.reset();
}

void MapLibreQuickItemBase::itemChange(ItemChange change, const ItemChangeData& data) {
    QQuickItem::itemChange(change, data);

    if (change == ItemSceneChange && data.window) {
        // Connect to window changed signals if needed
        if (!m_connected) {
            m_connected = true;
        }
    }
}

void MapLibreQuickItemBase::ensureMap(int w, int h, float dpr, void* /*backendSpecificData*/) {
    if (m_map) {
        return;
    }

    qDebug() << "Creating MapLibre map with size:" << w << "x" << h << "dpr:" << dpr;

    // Create map settings
    Settings settings;
    settings.setApiKey(""); // No API key needed for OpenStreetMap styles
    settings.setApiBaseUrl("");
    settings.setLocalFontFamily("Arial");
    settings.setContextMode(Settings::GLContextMode::SharedGLContext);

    // Create the map
    m_map = std::make_unique<Map>(nullptr, settings, QSize(w, h), dpr);

    // Set a public style that doesn't require API key
    m_map->setStyleUrl("https://demotiles.maplibre.org/style.json");

    qDebug() << "MapLibre map created successfully";
}

// Mouse event handlers
void MapLibreQuickItemBase::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_lastMousePos = event->position();
        m_dragging = true;
    }
    QQuickItem::mousePressEvent(event);
}

void MapLibreQuickItemBase::mouseMoveEvent(QMouseEvent* event) {
    if (m_dragging && m_map) {
        const QPointF delta = event->position() - m_lastMousePos;
        // Implement map panning here
        m_lastMousePos = event->position();
    }
    QQuickItem::mouseMoveEvent(event);
}

void MapLibreQuickItemBase::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
    }
    QQuickItem::mouseReleaseEvent(event);
}

void MapLibreQuickItemBase::wheelEvent(QWheelEvent* event) {
    if (m_map) {
        // Implement zoom functionality here
        const QPoint numDegrees = event->angleDelta() / 8;
        if (!numDegrees.isNull()) {
            const double zoomFactor = numDegrees.y() > 0 ? 1.1 : 0.9;
            // Apply zoom
        }
    }
    QQuickItem::wheelEvent(event);
}
