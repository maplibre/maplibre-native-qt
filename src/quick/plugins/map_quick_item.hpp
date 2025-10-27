// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include <QtCore/qtmetamacros.h>
#include <QMapLibre/Map>
#include <QMapLibre/Settings>

#include <QtQuick/QQuickItem>
#include <QtQuick/QSGNode>

#include <memory>
#include "types.hpp"

namespace QMapLibre {

class MapQuickItem : public QQuickItem {
    Q_OBJECT
    QML_NAMED_ELEMENT(MapLibre)
    QML_ADDED_IN_VERSION(3, 0)

    Q_PROPERTY(QString style READ style WRITE setStyle)
    Q_PROPERTY(QVariantList coordinate READ coordinate WRITE setCoordinate NOTIFY coordinateChanged)
    Q_PROPERTY(double zoomLevel READ zoomLevel WRITE setZoomLevel NOTIFY zoomLevelChanged)

public:
    explicit MapQuickItem(QQuickItem *parent = nullptr);

    [[nodiscard]] QString style() const { return m_style; }
    void setStyle(const QString &style);

    [[nodiscard]] double zoomLevel() const { return m_zoomLevel; }
    void setZoomLevel(double zoomLevel);

    [[nodiscard]] QVariantList coordinate() const { return m_coordinate; }
    void setCoordinate(const QVariantList &coordinate);
    Q_INVOKABLE void setCoordinateFromPixel(const QPointF &pixel);

    Q_INVOKABLE void pan(const QPointF &offset);
    Q_INVOKABLE void scale(double scale, const QPointF &center);
    Q_INVOKABLE void easeTo(const QVariantMap &camera, const QVariantMap &animation = QVariantMap());
    Q_INVOKABLE void flyTo(const QVariantMap &camera, const QVariantMap &animation = QVariantMap());

    enum SyncState : int {
        NoSync = 0,
        ViewportSync = 1 << 0,
        CameraOptionsSync = 1 << 1,
    };
    Q_DECLARE_FLAGS(SyncStates, SyncState);

signals:
    void coordinateChanged();
    void zoomLevelChanged();

protected:
    void componentComplete() override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data) override;

private slots:
    void initialize();
    void onMapChanged(Map::MapChange change);

private:
    QSGNode *updateMapNode(QSGNode *node);

    Settings m_settings;
    std::shared_ptr<Map> m_map;

    SyncStates m_syncState = NoSync;
    QVariantList m_coordinate{0, 0};
    double m_zoomLevel{};
    QString m_style;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(MapQuickItem::SyncStates)

} // namespace QMapLibre
