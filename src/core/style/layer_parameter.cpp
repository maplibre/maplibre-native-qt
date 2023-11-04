// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#include "layer_parameter.hpp"
#include "style_parameter.hpp"

namespace QMapLibre {

LayerParameter::LayerParameter(QObject *parent)
    : StyleParameter(parent) {}

LayerParameter::~LayerParameter() = default;

QString LayerParameter::type() const {
    return m_type;
}

void LayerParameter::setType(const QString &type) {
    if (m_type.isEmpty()) {
        m_type = type;
    }
}

QJsonObject LayerParameter::layout() const {
    return m_layout;
}

void LayerParameter::setLayout(const QJsonObject &layout) {
    if (m_layout == layout) {
        return;
    }

    m_layout = layout;

    Q_EMIT layoutUpdated();
}

void LayerParameter::setLayoutProperty(const QString &key, const QVariant &value) {
    m_layout[key] = value.toJsonValue();

    Q_EMIT layoutUpdated();
}

QJsonObject LayerParameter::paint() const {
    return m_paint;
}

void LayerParameter::setPaint(const QJsonObject &paint) {
    if (m_paint == paint) {
        return;
    }

    m_paint = paint;

    Q_EMIT paintUpdated();
}

void LayerParameter::setPaintProperty(const QString &key, const QVariant &value) {
    m_paint[key] = value.toJsonValue();

    Q_EMIT paintUpdated();
}

} // namespace QMapLibre
