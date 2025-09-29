// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include "export_core.hpp"
#include "filter_parameter.hpp"
#include "layer_parameter.hpp"
#include "style_change_p.hpp"
#include "types.hpp"

#include <QtCore/QString>
#include <QtCore/QVariant>
#include <QtCore/QVariantMap>

namespace QMapLibre {

class Map;

class Q_MAPLIBRE_CORE_EXPORT StyleAddLayer : public StyleChange {
public:
    explicit StyleAddLayer(const Feature& feature, const std::vector<FeatureProperty>& properties, QString before);
    explicit StyleAddLayer(const LayerParameter* parameter, QString before);

    void apply(Map* map) override;

private:
    QString m_id;
    QVariantMap m_params;
    QString m_before;

    std::vector<std::unique_ptr<StyleChange>> m_propertyChanges;
};

class Q_MAPLIBRE_CORE_EXPORT StyleRemoveLayer : public StyleChange {
public:
    explicit StyleRemoveLayer(QString id);
    explicit StyleRemoveLayer(const Feature& feature);
    explicit StyleRemoveLayer(const LayerParameter* parameter);

    void apply(Map* map) override;

private:
    QString m_id;
};

class Q_MAPLIBRE_CORE_EXPORT StyleSetLayoutProperties : public StyleChange {
public:
    explicit StyleSetLayoutProperties(QString layerId, const QString& propertyName, const QVariant& value);
    explicit StyleSetLayoutProperties(QString layerId, const std::vector<FeatureProperty>& properties);
    explicit StyleSetLayoutProperties(const LayerParameter* parameter);

    void apply(Map* map) override;

private:
    QString m_layerId;
    std::vector<FeatureProperty> m_properties;
};

class Q_MAPLIBRE_CORE_EXPORT StyleSetPaintProperties : public StyleChange {
public:
    explicit StyleSetPaintProperties(QString layerId, const QString& propertyName, const QVariant& value);
    explicit StyleSetPaintProperties(QString layerId, const std::vector<FeatureProperty>& properties);
    explicit StyleSetPaintProperties(const LayerParameter* parameter);

    void apply(Map* map) override;

private:
    QString m_layerId;
    std::vector<FeatureProperty> m_properties;
};

class Q_MAPLIBRE_CORE_EXPORT StyleSetFilter : public StyleChange {
public:
    explicit StyleSetFilter(QString layerId, QVariantList expression);
    explicit StyleSetFilter(const FilterParameter* parameter);

    void apply(Map* map) override;

private:
    QString m_layerId;
    QVariantList m_expression;
};

} // namespace QMapLibre
