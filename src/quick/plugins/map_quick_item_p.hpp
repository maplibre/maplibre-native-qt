// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include "map_quick_item.hpp"

#include <QMapLibre/Map>
#include <QMapLibre/Settings>
#include <QMapLibre/StyleParameter>

#include <QtCore/QObject>

#include <memory>

namespace QMapLibre {

class StyleChange;

class MapQuickItemPrivate {
    Q_DECLARE_PUBLIC(MapQuickItem)

public:
    explicit MapQuickItemPrivate(MapQuickItem *q);

    void initialize();

    void addStyleParameter(StyleParameter *parameter);
    void removeStyleParameter(StyleParameter *parameter);
    void clearStyleParameters();

    void syncStyleChanges();

    Settings m_settings;
    std::shared_ptr<Map> m_map;

    MapQuickItem::SyncStates m_syncState = MapQuickItem::NoSync;
    QVariantList m_coordinate{0, 0};
    double m_zoomLevel{};
    QString m_style;
    bool m_styleLoaded{};

    QString m_mapItemsBefore; // TODO: make this a property
    QList<StyleParameter *> m_mapParameters;
    std::vector<std::unique_ptr<StyleChange>> m_styleChanges;

    MapQuickItem *q_ptr{};

private:
    Q_DISABLE_COPY(MapQuickItemPrivate);
};

} // namespace QMapLibre
