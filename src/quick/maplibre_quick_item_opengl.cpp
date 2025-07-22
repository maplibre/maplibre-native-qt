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
#include <QSGSimpleTextureNode>
#include <qopengl.h>
#include <QOpenGLContext>
#include <QtQuick/qsgrendererinterface.h>
#include <vector>

using namespace QMapLibreQuick;

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
            qDebug() << "Map style URL:" << m_map->styleUrl();
            auto coord = m_map->coordinate();
            qDebug() << "Map coordinate:" << coord.first << "," << coord.second;
            qDebug() << "Map zoom:" << m_map->zoom();
        }
    } catch (const std::exception &e) {
        qWarning() << "Failed to create MapLibre map:" << e.what();
        m_map.reset();
    }
}

QSGNode *MapLibreQuickItemOpenGL::updatePaintNode(QSGNode *node, UpdatePaintNodeData *) {
    qDebug() << "=== OPENGL TEXTURE RENDERING: updatePaintNode called ===";
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

    // Create or reuse texture node
    auto *textureNode = dynamic_cast<QSGSimpleTextureNode *>(node);
    if (!textureNode) {
        if (node) {
            delete node;
        }
        qDebug() << "OPENGL TEXTURE RENDERING: Creating new QSGSimpleTextureNode";
        textureNode = new QSGSimpleTextureNode();
        textureNode->setFiltering(QSGTexture::Linear);
        node = textureNode;
    }

    // Perform map rendering and get texture
    if (m_map) {
        try {
            // Update map size if needed
            const qreal dpr = window()->devicePixelRatio();
            const QSize mapSize(static_cast<int>(width() * dpr), static_cast<int>(height() * dpr));
            m_map->resize(mapSize);
            
            // CRITICAL: Set up framebuffer for texture sharing before rendering
            QOpenGLContext* context = QOpenGLContext::currentContext();
            if (context) {
                // Create a dedicated framebuffer for MapLibre rendering
                // We need a non-zero FBO ID to trigger the OpenGL backend's texture creation
                GLuint framebufferForMapLibre = 1; // Use FBO ID 1 to trigger proper texture setup
                m_map->setOpenGLFramebufferObject(framebufferForMapLibre, mapSize);
                qDebug() << "OPENGL TEXTURE RENDERING: Configured framebuffer" << framebufferForMapLibre << "with size" << mapSize;
            }
            
            // Render the map to the configured FBO
            qDebug() << "OPENGL TEXTURE RENDERING: Calling map render";
            qDebug() << "Before render - Style URL:" << m_map->styleUrl();
            auto currentCoord = m_map->coordinate();
            qDebug() << "Before render - Coordinate:" << currentCoord.first << "," << currentCoord.second << "zoom:" << m_map->zoom();
            m_map->render();
            qDebug() << "OPENGL TEXTURE RENDERING: Map render completed";
            
            // Try to get MapLibre's OpenGL framebuffer texture ID for zero-copy sharing
            GLuint maplibreTextureId = m_map->getFramebufferTextureId();
            if (maplibreTextureId > 0) {
                qDebug() << "OPENGL TEXTURE RENDERING: Got MapLibre framebuffer texture ID:" << maplibreTextureId;
                
                // Wrap it directly as QSGTexture (zero-copy!)
                QSGTexture *qtTexture = QNativeInterface::QSGOpenGLTexture::fromNative(
                    maplibreTextureId,
                    window(),
                    mapSize,
                    QQuickWindow::TextureHasAlphaChannel
                );
                
                if (qtTexture) {
                    qDebug() << "OPENGL TEXTURE RENDERING: Successfully created zero-copy texture from framebuffer";
                    qtTexture->setFiltering(QSGTexture::Linear);
                    
                    // Create a new texture node and manually set flipped texture coordinates
                    delete textureNode;
                    textureNode = new QSGSimpleTextureNode();
                    textureNode->setTexture(qtTexture);
                    textureNode->setRect(boundingRect());
                    
                    // Get the geometry and manually flip the texture coordinates
                    QSGGeometry *geometry = textureNode->geometry();
                    if (geometry && geometry->vertexCount() == 4) {
                        QSGGeometry::TexturedPoint2D *vertices = geometry->vertexDataAsTexturedPoint2D();
                        
                        // Standard Qt texture coordinates (top-left origin):
                        // vertices[0] = top-left:     (x0, y0, 0.0, 0.0)
                        // vertices[1] = bottom-left:  (x0, y1, 0.0, 1.0)  
                        // vertices[2] = top-right:    (x1, y0, 1.0, 0.0)
                        // vertices[3] = bottom-right: (x1, y1, 1.0, 1.0)
                        
                        // For OpenGL framebuffer (bottom-left origin), flip Y coords:
                        // We want: top maps to bottom of texture, bottom maps to top
                        vertices[0].ty = 1.0f; // top-left -> use bottom of texture
                        vertices[1].ty = 0.0f; // bottom-left -> use top of texture  
                        vertices[2].ty = 1.0f; // top-right -> use bottom of texture
                        vertices[3].ty = 0.0f; // bottom-right -> use top of texture
                        
                        textureNode->markDirty(QSGNode::DirtyGeometry);
                        qDebug() << "OPENGL TEXTURE RENDERING: Flipped texture coordinates for correct orientation";
                    }
                    
                    textureNode->setOwnsTexture(false); // Don't delete MapLibre's texture!
                    return textureNode;
                } else {
                    qDebug() << "OPENGL TEXTURE RENDERING: Failed to wrap framebuffer texture";
                }
            } else {
                qDebug() << "OPENGL TEXTURE RENDERING: No framebuffer texture ID available - zero-copy not supported yet";
            }
        } catch (const std::exception &e) {
            qWarning() << "OPENGL TEXTURE RENDERING: Exception:" << e.what();
        }
    }

    // Zero-copy texture sharing failed - this should not happen in normal operation
    qWarning() << "OPENGL TEXTURE RENDERING: Zero-copy texture sharing unavailable";
    return nullptr;
}

QMapLibre::Map *MapLibreQuickItemOpenGL::getMap() const {
    return m_map.get();
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
