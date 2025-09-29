// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#include "filter_parameter.hpp"
#include "layer_parameter.hpp"
#include "layer_style_change_p.hpp"
#include "style_change_utils_p.hpp"
#include "types.hpp"

#include <QMapLibre/Map>

namespace QMapLibre {

/*! \cond PRIVATE */

// StyleAddLayer
StyleAddLayer::StyleAddLayer(const Feature& feature, const std::vector<FeatureProperty>& properties, QString before)
    : m_id(feature.id.toString()),
      m_before(std::move(before)) {
    m_params[QStringLiteral("source")] = feature.id;

    switch (feature.type) {
        case Feature::PointType:
            m_params[QStringLiteral("type")] = QStringLiteral("circle");
            break;
        case Feature::LineStringType:
            m_params[QStringLiteral("type")] = QStringLiteral("line");
            break;
        case Feature::PolygonType:
            m_params[QStringLiteral("type")] = QStringLiteral("fill");
            break;
    }

    for (const FeatureProperty& property : properties) {
        if (property.type == FeatureProperty::LayoutProperty) {
            m_propertyChanges.emplace_back(
                std::make_unique<StyleSetLayoutProperties>(m_id, property.name, property.value));
        } else if (property.type == FeatureProperty::PaintProperty) {
            m_propertyChanges.emplace_back(
                std::make_unique<StyleSetPaintProperties>(m_id, property.name, property.value));
        }
    }
}

StyleAddLayer::StyleAddLayer(const LayerParameter* parameter, QString before)
    : m_id(parameter->styleId()),
      m_before(std::move(before)) {
    m_params[QStringLiteral("type")] = parameter->type();

    const QList<QByteArray> propertyNames = StyleChangeUtils::allPropertyNamesList(parameter);
    for (const QByteArray& propertyName : propertyNames) {
        const QVariant value = parameter->property(propertyName);
        m_params[StyleChangeUtils::formatPropertyName(propertyName)] = value;
    }

    m_propertyChanges.push_back(std::make_unique<StyleSetLayoutProperties>(parameter));
    m_propertyChanges.push_back(std::make_unique<StyleSetPaintProperties>(parameter));
}

void StyleAddLayer::apply(Map* map) {
    if (map == nullptr) {
        return;
    }

    if (!map->layerExists(m_id)) {
        map->addLayer(m_id, m_params, m_before);
    }

    for (const std::unique_ptr<StyleChange>& change : m_propertyChanges) {
        if (change->isValid()) {
            change->apply(map);
        }
    }
}

// StyleRemoveLayer
StyleRemoveLayer::StyleRemoveLayer(QString id)
    : m_id(std::move(id)) {}

StyleRemoveLayer::StyleRemoveLayer(const Feature& feature)
    : m_id(feature.id.toString()) {}

StyleRemoveLayer::StyleRemoveLayer(const LayerParameter* parameter)
    : m_id(parameter->styleId()) {
    Q_UNUSED(parameter)
}

void StyleRemoveLayer::apply(Map* map) {
    if (map == nullptr) {
        return;
    }

    map->removeLayer(m_id);
}

// StyleSetLayoutProperties
StyleSetLayoutProperties::StyleSetLayoutProperties(QString layerId, const QString& propertyName, const QVariant& value)
    : m_layerId(std::move(layerId)) {
    m_properties.emplace_back(FeatureProperty::LayoutProperty, propertyName, value);
}

StyleSetLayoutProperties::StyleSetLayoutProperties(const LayerParameter* parameter)
    : m_layerId(parameter->styleId()) {
    const QJsonObject& layout = parameter->layout();
    for (const QString& key : layout.keys()) {
        m_properties.emplace_back(
            FeatureProperty::LayoutProperty, StyleChangeUtils::formatPropertyName(key), layout.value(key).toVariant());
    }
}

void StyleSetLayoutProperties::apply(Map* map) {
    if (map == nullptr) {
        return;
    }

    for (const FeatureProperty& property : m_properties) {
        map->setLayoutProperty(m_layerId, property.name, property.value);
    }
}

// StyleSetPaintProperties
StyleSetPaintProperties::StyleSetPaintProperties(QString layerId, const QString& propertyName, const QVariant& value)
    : m_layerId(std::move(layerId)) {
    m_properties.emplace_back(FeatureProperty::PaintProperty, propertyName, value);
}

StyleSetPaintProperties::StyleSetPaintProperties(const LayerParameter* parameter)
    : m_layerId(parameter->styleId()) {
    const QJsonObject& paint = parameter->paint();
    for (const QString& key : paint.keys()) {
        m_properties.emplace_back(
            FeatureProperty::PaintProperty, StyleChangeUtils::formatPropertyName(key), paint.value(key).toVariant());
    }
}

void StyleSetPaintProperties::apply(Map* map) {
    if (map == nullptr) {
        return;
    }

    for (const FeatureProperty& property : m_properties) {
        map->setPaintProperty(m_layerId, property.name, property.value);
    }
}

// StyleSetFilter
StyleSetFilter::StyleSetFilter(QString layerId, QVariantList expression)
    : m_layerId(std::move(layerId)),
      m_expression(std::move(expression)) {}

StyleSetFilter::StyleSetFilter(const FilterParameter* parameter)
    : m_layerId(parameter->styleId()),
      m_expression(parameter->expression()) {}

void StyleSetFilter::apply(Map* map) {
    if (map == nullptr) {
        return;
    }

    map->setFilter(m_layerId, m_expression);
}

/*! \endcond */

} // namespace QMapLibre
