// MapLibreQuickItem Vulkan backend implementation

#include "maplibre_quick_item_vulkan.hpp"

#include <QDebug>
#include <QTimer>
#include <QMapLibre/Map>
#include <QMapLibre/Settings>
#include <QMapLibre/Types>
#include <QMouseEvent>
#include <QQuickWindow>
#include <QSGSimpleTextureNode>
#include <QSGSimpleRectNode>
#include <QWheelEvent>
#include <cmath>

using namespace QMapLibreQuick;
using namespace QMapLibre;

MapLibreQuickItemVulkan::MapLibreQuickItemVulkan() {
    setFlag(ItemHasContents, true);
    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton | Qt::MiddleButton);
    setAcceptHoverEvents(true);
    
    // Set a default size to ensure the item is visible
    setImplicitWidth(400);
    setImplicitHeight(300);
}

MapLibreQuickItemVulkan::~MapLibreQuickItemVulkan() {
    releaseResources();
}

void MapLibreQuickItemVulkan::releaseResources() {
    m_map.reset();
}

void MapLibreQuickItemVulkan::ensureMap(int w, int h, float dpr) {
    if (m_map) {
        return;
    }

    if (w <= 0 || h <= 0 || dpr <= 0.0f) {
        return;
    }

    try {
        // Create with minimal settings
        Settings settings;
        settings.setContextMode(Settings::SharedGLContext);
        
        m_map = std::make_unique<Map>(nullptr, settings, QSize(w * dpr, h * dpr), dpr);
        
        if (m_map) {
            // Connect to the map's rendering signals
            QObject::connect(m_map.get(), &Map::needsRendering, this, [this]() {
                update();
            });
            
            // Set style and coordinates after a delay
            QTimer::singleShot(1000, this, [this]() {
                if (m_map) {
                    m_map->setStyleUrl("https://demotiles.maplibre.org/style.json");
                    m_map->setCoordinateZoom(QMapLibre::Coordinate{51.5074, -0.1278}, 4.0);
                    m_map->setConnectionEstablished();
                }
            });
        }
    } catch (const std::exception &e) {
        qWarning() << "Failed to create MapLibre map:" << e.what();
        m_map.reset();
    }
}

QSGNode *MapLibreQuickItemVulkan::updatePaintNode(QSGNode *node, UpdatePaintNodeData *) {
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

    // Create renderer if needed
    if (!m_rendererBound) {
        m_map->createRendererWithVulkanWindow(window());
        m_rendererBound = true;
    }

    // Ensure rendering happens before getting texture
    if (m_rendererBound && m_map) {
        try {
            m_map->render();
        } catch (const std::exception &e) {
            qWarning() << "Exception during render:" << e.what();
        }
    }

    // Try to get the native Vulkan texture
    void *nativeTex = nullptr;
    if (m_rendererBound && m_map) {
        try {
            nativeTex = m_map->nativeColorTexture();
        } catch (const std::exception &e) {
            qWarning() << "Exception getting native texture:" << e.what();
        }
    }

    if (nativeTex) {
        // We have a texture - try to display it
        const int texWidth = width() * window()->devicePixelRatio();
        const int texHeight = height() * window()->devicePixelRatio();
        
        auto *textureNode = dynamic_cast<QSGSimpleTextureNode *>(node);
        if (!textureNode) {
            if (node) {
                delete node;
            }
            textureNode = new QSGSimpleTextureNode();
            node = textureNode;
        }

        // Try to read pixel data from the map
        if (auto imageData = m_map->readVulkanImageData()) {
            if (imageData && imageData->data.get()) {
                QImage mapImage(imageData->data.get(), 
                               imageData->size.width, 
                               imageData->size.height, 
                               QImage::Format_RGBA8888);
                
                if (!mapImage.isNull()) {
                    // Scale to display size if needed
                    if (mapImage.size() != QSize(texWidth, texHeight)) {
                        mapImage = mapImage.scaled(texWidth, texHeight, Qt::KeepAspectRatio, Qt::FastTransformation);
                    }
                    
                    // Create Qt texture from the map image
                    QSGTexture *qtTexture = window()->createTextureFromImage(mapImage);
                    if (qtTexture) {
                        qtTexture->setFiltering(QSGTexture::Linear);
                        textureNode->setTexture(qtTexture);
                        textureNode->setRect(boundingRect());
                        textureNode->setOwnsTexture(true);
                        return textureNode;
                    }
                }
            }
        }
    }

    // Return transparent placeholder if no texture available
    auto *rectNode = dynamic_cast<QSGSimpleRectNode *>(node);
    if (!rectNode) {
        if (node) {
            delete node;
        }
        rectNode = new QSGSimpleRectNode();
        node = rectNode;
    }
    
    rectNode->setColor(QColor(0, 0, 0, 0)); // Transparent
    rectNode->setRect(boundingRect());
    return rectNode;
}

void MapLibreQuickItemVulkan::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) {
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    
    if (m_map && newGeometry.size() != oldGeometry.size()) {
        const float dpr = window() ? window()->devicePixelRatio() : 1.0f;
        m_map->resize(QSize(static_cast<int>(newGeometry.width() * dpr),
                           static_cast<int>(newGeometry.height() * dpr)));
        update();
    }
}

void MapLibreQuickItemVulkan::itemChange(ItemChange change, const ItemChangeData &data) {
    QQuickItem::itemChange(change, data);

    if (change == ItemSceneChange) {
        if (QQuickWindow *win = window()) {
            QObject::connect(win, &QQuickWindow::sceneGraphInitialized, this, [this]() {
                if (!m_map) {
                    ensureMap(width(), height(), window()->devicePixelRatio());
                }
            }, Qt::DirectConnection);
        }
    }
}

// Mouse interaction
void MapLibreQuickItemVulkan::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_lastMousePos = event->position();
        m_dragging = true;
    }
    event->accept();
}

void MapLibreQuickItemVulkan::mouseMoveEvent(QMouseEvent *event) {
    if (m_dragging && m_map) {
        const QPointF delta = event->position() - m_lastMousePos;
        m_map->moveBy(delta);
        m_lastMousePos = event->position();
        update();
    }
    event->accept();
}

void MapLibreQuickItemVulkan::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
    }
    event->accept();
}

void MapLibreQuickItemVulkan::wheelEvent(QWheelEvent *event) {
    if (m_map) {
        const QPoint numDegrees = event->angleDelta() / 8;
        if (!numDegrees.isNull()) {
            const double currentZoom = m_map->zoom();
            const double zoomDelta = numDegrees.y() / 120.0;
            const double newZoom = std::max(0.0, std::min(22.0, currentZoom + zoomDelta));
            m_map->setZoom(newZoom);
            update();
        }
    }
    QQuickItem::wheelEvent(event);
}