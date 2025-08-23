#pragma once

#include <QMapLibre/Settings>

#include <QtQuick/QQuickItem>
#include <QtQuick/QSGNode>

namespace QMapLibre {

class Map;

class MapQuickItem : public QQuickItem {
    Q_OBJECT
    QML_NAMED_ELEMENT(MapLibre)
    QML_ADDED_IN_VERSION(3, 0)

public:
    explicit MapQuickItem(QQuickItem *parent = nullptr);

protected:
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data) override;

private:
    Settings m_settings;
    Map *m_map{};

    bool m_initialized{};
};

} // namespace QMapLibre
