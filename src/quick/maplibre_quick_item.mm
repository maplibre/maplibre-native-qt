#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <AppKit/AppKit.h>
#include <CoreFoundation/CoreFoundation.h>
// MapLibreQuickItem implementation for Metal backend

#include "maplibre_quick_item.hpp"

#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QSGNode>
#include <QDebug>
#include <QSGSimpleTextureNode>
#include <QSGTexture>
#include <QtQuick/qsgtexture_platform.h>
#include <QTimer>
#include <QMouseEvent>
#include <QWheelEvent>

#include "utils/metal_renderer_backend.hpp"

// Need Metal layer declaration already included above

#include <QMapLibre/Map>
#include <QMapLibre/Settings>

using namespace QMapLibreQuick;
using namespace QMapLibre;

MapLibreQuickItem::MapLibreQuickItem() {
    setFlag(ItemHasContents, true);
}

void MapLibreQuickItem::releaseResources() {
    m_map.reset();
}

void MapLibreQuickItem::geometryChange(const QRectF &newG, const QRectF &oldG) {
    QQuickItem::geometryChange(newG, oldG);
    if (newG.size() != oldG.size()) {
        m_size = newG.size().toSize();
        if (m_map) {
            m_map->resize(QSize(m_size.width() * window()->devicePixelRatio(), m_size.height() * window()->devicePixelRatio()));
        }
        update();
    }
}

void MapLibreQuickItem::ensureMap(int w, int h, float dpr, void *metalLayer) {
    if (m_map)
        return;

    if (!metalLayer) {
        // Still no MetalLayer from Qt – create our own sublayer.
        NSView *view = (NSView *)window()->winId();
        if (![view wantsLayer]) {
            [view setWantsLayer:YES];
        }
        CAMetalLayer *newLayer = [CAMetalLayer layer];
        id<MTLDevice> dev = MTLCreateSystemDefaultDevice();
        newLayer.device = dev;
        newLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        newLayer.framebufferOnly = NO;
        newLayer.displaySyncEnabled = NO;
        if ([newLayer respondsToSelector:@selector(setAllowsNextDrawableTimeout:)])
            newLayer.allowsNextDrawableTimeout = NO;

        newLayer.frame = view.bounds;
        newLayer.drawableSize = CGSizeMake(w * dpr, h * dpr);
        [view.layer addSublayer:newLayer];
        metalLayer = (__bridge void *)newLayer;

        qDebug() << "[MapLibreQuick] Fallback: created CAMetalLayer sublayer";
        m_ownsLayer = true;
    }

    if (metalLayer) {
        m_layerPtr = metalLayer;
        // Treat the opaque pointer as the Objective-C runtime layer
        CAMetalLayer *layer = (__bridge CAMetalLayer *)metalLayer;

        id<MTLDevice> dev = MTLCreateSystemDefaultDevice();
        layer.device = dev;
        layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        layer.framebufferOnly = NO;
        layer.drawableSize = CGSizeMake(w * dpr, h * dpr);

        // Off-screen layer: disable vsync and drawable timeout so
        // nextDrawable() returns even when the layer is not presented
        // directly by Core Animation.
        if ([layer respondsToSelector:@selector(setDisplaySyncEnabled:)]) {
            layer.displaySyncEnabled = NO;
        }
        if ([layer respondsToSelector:@selector(setAllowsNextDrawableTimeout:)]) {
            layer.allowsNextDrawableTimeout = NO;
        }

        qDebug() << "Configured CAMetalLayer: size" << layer.drawableSize.width << "x" << layer.drawableSize.height
                 << ", device" << (void *)dev;

        // Defer binding the renderer until the first beforeRendering callback once a drawable is available.
    }

    // Map constructor takes size and DPR; renderer will pick Metal automatically
    m_map = std::make_unique<Map>(nullptr, Settings{}, QSize(w * dpr, h * dpr), dpr);
    // Renderer will be created lazily in beforeRendering.

    if (window()) {
        window()->setColor(Qt::transparent);
    }

    // First frame will be rendered from afterRendering once the MetalLayer is ready.

    m_map->setStyleUrl("https://demotiles.maplibre.org/style.json");
    m_map->setCoordinateZoom({40.7128, -74.0060}, 10);
}

