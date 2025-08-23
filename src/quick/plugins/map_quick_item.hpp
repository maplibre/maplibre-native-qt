#pragma once

#include <QMapLibre/Map>
#include <QMapLibre/Settings>

#include <QtQuick/QQuickItem>
#include <QtQuick/QSGNode>

#include <memory>

namespace QMapLibre {

class MapQuickItem : public QQuickItem {
    Q_OBJECT
    QML_NAMED_ELEMENT(MapLibre)
    QML_ADDED_IN_VERSION(3, 0)

public:
    explicit MapQuickItem(QQuickItem *parent = nullptr);

protected:
    void componentComplete() override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data) override;

    void wheelEvent(QWheelEvent *event) override;

private slots:
    void initialize();
    void onMapChanged(Map::MapChange change);

private:
    QSGNode *updateMapNode(QSGNode *node);

    Settings m_settings;
    std::shared_ptr<Map> m_map;
};

} // namespace QMapLibre
