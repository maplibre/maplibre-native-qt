#pragma once

#include <QtQml/qqmlregistration.h>
#include <QMapLibre/Map>
#include <QQuickItem>
#include <QSGTexture>
#include <memory>

#if QT_CONFIG(vulkan)
#include <vulkan/vulkan.h>
#endif

namespace QMapLibre {
class Map;
}

namespace QMapLibreQuick {

/**
 * @brief Vulkan backend implementation for MapLibre Quick item
 */
class MapLibreQuickItemVulkan : public QQuickItem {
    Q_OBJECT
    QML_NAMED_ELEMENT(MapLibreView)

public:
    MapLibreQuickItemVulkan();
    ~MapLibreQuickItemVulkan() override;

protected:
    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void releaseResources() override;
    void itemChange(ItemChange change, const ItemChangeData &data) override;

    // Mouse interaction
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void wheelEvent(QWheelEvent *) override;

private:
    void ensureMap(int w, int h, float dpr);
    void setupVulkanContext(QQuickWindow *quickWindow);

    std::unique_ptr<QMapLibre::Map> m_map;
    QSize m_size;
    bool m_connected = false;
    bool m_rendererBound = false;
    QMetaObject::Connection m_renderConnection;

    // interaction state
    QPointF m_lastMousePos;
    bool m_dragging{false};

    // Zero-copy texture caching
    QSGTexture *m_qtTextureWrapper{nullptr};
    VkImage m_lastVkImage{VK_NULL_HANDLE};
    QSize m_lastTextureSize;
};

// Type alias for the actual MapLibreQuickItem when using Vulkan
using MapLibreQuickItem = MapLibreQuickItemVulkan;

} // namespace QMapLibreQuick
