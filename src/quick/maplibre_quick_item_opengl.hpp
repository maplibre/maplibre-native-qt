#pragma once

#include <QtQml/qqmlregistration.h>
#include <qopengl.h>
#include <QMapLibre/Map>
#include <QQuickItem>
#include <QSGNode>
#include <memory>

namespace QMapLibreQuick {

/**
 * @brief OpenGL Quick item using QSGSimpleTextureNode for texture-based rendering
 */
class MapLibreQuickItemOpenGL : public QQuickItem {
    Q_OBJECT
    QML_NAMED_ELEMENT(MapLibreView)

public:
    MapLibreQuickItemOpenGL(QQuickItem *parent = nullptr);
    ~MapLibreQuickItemOpenGL() override;

protected:
    QSGNode *updatePaintNode(QSGNode *node, UpdatePaintNodeData *data) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void releaseResources() override;

public:
    QMapLibre::Map *getMap() const;

    // Mouse interaction
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void wheelEvent(QWheelEvent *) override;

private slots:
    void handleMapNeedsRendering();

private:
    void ensureMap(int w, int h, float dpr);

    std::unique_ptr<QMapLibre::Map> m_map;
    GLuint m_fbo{0};

    // interaction state
    QPointF m_lastMousePos;
    bool m_dragging{false};
};

// Type alias for the actual MapLibreQuickItem
using MapLibreQuickItem = MapLibreQuickItemOpenGL;

} // namespace QMapLibreQuick
