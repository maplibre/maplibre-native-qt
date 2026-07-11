// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include <QMapLibre/Map>
#include <QMapLibre/StyleParameter>

#include <QtQuick/QQuickItem>
#include <QtQuick/QSGNode>

#include <memory>

namespace QMapLibre {

class MapQuickItemPrivate;
class StyleChange;

class MapQuickItem : public QQuickItem {
    Q_OBJECT
    Q_DECLARE_PRIVATE(MapQuickItem)
    QML_NAMED_ELEMENT(MapLibre)
    QML_ADDED_IN_VERSION(3, 0)

    Q_PROPERTY(QString style READ style WRITE setStyle)
    Q_PROPERTY(QVariantList coordinate READ coordinate WRITE setCoordinate NOTIFY coordinateChanged)
    Q_PROPERTY(double zoomLevel READ zoomLevel WRITE setZoomLevel NOTIFY zoomLevelChanged)

public:
    enum SyncState : int {
        NoSync = 0,
        ViewportSync = 1 << 0,
        CameraOptionsSync = 1 << 1,
    };
    Q_DECLARE_FLAGS(SyncStates, SyncState);

    explicit MapQuickItem(QQuickItem *parent = nullptr);
    ~MapQuickItem() override;

    [[nodiscard]] QString style() const;
    void setStyle(const QString &style);

    [[nodiscard]] double zoomLevel() const;
    void setZoomLevel(double zoomLevel);

    [[nodiscard]] QVariantList coordinate() const;
    void setCoordinate(const QVariantList &coordinate);
    Q_INVOKABLE void setCoordinateFromPixel(const QPointF &pixel);

    Q_INVOKABLE void pan(const QPointF &offset);
    Q_INVOKABLE void scale(double scale, const QPointF &center);
    Q_INVOKABLE void easeTo(const QVariantMap &camera, const QVariantMap &animation = QVariantMap());
    Q_INVOKABLE void flyTo(const QVariantMap &camera, const QVariantMap &animation = QVariantMap());

    Q_INVOKABLE void addStyleParameter(StyleParameter *parameter);
    Q_INVOKABLE void removeStyleParameter(StyleParameter *parameter);
    Q_INVOKABLE void clearStyleParameters();

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
    void onStyleParameterUpdated(StyleParameter *parameter);

private:
    QSGNode *updateMapNode(QSGNode *node);

    std::unique_ptr<MapQuickItemPrivate> d_ptr;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(MapQuickItem::SyncStates)

} // namespace QMapLibre
