#pragma once

#include <QtQml/qqmlregistration.h>
#include <QMapLibre/Map>
#include <QQuickItem>
#include <QString>
#include <memory>

namespace QMapLibre {
class Map;
}

namespace QMapLibreQuick {

/**
 * @brief Base class for MapLibre Quick item implementations
 *
 * This class contains the common interface and functionality shared
 * across all rendering backend implementations (OpenGL, Metal, Vulkan).
 */
class MapLibreQuickItemBase : public QQuickItem {
    Q_OBJECT

public:
    MapLibreQuickItemBase();
    virtual ~MapLibreQuickItemBase() = default;

protected:
    // Common interface that all backends must implement
    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void releaseResources() override;
    void itemChange(ItemChange change, const ItemChangeData &data) override;

    // Mouse interaction - common to all backends
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void wheelEvent(QWheelEvent *) override;

    // Backend-specific virtual methods
    virtual QSGNode *renderFrame(QSGNode *oldNode) = 0;
    virtual void initializeBackend() = 0;
    virtual void cleanupBackend() = 0;

    // Common helper methods
    void ensureMap(int w, int h, float dpr, void *backendSpecificData = nullptr);

    // Common member variables
    std::unique_ptr<QMapLibre::Map> m_map;
    QSize m_size;
    bool m_connected{false};
    bool m_rendererBound = false;

    // Interaction state
    QPointF m_lastMousePos;
    bool m_dragging{false};
};

} // namespace QMapLibreQuick
