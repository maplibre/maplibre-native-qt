// Copyright (C) 2023 MapLibre contributors
// SPDX-License-Identifier: BSD-2-Clause

#include "texture_node_metal_p.hpp"

#include <QtCore/QDebug>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGRendererInterface>

#if TARGET_OS_IPHONE
#import <UIKit/UIKit.h>
#else
#import <AppKit/AppKit.h>
#endif
#include <CoreFoundation/CoreFoundation.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

namespace {
constexpr int kMinimumSize{64};
} // namespace

namespace QMapLibre {

TextureNodeMetal::~TextureNodeMetal() {
    if (m_currentDrawable != nullptr) {
        CFRelease(m_currentDrawable);
    }
}

void TextureNodeMetal::resize(const QSize &size, qreal pixelRatio, QQuickWindow * /* window */) {
    m_size = size.expandedTo(QSize(kMinimumSize, kMinimumSize));
    m_pixelRatio = pixelRatio;

    qDebug() << "TextureNodeMetal::resize - size:" << m_size << "pixelRatio:" << m_pixelRatio
             << "physical size:" << (m_size * m_pixelRatio);

    // Pass logical size; mbgl::Map handles DPI scaling internally via pixelRatio passed at construction
    m_map->resize(m_size, m_pixelRatio);

    // Update Metal layer drawable size if we have one
    if (m_layerPtr != nullptr) {
        auto *layer = (__bridge CAMetalLayer *)m_layerPtr;
        layer.drawableSize = CGSizeMake(m_size.width() * m_pixelRatio, m_size.height() * m_pixelRatio);
    }
}

void TextureNodeMetal::render(QQuickWindow *window) {
    QSGRendererInterface *ri = window->rendererInterface();
    if (ri == nullptr) {
        qWarning() << "TextureNodeMetal: No renderer interface";
        return;
    }

    // Try to get Metal layer from Qt first
    if (m_layerPtr == nullptr) {
        m_layerPtr = ri->getResource(window, "MetalLayer");

        // If Qt doesn't provide a layer, create our own
        if (m_layerPtr == nullptr) {
#if TARGET_OS_IPHONE
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)
            auto *view = reinterpret_cast<UIView *>(window->winId());
#else
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)
            auto *view = reinterpret_cast<NSView *>(window->winId());
            if (![view wantsLayer]) {
                [view setWantsLayer:YES];
            }
#endif

            CAMetalLayer *newLayer = [CAMetalLayer layer];
            id<MTLDevice> dev = MTLCreateSystemDefaultDevice();
            newLayer.device = dev;
            newLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
            newLayer.framebufferOnly = NO;
#if !TARGET_OS_IPHONE
            newLayer.displaySyncEnabled = NO;
#endif
            if ([newLayer respondsToSelector:@selector(setAllowsNextDrawableTimeout:)]) {
                newLayer.allowsNextDrawableTimeout = NO;
            }

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

    if (m_layerPtr == nullptr) {
        qWarning() << "TextureNodeMetal: No Metal layer available";
        return;
    }

    // Configure layer if we have one
    auto *layer = (__bridge CAMetalLayer *)m_layerPtr;
    if (layer.device == nullptr) {
        id<MTLDevice> dev = MTLCreateSystemDefaultDevice();
        layer.device = dev;
        layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        layer.framebufferOnly = NO;
        layer.drawableSize = CGSizeMake(m_size.width() * m_pixelRatio, m_size.height() * m_pixelRatio);

        // Off-screen layer: disable vsync and drawable timeout
#if !TARGET_OS_IPHONE
        if ([layer respondsToSelector:@selector(setDisplaySyncEnabled:)]) {
            layer.displaySyncEnabled = NO;
        }
#endif
        if ([layer respondsToSelector:@selector(setAllowsNextDrawableTimeout:)]) {
            layer.allowsNextDrawableTimeout = NO;
        }
    }

    // Create renderer with Metal layer if not already created
    if (!m_rendererBound) {
        m_map->createRenderer(m_layerPtr);
        m_rendererBound = true;
    }

    // Handle drawable for owned layers
    if (m_ownsLayer) {
        id<CAMetalDrawable> drawable = [layer nextDrawable];
        if (drawable == nullptr) {
            qWarning() << "TextureNodeMetal: No drawable available";
            return;
        }
        // Keep reference to prevent premature release
        if (m_currentDrawable != nullptr) {
            CFRelease(m_currentDrawable);
        }
        m_currentDrawable = CFRetain((__bridge CFTypeRef)drawable);
        m_map->setCurrentDrawable((void *)drawable.texture);
    } else {
        // Use Qt's current swap-chain texture
        void *qtTexPtr = ri->getResource(window, "CurrentMetalTexture");
        if (qtTexPtr != nullptr) {
            m_map->setCurrentDrawable(qtTexPtr);
        }
    }

    // Begin external commands before MapLibre render
    window->beginExternalCommands();
    m_map->render();
    window->endExternalCommands();

    // Get the native texture from MapLibre
    void *nativeTex = m_map->nativeColorTexture();
    if (nativeTex == nullptr) {
        qWarning() << "TextureNodeMetal: No native texture available";
        return;
    }

    // Use Qt's native interface to wrap the Metal texture
    const int texWidth = static_cast<int>(m_size.width() * m_pixelRatio);
    const int texHeight = static_cast<int>(m_size.height() * m_pixelRatio);

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

    qDebug() << "Rendered TextureNodeMetal with size:" << m_size << "pixelRatio:" << m_pixelRatio
             << "drawable size:" << (m_size.width() * m_pixelRatio) << "x" << (m_size.height() * m_pixelRatio);
}

} // namespace QMapLibre
