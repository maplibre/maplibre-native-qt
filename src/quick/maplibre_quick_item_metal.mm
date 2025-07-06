#if defined(__APPLE__)
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <AppKit/AppKit.h>
#include <CoreFoundation/CoreFoundation.h>
#endif

// MapLibreQuickItem Metal backend implementation for Apple platforms

#include "maplibre_quick_item_metal.hpp"

#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QSGNode>
#include <QSGSimpleTextureNode>
#include <QSGTexture>
#include <QtQuick/qsgtexture_platform.h>
#include <QDebug>

#include "utils/metal_renderer_backend.hpp"

using namespace QMapLibreQuick;

MapLibreQuickItemMetal::MapLibreQuickItemMetal() {
    qDebug() << "Creating MapLibreQuickItemMetal";
}

MapLibreQuickItemMetal::~MapLibreQuickItemMetal() {
#if defined(__APPLE__)
    if (m_currentDrawable) {
        CFRelease(m_currentDrawable);
    }
#endif
}

void MapLibreQuickItemMetal::initializeBackend() {
    qDebug() << "Initializing Metal backend";
    // Metal initialization specific code would go here
}

void MapLibreQuickItemMetal::cleanupBackend() {
    qDebug() << "Cleaning up Metal backend";
#if defined(__APPLE__)
    if (m_currentDrawable) {
        CFRelease(m_currentDrawable);
        m_currentDrawable = nullptr;
    }
    // Clean up Metal layer if we own it
    if (m_ownsLayer && m_layerPtr) {
        // Release the Metal layer
        m_layerPtr = nullptr;
        m_ownsLayer = false;
    }
#endif
}

QSGNode* MapLibreQuickItemMetal::renderFrame(QSGNode* oldNode) {
#if defined(__APPLE__)
    if (!m_map || !window()) {
        qDebug() << "Metal: Early return - Map:" << (m_map ? "exists" : "null") << "Window:" << (window() ? "exists" : "null");
        return oldNode;
    }

    qDebug() << "Metal: Continuing with rendering, map and window both exist";

    // Calculate actual render size with device pixel ratio
    const QSize windowSize(static_cast<int>(width()), static_cast<int>(height()));
    const float dpr = window() ? window()->devicePixelRatio() : 1.0f;
    const QSize renderSize(static_cast<int>(windowSize.width() * dpr),
                          static_cast<int>(windowSize.height() * dpr));

    qDebug() << "Metal: renderFrame: windowSize=" << windowSize << "renderSize=" << renderSize;

    // Create or update the scene graph node
    QSGSimpleTextureNode* node = static_cast<QSGSimpleTextureNode*>(oldNode);
    if (!node) {
        node = new QSGSimpleTextureNode();
        node->setFiltering(QSGTexture::Linear);
        qDebug() << "Metal: Created new QSGSimpleTextureNode";
    }

    // Metal-specific rendering implementation would go here
    // This is a placeholder implementation that creates a colored texture
    const QSize textureSize(static_cast<int>(width()), static_cast<int>(height()));
    if (textureSize.width() > 0 && textureSize.height() > 0) {
        QImage image(textureSize, QImage::Format_RGBA8888);
        image.fill(QColor(50, 150, 50, 255)); // Green background for Metal

        QPainter painter(&image);
        painter.setPen(Qt::white);
        painter.setFont(QFont("Arial", 14));
        painter.drawText(image.rect(), Qt::AlignCenter, "MapLibre Metal Backend\n(Placeholder Implementation)");

        auto texture = window()->createTextureFromImage(image);
        if (texture) {
            node->setTexture(texture);
            node->setOwnsTexture(true);
            node->setRect(boundingRect());
            node->markDirty(QSGNode::DirtyGeometry | QSGNode::DirtyMaterial);
        }
    }

    return node;
#else
    // Non-Apple platforms - return placeholder
    QSGSimpleTextureNode* node = static_cast<QSGSimpleTextureNode*>(oldNode);
    if (!node) {
        node = new QSGSimpleTextureNode();
        node->setFiltering(QSGTexture::Linear);
    }

    const QSize textureSize(static_cast<int>(width()), static_cast<int>(height()));
    if (textureSize.width() > 0 && textureSize.height() > 0) {
        QImage image(textureSize, QImage::Format_RGBA8888);
        image.fill(QColor(100, 100, 100, 255)); // Gray background

        QPainter painter(&image);
        painter.setPen(Qt::white);
        painter.setFont(QFont("Arial", 14));
        painter.drawText(image.rect(), Qt::AlignCenter, "Metal Backend\nNot Available\n(Non-Apple Platform)");

        auto texture = window()->createTextureFromImage(image);
        if (texture) {
            node->setTexture(texture);
            node->setOwnsTexture(true);
            node->setRect(boundingRect());
            node->markDirty(QSGNode::DirtyGeometry | QSGNode::DirtyMaterial);
        }
    }

    return node;
#endif
}
