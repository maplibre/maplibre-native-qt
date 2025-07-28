// Copyright (C) 2024 MapLibre contributors
// SPDX-License-Identifier: BSD-2-Clause

#import <AppKit/AppKit.h>
#include <CoreFoundation/CoreFoundation.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include <QtQuick/qsgtexture_platform.h>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QtCore/QDebug>
#include "texture_node_metal.hpp"

namespace QMapLibre {

TextureNodeMetal::TextureNodeMetal(const Settings &settings,
                                   const QSize &size,
                                   qreal pixelRatio,
                                   QGeoMapMapLibre *geoMap)
    : TextureNodeBase(settings, size, pixelRatio, geoMap) {}

TextureNodeMetal::~TextureNodeMetal() {
    if (m_currentDrawable) {
        CFRelease(m_currentDrawable);
    }
}

void TextureNodeMetal::resize(const QSize &size, qreal pixelRatio, QQuickWindow * /* window */) {
    m_size = size.expandedTo(QSize(64, 64));
    m_pixelRatio = pixelRatio;

    qDebug() << "TextureNodeMetal::resize - size:" << m_size << "pixelRatio:" << m_pixelRatio
             << "physical size:" << (m_size * m_pixelRatio);

    // Pass logical size; mbgl::Map handles DPI scaling internally via pixelRatio passed at construction
    m_map->resize(m_size);

    // Update Metal layer drawable size if we have one
    if (m_layerPtr) {
        CAMetalLayer *layer = (__bridge CAMetalLayer *)m_layerPtr;
        layer.drawableSize = CGSizeMake(m_size.width() * m_pixelRatio, m_size.height() * m_pixelRatio);
    }
}

void TextureNodeMetal::render(QQuickWindow *window) {
    auto *ri = window->rendererInterface();
    if (!ri) {
        qWarning() << "TextureNodeMetal: No renderer interface";
        return;
    }

    // Try to get Metal layer from Qt first
    if (!m_layerPtr) {
        m_layerPtr = ri->getResource(window, "MetalLayer");

        // If Qt doesn't provide a layer, create our own
        if (!m_layerPtr) {
            NSView *view = (NSView *)window->winId();
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

            // Set the layer frame to match our texture size (in points)
            newLayer.frame = CGRectMake(0, 0, m_size.width(), m_size.height());
            newLayer.drawableSize = CGSizeMake(m_size.width() * m_pixelRatio, m_size.height() * m_pixelRatio);
            [view.layer addSublayer:newLayer];
            m_layerPtr = (__bridge void *)newLayer;
            m_ownsLayer = true;

            qDebug() << "TextureNodeMetal: Created Metal layer with frame:" << m_size.width() << "x" << m_size.height()
                     << "drawable size:" << (m_size.width() * m_pixelRatio) << "x" << (m_size.height() * m_pixelRatio);
        }
    }

    if (!m_layerPtr) {
        qWarning() << "TextureNodeMetal: No Metal layer available";
        return;
    }

    // Configure layer if we have one
    CAMetalLayer *layer = (__bridge CAMetalLayer *)m_layerPtr;
    if (!layer.device) {
        id<MTLDevice> dev = MTLCreateSystemDefaultDevice();
        layer.device = dev;
        layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        layer.framebufferOnly = NO;
        layer.drawableSize = CGSizeMake(m_size.width() * m_pixelRatio, m_size.height() * m_pixelRatio);

        // Off-screen layer: disable vsync and drawable timeout
        if ([layer respondsToSelector:@selector(setDisplaySyncEnabled:)]) {
            layer.displaySyncEnabled = NO;
        }
        if ([layer respondsToSelector:@selector(setAllowsNextDrawableTimeout:)]) {
            layer.allowsNextDrawableTimeout = NO;
        }
    }

    // Create renderer with Metal layer if not already created
    if (!m_rendererBound) {
        m_map->createRendererWithMetalLayer(m_layerPtr);
        m_rendererBound = true;
    }

    // Handle drawable for owned layers
    if (m_ownsLayer) {
        id<CAMetalDrawable> drawable = [layer nextDrawable];
        if (!drawable) {
            qWarning() << "TextureNodeMetal: No drawable available";
            return;
        }
        // Keep reference to prevent premature release
        if (m_currentDrawable) {
            CFRelease(m_currentDrawable);
        }
        m_currentDrawable = const_cast<void *>(CFRetain((__bridge CFTypeRef)drawable));
        m_map->setCurrentDrawable((void *)drawable.texture);
    } else {
        // Use Qt's current swap-chain texture
        void *qtTexPtr = ri->getResource(window, "CurrentMetalTexture");
        if (qtTexPtr) {
            m_map->setCurrentDrawable(qtTexPtr);
        }
    }

    // Begin external commands before MapLibre render
    window->beginExternalCommands();
    m_map->render();
    window->endExternalCommands();

    // Get the native texture from MapLibre
    void *nativeTex = m_map->nativeColorTexture();
    if (!nativeTex) {
        qWarning() << "TextureNodeMetal: No native texture available";
        return;
    }

    // Use Qt's native interface to wrap the Metal texture
    const int texWidth = m_size.width() * m_pixelRatio;
    const int texHeight = m_size.height() * m_pixelRatio;

    auto mtlTex = (__bridge id<MTLTexture>)nativeTex;

    // Debug: check actual Metal texture dimensions
    qDebug() << "TextureNodeMetal: Metal texture actual size:" << mtlTex.width << "x" << mtlTex.height
             << "expected:" << texWidth << "x" << texHeight;

    QSGTexture *qtTex = QNativeInterface::QSGMetalTexture::fromNative(
        mtlTex, window, QSize(texWidth, texHeight), QQuickWindow::TextureHasAlphaChannel);

    setTexture(qtTex);
    setOwnsTexture(true);

    // Set the texture rect to match the viewport size
    // For Metal, flip the rect vertically to correct orientation
    setRect(QRectF(0, m_size.height(), m_size.width(), -m_size.height()));

    markDirty(QSGNode::DirtyMaterial | QSGNode::DirtyGeometry);
}

} // namespace QMapLibre
