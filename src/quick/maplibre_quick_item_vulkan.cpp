// MapLibreQuickItem Vulkan backend implementation

#include "maplibre_quick_item_vulkan.hpp"

#include <QDebug>
#include <QPainter>
#include <QQuickWindow>
#include <QSGNode>
#include <QSGRendererInterface>
#include <QSGSimpleTextureNode>
#include <QSGTexture>

#include "utils/vulkan_renderer_backend.hpp"

using namespace QMapLibreQuick;

MapLibreQuickItemVulkan::MapLibreQuickItemVulkan() {
    qDebug() << "Creating MapLibreQuickItemVulkan";
}

void MapLibreQuickItemVulkan::initializeBackend() {
    qDebug() << "Initializing Vulkan backend";
    // Vulkan initialization specific code would go here
}

void MapLibreQuickItemVulkan::cleanupBackend() {
    qDebug() << "Cleaning up Vulkan backend";
    // Vulkan cleanup code would go here
}

QSGNode* MapLibreQuickItemVulkan::renderFrame(QSGNode* oldNode) {
    if (!m_map || !window()) {
        qDebug() << "Vulkan: Early return - Map:" << (m_map ? "exists" : "null")
                 << "Window:" << (window() ? "exists" : "null");
        return oldNode;
    }

    qDebug() << "Vulkan: Continuing with rendering, map and window both exist";

    // Calculate actual render size with device pixel ratio
    const QSize windowSize(static_cast<int>(width()), static_cast<int>(height()));
    const float dpr = window() ? window()->devicePixelRatio() : 1.0f;
    const QSize renderSize(static_cast<int>(windowSize.width() * dpr), static_cast<int>(windowSize.height() * dpr));

    qDebug() << "Vulkan: renderFrame: windowSize=" << windowSize << "renderSize=" << renderSize;

    // Create or update the scene graph node
    QSGSimpleTextureNode* node = static_cast<QSGSimpleTextureNode*>(oldNode);
    if (!node) {
        node = new QSGSimpleTextureNode();
        node->setFiltering(QSGTexture::Linear);
        qDebug() << "Vulkan: Created new QSGSimpleTextureNode";
    }

    // Vulkan-specific rendering implementation would go here
    // This is a placeholder implementation that creates a colored texture
    const QSize textureSize(static_cast<int>(width()), static_cast<int>(height()));
    if (textureSize.width() > 0 && textureSize.height() > 0) {
        QImage image(textureSize, QImage::Format_RGBA8888);
        image.fill(QColor(50, 50, 150, 255)); // Blue background for Vulkan

        QPainter painter(&image);
        painter.setPen(Qt::white);
        painter.setFont(QFont("Arial", 14));
        painter.drawText(image.rect(), Qt::AlignCenter, "MapLibre Vulkan Backend\n(Placeholder Implementation)");

        auto texture = window()->createTextureFromImage(image);
        if (texture) {
            node->setTexture(texture);
            node->setOwnsTexture(true);
            node->setRect(boundingRect());
            node->markDirty(QSGNode::DirtyGeometry | QSGNode::DirtyMaterial);
        }
    }

    return node;
}
