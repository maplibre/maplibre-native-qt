// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#include "map_quick_style.hpp"

#include "map_quick_item.hpp"

#include <QMapLibre/StyleParameter>

namespace QMapLibre {

MapQuickStyle::MapQuickStyle(QQuickItem *parent)
    : QQuickItem(parent) {
    connect(this, &QQuickItem::parentChanged, this, &MapQuickStyle::onParentChanged);
}

void MapQuickStyle::onParentChanged(QQuickItem *parent) {
    if (parent == nullptr) {
        return;
    }

    auto *mapItem = qobject_cast<MapQuickItem *>(parent);
    if (mapItem == nullptr) {
        return;
    }

    m_map = mapItem;

    for (StyleParameter *p : m_parameters) {
        m_map->addStyleParameter(p);
    }
}

void MapQuickStyle::addParameter(StyleParameter *parameter) {
    if (!parameter->isReady()) {
        connect(parameter, &StyleParameter::ready, this, &MapQuickStyle::addParameter);
        return;
    }

    disconnect(parameter);
    if (m_parameters.contains(parameter)) {
        return;
    }

    parameter->setParent(this);
    m_parameters.append(parameter); // parameter now owned by QDeclarativeGeoMap
    if (m_map != nullptr) {
        m_map->addStyleParameter(parameter);
    }
}

void MapQuickStyle::removeParameter(StyleParameter *parameter) {
    if (!m_parameters.contains(parameter)) {
        return;
    }

    if (m_map != nullptr) {
        m_map->removeStyleParameter(parameter);
    }

    m_parameters.removeOne(parameter);
}

void MapQuickStyle::clearParameters() {
    if (m_map != nullptr) {
        m_map->clearStyleParameters();
    }

    m_parameters.clear();
}

QList<QObject *> MapQuickStyle::parameters() {
    QList<QObject *> list;
    for (StyleParameter *p : std::as_const(m_parameters)) {
        list << p;
    }
    return list;
}

void MapQuickStyle::populateParameters() {
    QObjectList kids = children();
    const QList<QQuickItem *> quickKids = childItems();
    for (int i = 0; i < quickKids.count(); ++i) {
        kids.append(quickKids.at(i));
    }
    for (QObject *kid : kids) {
        auto *parameter = qobject_cast<StyleParameter *>(kid);
        if (parameter != nullptr) {
            addParameter(parameter);
        }
    }
}

void MapQuickStyle::componentComplete() {
    populateParameters();
    QQuickItem::componentComplete();
}

} // namespace QMapLibre