QSGNode *MapLibreQuickItem::updatePaintNode(QSGNode *node, UpdatePaintNodeData *) {
    if (!window())
        return node;

    auto *ri = window()->rendererInterface();
    if (!ri) {
        qWarning() << "No rendererInterface";
        return node;
    }

    if (!m_map)
        ensureMap(width(), height(), window()->devicePixelRatio(), m_layerPtr);

    if (!m_connected) {
        QObject::connect(window(), &QQuickWindow::beforeRendering, this, [this]() {
            auto *ri = window()->rendererInterface();
            if (!ri)
                return;

            if (!m_layerPtr) {
                m_layerPtr = ri->getResource(window(), "MetalLayer");
                if (!m_layerPtr) {
                    qDebug() << "beforeRendering: MetalLayer still null, skip render";
                    return;
                }
                qDebug() << "beforeRendering: obtained MetalLayer" << m_layerPtr;
            }

            if (!m_map) {
                ensureMap(width(), height(), window()->devicePixelRatio(), m_layerPtr);
            }

            if (m_ownsLayer) {
                CAMetalLayer *layer = (__bridge CAMetalLayer *)m_layerPtr;
                id<CAMetalDrawable> drawable = [layer nextDrawable];
                if (!drawable) {
                    qDebug() << "fallback nextDrawable nil – skip frame";
                    return;
                }
                // Keep previous drawables until shutdown (leak guard). Not releasing avoids premature invalidation.
                m_currentDrawable = const_cast<void*>(CFRetain((__bridge CFTypeRef)drawable));
                // Lazily create renderer once we have a valid drawable
                if (!m_rendererBound) {
                    m_map->createRendererWithMetalLayer(m_layerPtr);
                    m_rendererBound = true;
                    qDebug() << "beforeRendering: bound renderer";
                }

                m_map->setCurrentDrawable((void *)drawable.texture);
            } else {
                if (!m_rendererBound) {
                    m_map->createRendererWithMetalLayer(m_layerPtr);
                    m_rendererBound = true;
                    qDebug() << "beforeRendering: bound renderer";
                }
                // Provide the current swap-chain texture from Qt
                void *qtTexPtr = ri->getResource(window(), "CurrentMetalTexture");
                if (qtTexPtr) {
                    m_map->setCurrentDrawable(qtTexPtr);
                }
            }

            window()->beginExternalCommands();
            m_map->render();
            window()->endExternalCommands();

            // Trigger a texture update in the SG
            QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection);
        }, Qt::DirectConnection);
        m_connected = true;
    }

    void *nativeTex = m_map ? m_map->nativeColorTexture() : nullptr;
    qDebug() << "nativeTex" << nativeTex;
    if (!nativeTex) {
        qDebug() << "nativeTex still nil – skip SG node update";
        if (node) {
            delete node;
        }
        return nullptr;
    }

    auto *textureNode = static_cast<QSGSimpleTextureNode *>(node);
    if (!textureNode) {
        textureNode = new QSGSimpleTextureNode();
        node = textureNode;
    }

    const int texWidth = width() * window()->devicePixelRatio();
    const int texHeight = height() * window()->devicePixelRatio();

    // Use Qt's native interface helper to wrap the Metal texture
    auto mtlTex = (__bridge id<MTLTexture>)nativeTex;
    QSGTexture *qtTex = QNativeInterface::QSGMetalTexture::fromNative(
        mtlTex,
        window(),
        QSize(texWidth, texHeight),
        QQuickWindow::TextureHasAlphaChannel);

    textureNode->setTexture(qtTex);
    textureNode->setRect(boundingRect());
    textureNode->setOwnsTexture(true);
    textureNode->markDirty(QSGNode::DirtyMaterial);
    return textureNode;
}

void MapLibreQuickItem::itemChange(ItemChange change, const ItemChangeData &data) {
    QQuickItem::itemChange(change, data);

    qDebug() << "itemChange:" << change;
    if (change == ItemSceneChange) {
        if (QQuickWindow *win = window()) {
            // Once the scene graph is ready we can obtain the Metal layer and create the map.
            QObject::connect(win, &QQuickWindow::sceneGraphInitialized, this, [this]() {
                if (m_map)
                    return;

                auto *ri = window()->rendererInterface();
                if (!ri) {
                    qWarning() << "rendererInterface still null";
                    return;
                }

                void *layerPtr = ri->getResource(window(), "MetalLayer");

                if (!layerPtr) {
                    qWarning() << "MetalLayer still not ready";
                    return;
                }

                ensureMap(width(), height(), window()->devicePixelRatio(), layerPtr);

                // First valid layer obtained – trigger update to paint.
                update();
            }, Qt::DirectConnection);
        }
    }
}

void MapLibreQuickItem::mousePressEvent(QMouseEvent *ev) {
    if (!m_map) {
        QQuickItem::mousePressEvent(ev);
        return;
    }
    if (ev->button() == Qt::LeftButton) {
        m_dragging = true;
        m_lastMousePos = ev->position();
        ev->accept();
    } else {
        QQuickItem::mousePressEvent(ev);
    }
}

void MapLibreQuickItem::mouseMoveEvent(QMouseEvent *ev) {
    if (m_dragging && m_map) {
        QPointF delta = ev->position() - m_lastMousePos;
        // Map::moveBy expects screen pixel delta; invert so drag direction feels natural
        m_map->moveBy(-delta);
        m_lastMousePos = ev->position();
        update();
        ev->accept();
    } else {
        QQuickItem::mouseMoveEvent(ev);
    }
}

void MapLibreQuickItem::mouseReleaseEvent(QMouseEvent *ev) {
    if (ev->button() == Qt::LeftButton && m_dragging) {
        m_dragging = false;
        ev->accept();
    } else {
        QQuickItem::mouseReleaseEvent(ev);
    }
}

void MapLibreQuickItem::wheelEvent(QWheelEvent *ev) {
    if (!m_map) {
        QQuickItem::wheelEvent(ev);
        return;
    }
    qreal angle = ev->angleDelta().y();
    if (angle == 0) {
        QQuickItem::wheelEvent(ev);
        return;
    }
    double factor = angle > 0 ? 1.2 : 0.8;
    m_map->scaleBy(factor, ev->position());
    update();
    ev->accept();
} 