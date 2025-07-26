// MapLibreQuickItem Vulkan backend implementation

#include "maplibre_quick_item_vulkan.hpp"

#include <QtQuick/qsgtexture_platform.h>
#include <QDebug>
#include <QMapLibre/Map>
#include <QMapLibre/Settings>
#include <QMapLibre/Types>
#include <QMouseEvent>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QSGSimpleRectNode>
#include <QSGSimpleTextureNode>
#include <QTimer>
#include <QWheelEvent>
#include <cmath>

#if QT_CONFIG(vulkan)
#include <vulkan/vulkan.h>
#include <mbgl/vulkan/texture2d.hpp>
#endif

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
            // Don't connect to needsRendering signal yet - we'll do it after renderer is created
            // to avoid premature rendering

            // Connect to map changed signal to detect style loading
            QObject::connect(m_map.get(), &Map::mapChanged, this, [this](Map::MapChange change) {
                if (change == Map::MapChangeDidFinishLoadingStyle) {
                    update();
                }
            });

            // Connect to map loading failed signal
            QObject::connect(m_map.get(),
                             &Map::mapLoadingFailed,
                             this,
                             [this](Map::MapLoadingFailure failure, const QString &reason) {});

            // Set style URL and coordinates immediately
            m_map->setStyleUrl("https://demotiles.maplibre.org/style.json");
            m_map->setCoordinateZoom(QMapLibre::Coordinate{51.5074, -0.1278}, 4.0);
            m_map->setConnectionEstablished();
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

    // Check Qt's graphics API
    auto *ri = window()->rendererInterface();
    if (ri) {
        if (ri->graphicsApi() != QSGRendererInterface::Vulkan) {
            qWarning() << "Qt Quick is not using Vulkan RHI! Current API:" << ri->graphicsApi();
        }
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
        // For Qt 6.10+, Qt Quick uses RHI and manages Vulkan internally
        // MapLibre needs to create its own Vulkan instance
        QQuickWindow *qWindow = window();
        if (!qWindow) {
            qWarning() << "No window available for Vulkan renderer";
            return node;
        }

        // Create renderer with Qt's Vulkan window
        try {
            // Try to get Qt's Vulkan info and use Qt's device
            auto *ri = qWindow->rendererInterface();
            if (ri && ri->graphicsApi() == QSGRendererInterface::Vulkan) {
                // Qt returns pointers to the handles, not the handles themselves
                auto *qtPhysicalDevicePtr = reinterpret_cast<VkPhysicalDevice *>(
                    ri->getResource(qWindow, QSGRendererInterface::PhysicalDeviceResource));
                auto *qtDevicePtr = reinterpret_cast<VkDevice *>(
                    ri->getResource(qWindow, QSGRendererInterface::DeviceResource));
                auto *qtCommandQueuePtr = reinterpret_cast<VkQueue *>(
                    ri->getResource(qWindow, QSGRendererInterface::CommandQueueResource));

                VkPhysicalDevice qtPhysicalDevice = qtPhysicalDevicePtr ? *qtPhysicalDevicePtr : VK_NULL_HANDLE;
                VkDevice qtDevice = qtDevicePtr ? *qtDevicePtr : VK_NULL_HANDLE;
                VkQueue qtCommandQueue = qtCommandQueuePtr ? *qtCommandQueuePtr : VK_NULL_HANDLE;

                if (qtPhysicalDevice && qtDevice) {
                    // TODO: We need to get the graphics queue index from Qt
                    // For now, assume it's 0 (common case)
                    uint32_t graphicsQueueIndex = 0;

                    m_map->createRendererWithQtVulkanDevice(qWindow, qtPhysicalDevice, qtDevice, graphicsQueueIndex);
                } else {
                    m_map->createRendererWithVulkanWindow(qWindow);
                }
            } else {
                m_map->createRendererWithVulkanWindow(qWindow);
            }
            m_rendererBound = true;

            // Now connect to the map's rendering signals
            QObject::connect(m_map.get(), &Map::needsRendering, this, [this]() { update(); });

            // Ensure the backend has the correct size
            if (width() > 0 && height() > 0) {
                const float dpr = qWindow->devicePixelRatio();
                m_map->resize(QSize(static_cast<int>(width() * dpr), static_cast<int>(height() * dpr)));
            }
        } catch (const std::exception &e) {
            qWarning() << "Failed to create Vulkan renderer:" << e.what();
            return node;
        }
    }

    // Ensure rendering happens before getting texture
    if (m_rendererBound && m_map) {
        try {
            m_map->render();
        } catch (const std::exception &e) {
            qWarning() << "Exception during render:" << e.what();
        }
    }

    // Get the size for texture operations
    const int texWidth = width() * window()->devicePixelRatio();
    const int texHeight = height() * window()->devicePixelRatio();

    // Attempt zero-copy GPU rendering with offscreen texture
    if (m_rendererBound && m_map) {
        try {
            // Get the Vulkan texture directly for zero-copy access
            auto *vulkanTexture = m_map->getVulkanTexture();

            if (vulkanTexture) {
                // Ensure the texture is in the correct layout for sampling
                VkImage vkImage = vulkanTexture->getVulkanImage();
                VkImageLayout imageLayout = static_cast<VkImageLayout>(vulkanTexture->getVulkanImageLayout());

                // Check if we have a valid VkImage
                if (vkImage != VK_NULL_HANDLE) {
                    // Create or update texture node
                    auto *textureNode = dynamic_cast<QSGSimpleTextureNode *>(node);
                    if (!textureNode) {
                        delete node;
                        textureNode = new QSGSimpleTextureNode();
                        node = textureNode;
                    }

                    // Create QSGTexture from native Vulkan image (zero-copy)
                    QSGTexture *qtTexture = nullptr;

                    // Check if we can reuse existing texture wrapper
                    if (m_lastVkImage == vkImage && m_qtTextureWrapper && m_lastTextureSize.width() == texWidth &&
                        m_lastTextureSize.height() == texHeight) {
                        // Reuse existing wrapper for better performance
                        qtTexture = m_qtTextureWrapper;
                    } else {
                        // Create new wrapper
                        qtTexture = QNativeInterface::QSGVulkanTexture::fromNative(
                            vkImage,
                            imageLayout,
                            window(),
                            QSize(texWidth, texHeight),
                            QQuickWindow::TextureHasAlphaChannel);

                        if (qtTexture) {
                            // Store for reuse
                            m_qtTextureWrapper = qtTexture;
                            m_lastVkImage = vkImage;
                            m_lastTextureSize = QSize(texWidth, texHeight);
                        }
                    }

                    if (qtTexture) {
                        qtTexture->setFiltering(QSGTexture::Linear);
                        qtTexture->setMipmapFiltering(QSGTexture::None);
                        textureNode->setTexture(qtTexture);
                        textureNode->setRect(boundingRect());
                        textureNode->setOwnsTexture(false); // Don't delete - we manage it
                        return textureNode;
                    } else {
                    }
                } else {
                }
            } else {
                // Fallback: Try to get texture through legacy path
                void *nativeTex = m_map->nativeColorTexture();
                if (nativeTex) {
                }
            }
        } catch (const std::exception &e) {
            qWarning() << "Exception in zero-copy texture access:" << e.what();
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
        m_map->resize(QSize(static_cast<int>(newGeometry.width() * dpr), static_cast<int>(newGeometry.height() * dpr)));
        update();
    }
}

void MapLibreQuickItemVulkan::itemChange(ItemChange change, const ItemChangeData &data) {
    QQuickItem::itemChange(change, data);

    if (change == ItemSceneChange) {
        if (QQuickWindow *win = window()) {
            QObject::connect(
                win,
                &QQuickWindow::sceneGraphInitialized,
                this,
                [this]() {
                    if (!m_map) {
                        ensureMap(width(), height(), window()->devicePixelRatio());
                    }
                },
                Qt::DirectConnection);
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
        // Scale the delta by device pixel ratio since the map is rendered at DPR scale
        const qreal dpr = window() ? window()->devicePixelRatio() : 1.0;
        m_map->moveBy(delta * dpr);
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
