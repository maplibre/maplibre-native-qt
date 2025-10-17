// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#include "rhi_widget.hpp"
#include "rhi_widget_p.hpp"

#include <QMapLibre/Map>
#include <QMapLibre/Settings>

#include <rhi/qrhi.h>
#include <QImage>
#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>

#if defined(MLN_RENDER_BACKEND_METAL) && defined(__APPLE__)
// Metal headers are included in the backend
#endif

#if defined(MLN_RENDER_BACKEND_OPENGL)
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#endif

#include <private/qrhi_p.h>
#include <rhi/qrhi_platform.h>
#include <QDebug>
#include <QtCore/QDebug>
#include <QtGui/QWindow>

#if defined(MLN_RENDER_BACKEND_VULKAN)
#include <private/qrhivulkan_p.h>
#endif

namespace QMapLibre {

RhiWidget::RhiWidget(const Settings &settings)
    : QRhiWidget() {
    // Constructor start

    // Set the graphics API FIRST, before anything else
#if defined(MLN_RENDER_BACKEND_VULKAN)
    setApi(QRhiWidget::Api::Vulkan);
    // Configured for Vulkan backend
#elif defined(MLN_RENDER_BACKEND_METAL)
    setApi(QRhiWidget::Api::Metal);
    // Configured for Metal backend
#elif defined(MLN_RENDER_BACKEND_OPENGL)
    setApi(QRhiWidget::Api::OpenGL);
    // Configured for OpenGL backend

#endif

    // Initialize the private data
    d_ptr = std::make_unique<RhiWidgetPrivate>(this, settings);

    // Enable auto render target
    setAutoRenderTarget(true);

    // Handle coordinate system differences between backends
    // When using zero-copy rendering with external textures, we don't need mirroring
#if defined(MLN_RENDER_BACKEND_METAL)
    setMirrorVertically(false);
#elif defined(MLN_RENDER_BACKEND_VULKAN)
    setMirrorVertically(false);
#elif defined(MLN_RENDER_BACKEND_OPENGL)
    // OpenGL with zero-copy also doesn't need mirroring
    setMirrorVertically(false);
#else
    // Default case
    setMirrorVertically(false);
#endif

    // Set size policy to expand
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumSize(100, 100);

    // Constructor end
}

RhiWidget::~RhiWidget() {
    qDebug() << "RhiWidget destructor called";
    // Clean up resources
    releaseResources();

    // Ensure the map is properly destroyed
    if (d_ptr && d_ptr->m_map) {
        d_ptr->m_map->destroyRenderer();
        d_ptr->m_map.reset();
    }
}

Map *RhiWidget::map() {
    return d_ptr->m_map.get();
}

Map *RhiWidget::mapInstance() {
    return d_ptr->m_map.get();
}

void RhiWidget::initialize(QRhiCommandBuffer *cb) {
    Q_UNUSED(cb)

    qDebug() << "RhiWidget::initialize called";

    // Check if we need to reinitialize after reparenting
    bool isReinitializing = d_ptr->m_map && !d_ptr->m_initialized;
    if (isReinitializing) {
        qDebug() << "Reinitializing map after reparenting";
        // The map exists but was uninitialized due to reparenting
        // We need to recreate the renderer with the new context
        d_ptr->m_map->destroyRenderer();

        // Reconnect signals that were disconnected in releaseResources()
        QObject::connect(d_ptr->m_map.get(), &Map::needsRendering, this, [this]() { update(); });

        QObject::connect(d_ptr->m_map.get(), &Map::mapChanged, this, [this](Map::MapChange change) {
            qDebug() << "Map changed event:" << static_cast<int>(change);
            if (d_ptr->m_map && d_ptr->m_map->isFullyLoaded()) {
                qDebug() << "Map is fully loaded!";
            }
            if (change == Map::MapChange::MapChangeDidFinishLoadingMap ||
                change == Map::MapChange::MapChangeDidFinishLoadingStyle ||
                change == Map::MapChange::MapChangeDidFinishRenderingFrameFullyRendered) {
                update();
            }
        });
    }

    // Initialize - checking QRhi and API type

    // Get the initial FBO binding for QRhiWidget (OpenGL only)

    // Initialize the map (or recreate renderer for existing map)
    if (!d_ptr->m_map) {
        d_ptr->m_map = std::make_unique<Map>(this, d_ptr->m_settings, QSize(width(), height()), devicePixelRatio());

        QObject::connect(d_ptr->m_map.get(), &Map::needsRendering, this, [this]() { update(); });

        // Connect to map changed signal to know when style is loaded
        QObject::connect(d_ptr->m_map.get(), &Map::mapChanged, this, [this](Map::MapChange change) {
            qDebug() << "Map changed event:" << static_cast<int>(change);
            // Check if map is fully loaded
            if (d_ptr->m_map && d_ptr->m_map->isFullyLoaded()) {
                qDebug() << "Map is fully loaded!";
            }
            // Force update on important map changes
            if (change == Map::MapChange::MapChangeDidFinishLoadingMap ||
                change == Map::MapChange::MapChangeDidFinishLoadingStyle ||
                change == Map::MapChange::MapChangeDidFinishRenderingFrameFullyRendered) {
                update();
            }
        });
    }

    // Create the renderer based on the build configuration and runtime API
    // This needs to happen both for initial creation and after reparenting
    bool needsRendererCreation = !d_ptr->m_initialized;

    if (needsRendererCreation) {
#if defined(MLN_RENDER_BACKEND_VULKAN)
        if (api() == QRhiWidget::Api::Vulkan) {
            // For Vulkan, we need to use Qt's Vulkan device for proper integration
            // Get the QRhi instance to access Vulkan resources
            QRhi *qrhi = rhi();
            if (qrhi && qrhi->backend() == QRhi::Vulkan) {
                // Get native Vulkan handles from Qt
                const QRhiNativeHandles *nativeHandles = qrhi->nativeHandles();
                if (nativeHandles) {
                    // Cast to Vulkan-specific handles
                    const QRhiVulkanNativeHandles *vkHandles = static_cast<const QRhiVulkanNativeHandles *>(
                        nativeHandles);

                    if (vkHandles && vkHandles->physDev && vkHandles->dev) {
                        qDebug() << "Using Qt's Vulkan device for MapLibre renderer";
                        qDebug() << "PhysDev:" << vkHandles->physDev << "Dev:" << vkHandles->dev
                                 << "Queue family:" << vkHandles->gfxQueueFamilyIdx;

                        // QRhiWidget doesn't have a window handle in the traditional sense
                        // We need to find the top-level window
                        QWindow *topWindow = nullptr;
                        QWidget *w = window();
                        if (w) {
                            topWindow = w->windowHandle();
                        }

                        if (topWindow && vkHandles->inst) {
                            qDebug() << "Window handle:" << topWindow << "VulkanInstance:" << vkHandles->inst;
                            // Set the Vulkan instance on the window if not already set
                            if (!topWindow->vulkanInstance()) {
                                topWindow->setVulkanInstance(vkHandles->inst);
                            }
                            // Create renderer with Qt's Vulkan device
                            // The createRendererWithQtVulkanDevice function uses device sharing
                            d_ptr->m_map->createRendererWithQtVulkanDevice(
                                topWindow,                   // Window for surface creation
                                vkHandles->physDev,          // Physical device
                                vkHandles->dev,              // Device
                                vkHandles->gfxQueueFamilyIdx // Graphics queue family index
                            );
                        } else {
                            qWarning() << "No valid window or Vulkan instance - falling back to standalone renderer";
                            d_ptr->m_map->createRenderer(nullptr);
                        }
                    } else {
                        qWarning() << "Failed to get Vulkan handles from QRhi";
                        d_ptr->m_map->createRenderer(nullptr);
                    }
                } else {
                    qWarning() << "No native handles available from QRhi";
                    d_ptr->m_map->createRenderer(nullptr);
                }
            } else {
                qWarning() << "QRhi is not using Vulkan backend";
                d_ptr->m_map->createRenderer(nullptr);
            }
        }
#elif defined(MLN_RENDER_BACKEND_METAL)
        if (api() == QRhiWidget::Api::Metal) {
            // Creating Metal renderer
            // For Metal with QRhiWidget, we use offscreen rendering
            // QRhiWidget manages its own Metal layer, so we create with nullptr
            qDebug() << "Creating Metal renderer for" << (isReinitializing ? "reinitialization" : "initial setup");
            d_ptr->m_map->createRenderer(nullptr);
            d_ptr->m_metalRendererCreated = true;
            // Metal renderer created (offscreen mode)
        }
#elif defined(MLN_RENDER_BACKEND_OPENGL)
        if (api() == QRhiWidget::Api::OpenGL) {
            // Creating OpenGL renderer
            d_ptr->m_map->createRenderer(nullptr);
        }
#else
        // Fallback if no matching backend was compiled in
        qWarning() << "RhiWidget: No matching renderer backend available for API" << static_cast<int>(api());
        d_ptr->m_map->createRenderer(nullptr);
#endif

        // Only set initial view and style on first creation, not on reinitialization
        if (!isReinitializing) {
            // Set default location with a reasonable zoom level
            auto coord = d_ptr->m_settings.defaultCoordinate();
            auto zoom = d_ptr->m_settings.defaultZoom();

            // If no default is set, use a reasonable default
            if (zoom < 1.0) {
                coord = Coordinate(40.7128, -74.0060); // New York
                zoom = 10.0;
            }

            qDebug() << "Setting initial map view - Coordinate:" << coord.first << "," << coord.second
                     << "Zoom:" << zoom;
            d_ptr->m_map->setCoordinateZoom(coord, zoom);

            // Set default style
            if (!d_ptr->m_settings.styles().empty()) {
                // Setting style URL
                qDebug() << "RhiWidget: Setting style URL:" << d_ptr->m_settings.styles().front().url;
                d_ptr->m_map->setStyleUrl(d_ptr->m_settings.styles().front().url);
            } else if (!d_ptr->m_settings.providerStyles().empty()) {
                // Setting provider style URL
                qDebug() << "RhiWidget: Setting provider style URL:" << d_ptr->m_settings.providerStyles().front().url;
                d_ptr->m_map->setStyleUrl(d_ptr->m_settings.providerStyles().front().url);
            } else {
                qWarning() << "RhiWidget: No style URL available - using default demo tiles";
                // Use a default style for testing
                d_ptr->m_map->setStyleUrl("https://demotiles.maplibre.org/style.json");
            }

            emit mapChanged(d_ptr->m_map.get());
        }

        // Force an initial render
        d_ptr->m_map->render();
        update();
    } else if (isReinitializing) {
        // After reinitializing from reparenting, force a render
        qDebug() << "Forcing render after reinitialization";
        d_ptr->m_map->render();
        update();
    }

    d_ptr->m_initialized = true;
}

void RhiWidget::render(QRhiCommandBuffer *cb) {
    // Render - checking initialization status
    // cb is Qt's command buffer - for Vulkan, Qt has already begun a render pass
    Q_UNUSED(cb)

    // CRITICAL: Check initialized flag FIRST before doing ANYTHING
    // This prevents race conditions during reparenting
    if (!d_ptr->m_initialized) {
        qDebug() << "RhiWidget::render skipped - not initialized";
        return;
    }

    if (!d_ptr->m_map) {
        qDebug() << "RhiWidget::render skipped - no map";
        return;
    }

    // Safety check: ensure we have a valid color texture
    QRhiTexture *rhiTexture = colorTexture();
    if (!rhiTexture) {
        qDebug() << "RhiWidget::render skipped - no color texture available";
        return;
    }

    // QRhiWidget manages its own framebuffer differently than QOpenGLWidget.
    // We need to ensure we're rendering to the right target based on the backend.

    // Handle backend-specific setup
#if defined(MLN_RENDER_BACKEND_OPENGL)
    if (api() == QRhiWidget::Api::OpenGL) {
        // For OpenGL, get QRhiWidget's color texture for zero-copy rendering
        if (rhiTexture) {
            // Get the native OpenGL texture handle
            QRhiTexture::NativeTexture nativeTex = rhiTexture->nativeTexture();

            // The texture object is the GLuint texture ID for OpenGL
            // nativeTex.object is a quint64, we need to convert it to GLuint
            GLuint glTextureId = static_cast<GLuint>(nativeTex.object);

            if (glTextureId != 0) {
                // Pass the OpenGL texture to MapLibre for zero-copy rendering
                // We need to set this every frame as the texture might change
                d_ptr->m_map->setOpenGLRenderTarget(glTextureId, rhiTexture->pixelSize());
            } else {
                qWarning() << "RhiWidget: OpenGL texture ID is 0";
            }
        } else {
            qWarning() << "RhiWidget: No color texture available from QRhiWidget";
        }
    }
#endif

#if defined(MLN_RENDER_BACKEND_VULKAN)
    if (api() == QRhiWidget::Api::Vulkan) {
        // For Vulkan, we need to pass the texture but be careful about render passes
        // Qt has begun its render pass, so we need to work within that context

        // Get QRhiWidget's color texture for rendering
        QRhiTexture *rhiTexture = colorTexture();
        if (rhiTexture) {
            // Get the native Vulkan image handle
            QRhiTexture::NativeTexture nativeTex = rhiTexture->nativeTexture();

            void *vulkanImagePtr = reinterpret_cast<void *>(nativeTex.object);

            if (vulkanImagePtr) {
                // Pass the Vulkan image to MapLibre
                // MapLibre will need to handle the fact that a render pass is already active
                d_ptr->m_map->setVulkanRenderTarget(vulkanImagePtr, rhiTexture->pixelSize());
            }
        }

        // Update size for Vulkan renderer
        d_ptr->m_map->updateRenderer(size(), devicePixelRatio(), 0);
    }
#endif

#if defined(MLN_RENDER_BACKEND_METAL)
    if (api() == QRhiWidget::Api::Metal) {
        // For Metal, pass QRhiWidget's color texture to MapLibre
        // This allows MapLibre to render directly to QRhiWidget's surface

        // Create renderer if not yet created (without layer for external texture mode)
        if (!d_ptr->m_metalRendererCreated && d_ptr->m_map) {
            d_ptr->m_map->createRenderer(nullptr);
            d_ptr->m_metalRendererCreated = true;
            // Created Metal renderer (external texture mode)
        }

        // Get QRhiWidget's color texture for direct rendering
        QRhiTexture *rhiColorTexture = colorTexture();
        if (rhiColorTexture) {
            // Get the native Metal texture handle
            QRhiTexture::NativeTexture nativeTex = rhiColorTexture->nativeTexture();
            void *metalTexturePtr = reinterpret_cast<void *>(nativeTex.object);

            if (metalTexturePtr) {
                // Setting Metal texture for MapLibre rendering
                d_ptr->m_map->setMetalRenderTarget(metalTexturePtr);
            } else {
                // Native Metal texture is null
            }
        } else {
            // No QRhiTexture available from colorTexture()
        }

        // Update size for Metal renderer
        if (d_ptr->m_map) {
            d_ptr->m_map->updateRenderer(size(), devicePixelRatio(), 0);
        }
    }
#endif

    // Render the map to its own texture first
    d_ptr->m_map->render();

#if defined(MLN_RENDER_BACKEND_METAL)
    if (api() == QRhiWidget::Api::Metal) {
        Q_UNUSED(cb);
    }
#endif

#if defined(MLN_RENDER_BACKEND_VULKAN)
    if (api() == QRhiWidget::Api::Vulkan) {
    }
#endif

    // Force Qt to redraw the widget to show the rendered content
    // Request continuous updates for animations
    update();
}

void RhiWidget::releaseResources() {
    qDebug() << "RhiWidget::releaseResources called";
    // Clean up resources when the widget is being destroyed or re-parented
    d_ptr->m_initialized = false;

    // Stop rendering immediately
    if (d_ptr->m_map) {
        // Disconnect signals to stop rendering
        d_ptr->m_map->disconnect();

#if defined(MLN_RENDER_BACKEND_OPENGL)
        // Only reset OpenGL resources if we have a valid context
        QRhi *rhiInstance = rhi();
        if (rhiInstance && rhiInstance->backend() == QRhi::OpenGLES2) {
            qDebug() << "Resetting OpenGL render target to 0";
            // Reset to default rendering (no external texture)
            d_ptr->m_map->setOpenGLRenderTarget(0, QSize());
        }
#endif

#if defined(MLN_RENDER_BACKEND_METAL)
        // For Metal, we MUST destroy the renderer during reparenting because
        // the Metal layer is tied to the old window context and becomes invalid
        qDebug() << "Destroying Metal renderer due to reparenting";
        d_ptr->m_map->destroyRenderer();
        d_ptr->m_metalRendererCreated = false;
#endif

        // For other backends, don't destroy the renderer during reparenting
        // The renderer will be recreated in initialize() if needed
    }
}

void RhiWidget::mousePressEvent(QMouseEvent *event) {
    d_ptr->handleMousePressEvent(event);
}

void RhiWidget::mouseMoveEvent(QMouseEvent *event) {
    d_ptr->handleMouseMoveEvent(event);
}

void RhiWidget::wheelEvent(QWheelEvent *event) {
    d_ptr->handleWheelEvent(event);
}

void RhiWidget::paintEvent(QPaintEvent *event) {
    // For Vulkan, we might need special handling
#if defined(MLN_RENDER_BACKEND_VULKAN)
    if (api() == QRhiWidget::Api::Vulkan && d_ptr->m_map) {
        // Get MapLibre's rendered content
        auto *vulkanTexture = d_ptr->m_map->getVulkanTexture();
        if (vulkanTexture) {
            // The texture has been rendered, Qt should display it
            // For now, just ensure we update
            update();
        }
    }
#endif

    // Call base class implementation
    QRhiWidget::paintEvent(event);
}

void RhiWidget::showEvent(QShowEvent *event) {
    // ShowEvent
    qDebug() << "RhiWidget::showEvent";
    QRhiWidget::showEvent(event);

    // Force a render when shown, especially important after undocking
    if (d_ptr->m_initialized && d_ptr->m_map) {
        qDebug() << "RhiWidget::showEvent - forcing map render";
        d_ptr->m_map->render();
    }

    update(); // Force an update
}

void RhiWidget::hideEvent(QHideEvent *event) {
    qDebug() << "RhiWidget::hideEvent";
    // When hiding, release resources to avoid using stale OpenGL resources
    releaseResources();
    QRhiWidget::hideEvent(event);
}

bool RhiWidget::event(QEvent *e) {
    // Handle parent change which happens during undocking
    if (e->type() == QEvent::ParentAboutToChange) {
        qDebug() << "RhiWidget::ParentAboutToChange - releasing resources";
        // Mark as uninitialized FIRST to prevent any rendering
        d_ptr->m_initialized = false;
        // Release resources before the parent changes
        releaseResources();
    } else if (e->type() == QEvent::ParentChange) {
        qDebug() << "RhiWidget::ParentChange - parent changed";
        // After parent change, we'll get a new initialize() call
        // Make sure we stay in uninitialized state
        d_ptr->m_initialized = false;
    }

    return QRhiWidget::event(e);
}

void RhiWidget::resizeEvent(QResizeEvent *event) {
    // ResizeEvent
    QRhiWidget::resizeEvent(event);

    if (d_ptr->m_map) {
        d_ptr->updateMapSize(size(), devicePixelRatio());
    }
}

// RhiWidgetPrivate implementation

RhiWidgetPrivate::RhiWidgetPrivate(QObject *parent, const Settings &settings)
    : QObject(parent),
      m_settings(settings) {}

RhiWidgetPrivate::~RhiWidgetPrivate() = default;

void RhiWidgetPrivate::updateMapSize(const QSize &size, qreal dpr) {
    if (!m_map) {
        return;
    }

    m_currentSize = size;
    m_currentDpr = dpr;

    m_map->resize(size);

    // Update the renderer with the new size
    m_map->updateRenderer(size, dpr, 0);
}

void RhiWidgetPrivate::handleMousePressEvent(QMouseEvent *event) {
    m_lastPos = event->position();

    if (event->button() == Qt::LeftButton && event->modifiers() & Qt::ShiftModifier) {
        m_map->pitchBy(60.0);
    }
}

void RhiWidgetPrivate::handleMouseMoveEvent(QMouseEvent *event) {
    if (!(event->buttons() & Qt::LeftButton)) {
        return;
    }

    QPointF delta = event->position() - m_lastPos;

    if (!delta.isNull()) {
        if (event->modifiers() & Qt::ControlModifier) {
            m_map->pitchBy(delta.y());
            m_map->rotateBy(m_lastPos, event->position());
        } else {
            m_map->moveBy(delta);
        }
    }

    m_lastPos = event->position();
}

void RhiWidgetPrivate::handleWheelEvent(QWheelEvent *event) const {
    if (!m_map) {
        return;
    }

    const int degrees = event->angleDelta().y() / 8;
    const int steps = degrees / 15;

    if (steps != 0) {
        const double factor = steps > 0 ? 2.0 : 0.5;
        m_map->scaleBy(factor, event->position());
    }

    event->accept();
}

} // namespace QMapLibre
