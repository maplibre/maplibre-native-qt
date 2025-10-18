// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#include "map_widget.hpp"
#include "map_widget_p.hpp"

#include <QMapLibre/Map>
#include <QMapLibre/Settings>

#include <QtGui/rhi/qrhi.h>
#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtGui/QMouseEvent>
#include <QtGui/QWheelEvent>
#include <QtGui/QWindow>

#ifdef MLN_RENDER_BACKEND_OPENGL
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>
#endif

#ifdef MLN_RENDER_BACKEND_VULKAN
#include <QtGui/private/qrhivulkan_p.h>
#endif

namespace {
constexpr int MinimumSize = 64;
} // namespace

namespace QMapLibre {

/*!
    \defgroup QMapLibreWidgets QMapLibre Widgets
    \brief Qt Widgets for MapLibre.
*/

/*!
    \class MapWidget
    \brief A QRhiWidget-based MapLibre map widget with cross-platform rendering support.
    \ingroup QMapLibreWidgets

    \headerfile map_widget.hpp <QMapLibreWidgets/MapWidget>

    MapWidget provides hardware-accelerated map rendering using
    Qt's RHI (Rendering Hardware Interface), which abstracts over different
    graphics APIs (OpenGL, Vulkan, Metal).

    The widget is intended as a standalone map viewer in a Qt Widgets application.
    It owns its own instance of \ref QMapLibre::Map that is rendered to the widget.

    \fn MapWidget::onMouseDoubleClickEvent
    \brief Emitted when the user double-clicks the mouse.

    \fn MapWidget::onMouseMoveEvent
    \brief Emitted when the user moves the mouse.

    \fn MapWidget::onMousePressEvent
    \brief Emitted when the user presses the mouse.

    \fn MapWidget::onMouseReleaseEvent
    \brief Emitted when the user releases the mouse.
*/
MapWidget::MapWidget(const Settings &settings) {
    // Set the graphics API first, before anything else
#ifdef MLN_RENDER_BACKEND_VULKAN
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
    d_ptr = std::make_unique<MapWidgetPrivate>(this, settings);

    // Enable auto render target
    setAutoRenderTarget(true);

    // Handle coordinate system differences between backends
    // When using zero-copy rendering with external textures, we don't need mirroring
    setMirrorVertically(false);

    // Set size policy to expand
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // Set default minimum size
    setMinimumSize(MinimumSize, MinimumSize);
}

MapWidget::~MapWidget() {
    // Mark as not initialized
    if (d_ptr != nullptr) {
        d_ptr->m_initialized = false;
    }

    // Ensure the map is properly destroyed
    if (d_ptr != nullptr && d_ptr->m_map != nullptr) {
        d_ptr->m_map->destroyRenderer();
        d_ptr->m_map.reset();
    }
}

/*!
    \brief Get the QMapLibre::Map instance.
*/
Map *MapWidget::map() {
    return d_ptr->m_map.get();
}

/*!
    \brief Handle map change events.
*/
void MapWidget::handleMapChange(Map::MapChange change) {
    if (change == Map::MapChangeDidFinishLoadingMap) {
        constexpr int refreshInterval{250};
        // TODO: make it more elegant
        QTimer::singleShot(refreshInterval, this, qOverload<>(&MapWidget::update));
    }
}

/*!
    \brief Mouse press event handler.
*/
void MapWidget::mousePressEvent(QMouseEvent *event) {
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
void MapWidget::mouseReleaseEvent(QMouseEvent *event) {
    const QPointF &position = event->position();
    emit onMouseReleaseEvent(d_ptr->m_map->coordinateForPixel(position));
}

/*!
    \brief Mouse move event handler.
*/
void MapWidget::mouseMoveEvent(QMouseEvent *event) {
    const QPointF &position = event->position();
    emit onMouseMoveEvent(d_ptr->m_map->coordinateForPixel(position));

    d_ptr->handleMouseMoveEvent(event);
}

/*!
    \brief Mouse wheel event handler.
*/
void MapWidget::wheelEvent(QWheelEvent *event) {
    d_ptr->handleWheelEvent(event);
}

/*!
    \brief Widget resize event handler.
*/
void MapWidget::resizeEvent(QResizeEvent *event) {
    QRhiWidget::resizeEvent(event);

    if (d_ptr->m_map != nullptr) {
        d_ptr->m_map->resize(size(), devicePixelRatio());
    }
}

/*!
    \brief Widget main event handler.
*/
bool MapWidget::event(QEvent *e) {
    // Handle parent change which happens during undocking
    if (e->type() == QEvent::ParentAboutToChange) {
#ifdef MLN_RENDERER_DEBUGGING
        qDebug() << "MapWidget::event() - ParentAboutToChange - releasing resources";
#endif
        // Mark as uninitialized FIRST to prevent any rendering
        d_ptr->m_initialized = false;
        // Release resources before the parent changes
        releaseResources();
    } else if (e->type() == QEvent::ParentChange) {
#ifdef MLN_RENDERER_DEBUGGING
        qDebug() << "MapWidget::event() - ParentChange";
#endif
        // After parent change, we'll get a new initialize() call
        // Make sure we stay in uninitialized state
        d_ptr->m_initialized = false;
    }

    return QRhiWidget::event(e);
}

void MapWidget::initialize(QRhiCommandBuffer *cb) {
    Q_UNUSED(cb)

#ifdef MLN_RENDERER_DEBUGGING
    qDebug() << "MapWidget::initialize()";
#endif

    if (d_ptr->m_initialized) {
#ifdef MLN_RENDERER_DEBUGGING
        qDebug() << "MapWidget::initialize() - Already initialized, nothing to do";
#endif
        d_ptr->m_map->resize(size(), devicePixelRatio());
        return;
    }

    // Check if we need to reinitialize after reparenting
    const bool isReinitializing = !d_ptr->m_initialized && d_ptr->m_map != nullptr;

    // Initialize the map
    if (d_ptr->m_map == nullptr) {
        d_ptr->m_map = std::make_unique<Map>(this, d_ptr->m_settings, QSize(width(), height()), devicePixelRatio());
        // Connect to needsRendering signal to trigger updates
        QObject::connect(d_ptr->m_map.get(), &Map::needsRendering, this, qOverload<>(&MapWidget::update));
        // Connect to map changed signal to know when the map is loaded
        QObject::connect(d_ptr->m_map.get(), &Map::mapChanged, this, &MapWidget::handleMapChange);
    }

    // Create the renderer based on the build configuration and runtime API
    // This needs to happen both for initial creation and after reparenting
    if (!d_ptr->m_initialized) {
#if defined(MLN_RENDER_BACKEND_OPENGL)
#ifdef MLN_RENDERER_DEBUGGING
        qDebug() << "MapWidget::initialize() - Creating OpenGL renderer for"
                 << (isReinitializing ? "reinitialization" : "initial setup");
#endif
        d_ptr->m_map->createRenderer(nullptr);
#elif defined(MLN_RENDER_BACKEND_VULKAN)
#ifdef MLN_RENDERER_DEBUGGING
        qDebug() << "MapWidget::initialize() - Creating Vulkan renderer for"
                 << (isReinitializing ? "reinitialization" : "initial setup");
#endif
        // For Vulkan, we need to use Qt's Vulkan device for proper integration
        if (rhi() != nullptr && rhi()->backend() == QRhi::Vulkan) {
            // Get native Vulkan handles from Qt
            const QRhiNativeHandles *nativeHandles = rhi()->nativeHandles();
            if (nativeHandles != nullptr) {
                // Cast to Vulkan-specific handles
                const auto *vkHandles = static_cast<const QRhiVulkanNativeHandles *>(nativeHandles);
                if (vkHandles != nullptr && vkHandles->physDev != nullptr && vkHandles->dev != nullptr) {
#ifdef MLN_RENDERER_DEBUGGING
                    qDebug() << "MapWidget::initialize() - Using Qt's Vulkan device for MapLibre renderer";
                    qDebug() << "MapWidget::initialize() - PhysDev:" << vkHandles->physDev << "Dev:" << vkHandles->dev
                             << "Queue family:" << vkHandles->gfxQueueFamilyIdx;
#endif

                    // QRhiWidget doesn't have a window handle in the traditional sense
                    // We need to find the top-level window
                    QWindow *topWindow = window() != nullptr ? window()->windowHandle() : nullptr;
                    if (topWindow != nullptr && vkHandles->inst != nullptr) {
#ifdef MLN_RENDERER_DEBUGGING
                        qDebug() << "MapWidget::initialize() - Window handle:" << topWindow
                                 << "VulkanInstance:" << vkHandles->inst;
#endif
                        // Set the Vulkan instance on the window if not already set
                        if (topWindow->vulkanInstance() == nullptr) {
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
                        qWarning() << "MapWidget::initialize() - No valid window or Vulkan instance, falling back to "
                                      "standalone renderer";
                        d_ptr->m_map->createRenderer(nullptr);
                    }
                } else {
                    qWarning() << "MapWidget::initialize() - Failed to get Vulkan handles from QRhi";
                    d_ptr->m_map->createRenderer(nullptr);
                }
            } else {
                qWarning() << "MapWidget::initialize() - No native handles available from QRhi";
                d_ptr->m_map->createRenderer(nullptr);
            }
        } else {
            qWarning() << "MapWidget::initialize() - QRhi is not using Vulkan backend";
            d_ptr->m_map->createRenderer(nullptr);
        }
#elif defined(MLN_RENDER_BACKEND_METAL)
#ifdef MLN_RENDERER_DEBUGGING
        qDebug() << "MapWidget::initialize() - Creating Metal renderer for"
                 << (isReinitializing ? "reinitialization" : "initial setup");
#endif
        // For Metal with QRhiWidget, we use offscreen rendering
        // QRhiWidget manages its own Metal layer, so we create with nullptr
        d_ptr->m_map->createRenderer(nullptr);
#endif

        // Only set initial view and style on first creation, not on reinitialization
        if (!isReinitializing) {
            d_ptr->m_map->setCoordinateZoom(d_ptr->m_settings.defaultCoordinate(), d_ptr->m_settings.defaultZoom());

            // Set default style
            if (!d_ptr->m_settings.styles().empty()) {
                d_ptr->m_map->setStyleUrl(d_ptr->m_settings.styles().front().url);
            } else if (!d_ptr->m_settings.providerStyles().empty()) {
                d_ptr->m_map->setStyleUrl(d_ptr->m_settings.providerStyles().front().url);
            }
        }
    }

    d_ptr->m_initialized = true;

    // Make sure that the map gets repainted after initialization if all data is already loaded
    d_ptr->m_map->triggerRepaint();
}

void MapWidget::render(QRhiCommandBuffer *cb) {
    // Render - checking initialization status
    Q_UNUSED(cb)

    // Only render if initialized
    // This prevents race conditions during reparenting
    if (!d_ptr->m_initialized) {
#ifdef MLN_RENDERER_DEBUGGING
        qDebug() << "MapWidget::render() - skipped, not initialized";
#endif
        return;
    }

    // Get QRhiWidget's color texture for rendering
    QRhiTexture *rhiTexture = colorTexture();
    if (rhiTexture == nullptr) {
#ifdef MLN_RENDERER_DEBUGGING
        qDebug() << "MapWidget::render() - skipped, no color texture available";
#endif
        return;
    }

#ifdef MLN_RENDERER_DEBUGGING
    qDebug() << "MapWidget::render()";
#endif

    // Get the native texture handle
    const QRhiTexture::NativeTexture nativeTexture = rhiTexture->nativeTexture();

    // Handle backend-specific setup
#ifdef MLN_RENDER_BACKEND_OPENGL
    // The texture object is the GLuint texture ID for OpenGL
    // nativeTexture.object is a quint64, we need to convert it to GLuint
    auto glTextureId = static_cast<GLuint>(nativeTexture.object);

    if (glTextureId != 0) {
        // Pass the OpenGL texture to MapLibre for zero-copy rendering
        // We need to set this every frame as the texture might change
        d_ptr->m_map->setExternalDrawable(&glTextureId, rhiTexture->pixelSize());
    } else {
        qWarning() << "MapWidget::render() - OpenGL texture ID is 0";
    }
#elif defined(MLN_RENDER_BACKEND_VULKAN)
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)
    auto *vulkanImagePtr = reinterpret_cast<void *>(nativeTexture.object);
    if (vulkanImagePtr != nullptr) {
        // Pass the OpenGL texture to MapLibre for zero-copy rendering
        // We need to set this every frame as the texture might change
        d_ptr->m_map->setExternalDrawable(vulkanImagePtr, rhiTexture->pixelSize());
    }
#elif defined(MLN_RENDER_BACKEND_METAL)
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)
    auto *metalTexturePtr = reinterpret_cast<void *>(nativeTexture.object);
    if (metalTexturePtr != nullptr) {
        // Pass the Metal texture to MapLibre for zero-copy rendering
        // We need to set this every frame as the texture might change
        d_ptr->m_map->setExternalDrawable(metalTexturePtr, rhiTexture->pixelSize());
    }
#endif

    // Render the map to its own texture
    d_ptr->m_map->render();
}

void MapWidget::releaseResources() {
#ifdef MLN_RENDERER_DEBUGGING
    qDebug() << "MapWidget::releaseResources()";
#endif

    // Clean up resources when the widget is being destroyed or re-parented
    d_ptr->m_initialized = false;

    if (d_ptr->m_map == nullptr) {
        return;
    }

#ifdef MLN_RENDERER_DEBUGGING
    qDebug() << "MapWidget::releaseResources() - Destroying renderer due to reparenting";
#endif
    d_ptr->m_map->destroyRenderer();
}

/*! \cond PRIVATE */

MapWidgetPrivate::MapWidgetPrivate(QObject *parent, Settings settings)
    : QObject(parent),
      m_settings(std::move(settings)) {}

MapWidgetPrivate::~MapWidgetPrivate() = default;

void MapWidgetPrivate::handleMousePressEvent(QMouseEvent *event) {
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

void MapWidgetPrivate::handleMouseMoveEvent(QMouseEvent *event) {
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

void MapWidgetPrivate::handleWheelEvent(QWheelEvent *event) const {
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

/*! \endcond PRIVATE */

} // namespace QMapLibre
