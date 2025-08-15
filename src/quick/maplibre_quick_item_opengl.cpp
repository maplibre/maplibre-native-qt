// MapLibreQuickItem QRhi backend implementation for cross-platform compatibility

#include "maplibre_quick_item_opengl.hpp"

#include <QtQuick/qsgrendererinterface.h>
#include <qopengl.h>
#include <QDebug>
#include <QMapLibre/Map>
#include <QMapLibre/Settings>
#include <QMapLibre/Types>
#include <QMouseEvent>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QQuickWindow>
#include <QSGImageNode>
#include <QSGSimpleTextureNode>
#include <QTimer>
#include <QWheelEvent>
#include <cmath>
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

    // Clean up framebuffer
    if (m_fbo != 0) {
        if (QOpenGLContext *context = QOpenGLContext::currentContext()) {
            QOpenGLFunctions *gl = context->functions();
            gl->glDeleteFramebuffers(1, &m_fbo);
        }
    }

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

            // Set up default map settings
            qDebug() << "Setting map style and coordinates";
            m_map->setStyleUrl("https://demotiles.maplibre.org/style.json");
            m_map->setCoordinateZoom(QMapLibre::Coordinate{60.170448, 24.942046}, 10); // Helsinki
            m_map->setConnectionEstablished();

            qDebug() << "MapLibreQuickItemOpenGL: Map configured";
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

    // We'll create the node later based on what type works best
    if (!node) {
        qDebug() << "OPENGL TEXTURE RENDERING: Node will be created after texture is ready";
    }

    // Perform map rendering and get texture
    if (m_map) {
        try {
            // Update map size if needed - pass logical size, mbgl::Map handles DPI internally
            const QSize mapSize(static_cast<int>(width()), static_cast<int>(height()));
            m_map->resize(mapSize);

            // CRITICAL: Set up framebuffer for texture sharing before rendering
            QOpenGLContext *context = QOpenGLContext::currentContext();
            if (context) {
                try {
                    // Create a real framebuffer for MapLibre rendering if needed
                    QOpenGLFunctions *gl = context->functions();
                    if (m_fbo == 0) {
                        gl->glGenFramebuffers(1, &m_fbo);
                        qDebug() << "OPENGL TEXTURE RENDERING: Created new framebuffer" << m_fbo;
                    }

                    m_map->updateRenderer(mapSize, m_fbo);
                    qDebug() << "OPENGL TEXTURE RENDERING: Configured framebuffer" << m_fbo << "with size" << mapSize;

                    // Don't clear - let MapLibre handle its own clearing
                } catch (const std::exception &e) {
                    qWarning() << "OPENGL TEXTURE RENDERING: Exception during framebuffer setup:" << e.what();
                    return node;
                }
            }

            // Render the map to the configured FBO
            qDebug() << "OPENGL TEXTURE RENDERING: Calling map render";

            try {
                qDebug() << "Before render - Style URL:" << m_map->styleUrl();
                auto currentCoord = m_map->coordinate();
                qDebug() << "Before render - Coordinate:" << currentCoord.first << "," << currentCoord.second
                         << "zoom:" << m_map->zoom();

                // Save and restore OpenGL state to prevent conflicts
                QOpenGLFunctions *gl = context->functions();
                GLint prevFbo;
                gl->glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFbo);

                m_map->render();

                // Ensure we flush the OpenGL commands
                gl->glFlush();

                // Restore previous framebuffer
                gl->glBindFramebuffer(GL_FRAMEBUFFER, prevFbo);

                qDebug() << "OPENGL TEXTURE RENDERING: Map render completed";
            } catch (const std::exception &e) {
                qWarning() << "OPENGL TEXTURE RENDERING: Exception during map render:" << e.what();
                return node;
            } catch (...) {
                qWarning() << "OPENGL TEXTURE RENDERING: Unknown exception during map render";
                return node;
            }

            // Try to get MapLibre's OpenGL framebuffer texture ID for zero-copy sharing
            GLuint maplibreTextureId = m_map->getFramebufferTextureId();
            if (maplibreTextureId > 0) {
                qDebug() << "OPENGL TEXTURE RENDERING: Got MapLibre framebuffer texture ID:" << maplibreTextureId;

                // Wrap it directly as QSGTexture (zero-copy!)
                // Use alpha channel flag for proper transparency handling
                QSGTexture *qtTexture = QNativeInterface::QSGOpenGLTexture::fromNative(
                    maplibreTextureId, window(), mapSize, QQuickWindow::TextureHasAlphaChannel);

                if (qtTexture) {
                    qDebug() << "OPENGL TEXTURE RENDERING: Successfully created zero-copy texture from framebuffer";

                    // Create texture node for rendering
                    if (node) {
                        delete node;
                    }

                    qDebug() << "OPENGL TEXTURE RENDERING: Creating QSGSimpleTextureNode";
                    QSGSimpleTextureNode *textureNode = new QSGSimpleTextureNode();
                    textureNode->setTexture(qtTexture);
                    textureNode->setRect(boundingRect());
                    textureNode->setFiltering(QSGTexture::Linear);

                    // Flip Y coordinates for OpenGL framebuffer
                    QSGGeometry *geometry = textureNode->geometry();
                    if (geometry && geometry->vertexCount() == 4) {
                        QSGGeometry::TexturedPoint2D *vertices = geometry->vertexDataAsTexturedPoint2D();

                        // Y-flip for OpenGL framebuffer
                        vertices[0].ty = 1.0f - vertices[0].ty;
                        vertices[1].ty = 1.0f - vertices[1].ty;
                        vertices[2].ty = 1.0f - vertices[2].ty;
                        vertices[3].ty = 1.0f - vertices[3].ty;

                        textureNode->markDirty(QSGNode::DirtyGeometry);
                    }

                    textureNode->setOwnsTexture(false); // Don't delete MapLibre's texture!

                    return textureNode;
                } else {
                    qDebug() << "OPENGL TEXTURE RENDERING: Failed to wrap framebuffer texture";
                }
            } else {
                qDebug()
                    << "OPENGL TEXTURE RENDERING: No framebuffer texture ID available - zero-copy not supported yet";
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
        // Pass logical size; mbgl::Map handles DPI scaling internally
        m_map->resize(QSize(static_cast<int>(newGeometry.width()), static_cast<int>(newGeometry.height())));
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
