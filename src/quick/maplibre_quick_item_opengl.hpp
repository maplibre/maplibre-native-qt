#pragma once

#include <QtQml/qqmlregistration.h>
#include <QMapLibre/Map>
#include <QQuickFramebufferObject>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QString>
#include <QSize>
#include <QPointF>
#include <memory>

namespace QMapLibre {
class Map;
}

namespace QMapLibreQuick {

/**
 * @brief OpenGL backend implementation for MapLibre Quick item using QQuickFramebufferObject
 */
class MapLibreQuickItemOpenGL : public QQuickFramebufferObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(MapLibreView)
    
public:
    MapLibreQuickItemOpenGL();
    ~MapLibreQuickItemOpenGL() override = default;

    // QQuickFramebufferObject interface
    Renderer* createRenderer() const override;

protected:
    // Mouse interaction
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void wheelEvent(QWheelEvent *) override;

private:
    void ensureMap(int w, int h, float dpr);

    mutable std::unique_ptr<QMapLibre::Map> m_map;
    QSize m_size;
    bool m_connected{false};

    // Interaction state
    QPointF m_lastMousePos;
    bool m_dragging{false};

    friend class MapLibreRenderer;
};

// Type alias for the actual MapLibreQuickItem
using MapLibreQuickItem = MapLibreQuickItemOpenGL;

} // namespace QMapLibreQuick
