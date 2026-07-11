// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#include "declarative_style.hpp"
#include "style_parameter.hpp"

#include <QtLocation/private/qdeclarativegeomap_p.h>

namespace QMapLibre {

DeclarativeStyle::DeclarativeStyle(QQuickItem *parent)
    : QQuickItem(parent) {}

void DeclarativeStyle::setDeclarativeMap(QDeclarativeGeoMap *declarativeGeoMap) {
    const auto bindMap = [this, declarativeGeoMap]() {
        if (declarativeGeoMap == nullptr || declarativeGeoMap->map() == nullptr) {
            return;
        }

        setMap(qobject_cast<QGeoMapMapLibre *>(declarativeGeoMap->map()));
    };

    connect(declarativeGeoMap, &QDeclarativeGeoMap::mapReadyChanged, this, bindMap);
    QMetaObject::invokeMethod(this, bindMap, Qt::QueuedConnection);
}

void DeclarativeStyle::setMap(QGeoMapMapLibre *map) {
    if (map == nullptr || m_map != nullptr) {
        return;
    }

    m_map = map;

    for (StyleParameter *p : m_parameters) {
        m_map->addStyleParameter(p);
    }
}

void DeclarativeStyle::addParameter(StyleParameter *parameter) {
    if (!parameter->isReady()) {
        connect(parameter, &StyleParameter::ready, this, &DeclarativeStyle::addParameter);
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

void DeclarativeStyle::removeParameter(StyleParameter *parameter) {
    if (!m_parameters.contains(parameter)) {
        return;
    }

    if (m_map != nullptr) {
        m_map->removeStyleParameter(parameter);
    }

    m_parameters.removeOne(parameter);
}

void DeclarativeStyle::clearParameters() {
    if (m_map != nullptr) {
        m_map->clearStyleParameters();
    }

    m_parameters.clear();
}

QList<QObject *> DeclarativeStyle::parameters() {
    QList<QObject *> list;
    for (StyleParameter *p : std::as_const(m_parameters)) {
        list << p;
    }
    return list;
}

void DeclarativeStyle::populateParameters() {
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

void DeclarativeStyle::componentComplete() {
    populateParameters();
    QQuickItem::componentComplete();
}

} // namespace QMapLibre
