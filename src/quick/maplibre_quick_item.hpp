#pragma once

#include <QQuickItem>
#include <memory>
#include <QMapLibre/Map>
#include <QtQml/qqmlregistration.h>
#include <CoreFoundation/CoreFoundation.h>

namespace QMapLibre {
class Map;
}

namespace QMapLibreQuick {
class MapLibreQuickItem : public QQuickItem {
    Q_OBJECT
    QML_NAMED_ELEMENT(MapLibreView)
public:
    MapLibreQuickItem();
    ~MapLibreQuickItem() override {
        if (m_currentDrawable) {
            CFRelease(m_currentDrawable);
        }
    }

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
    void ensureMap(int w, int h, float dpr, void *metalLayer);

    std::unique_ptr<QMapLibre::Map> m_map;
    QSize  m_size;
    bool   m_connected{false};
    bool   m_rendererBound = false;
    void *m_layerPtr = nullptr; // persistent CAMetalLayer pointer we pass to MapLibre
    bool   m_ownsLayer = false; // true when we created the fallback CAMetalLayer
    void  *m_currentDrawable = nullptr; // retained CAMetalDrawable until frame ends

    // interaction state
    QPointF m_lastMousePos;
    bool    m_dragging{false};
};
} 