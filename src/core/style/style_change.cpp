// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#include "image_style_change_p.hpp"
#include "layer_style_change_p.hpp"
#include "source_style_change_p.hpp"
#include "style_change_p.hpp"

#include <QMapLibre/Map>

/*! \cond PRIVATE */

namespace QMapLibre {

// StyleChange
std::vector<std::unique_ptr<StyleChange>> StyleChange::addFeature(const Feature& feature,
                                                                  const std::vector<FeatureProperty>& properties,
                                                                  const QString& before) {
    std::vector<std::unique_ptr<StyleChange>> changes;

    changes.push_back(std::make_unique<StyleAddSource>(feature));
    changes.push_back(std::make_unique<StyleAddLayer>(feature, properties, before));

    return changes;
}

std::vector<std::unique_ptr<StyleChange>> StyleChange::removeFeature(const Feature& feature) {
    std::vector<std::unique_ptr<StyleChange>> changes;

    changes.push_back(std::make_unique<StyleRemoveLayer>(feature));
    changes.push_back(std::make_unique<StyleRemoveSource>(feature));

    return changes;
}

std::vector<std::unique_ptr<StyleChange>> StyleChange::addParameter(const StyleParameter* parameter,
                                                                    const QString& before) {
    std::vector<std::unique_ptr<StyleChange>> changes;

    const auto* sourceParameter = qobject_cast<const SourceParameter*>(parameter);
    if (sourceParameter != nullptr) {
        changes.push_back(std::make_unique<StyleAddSource>(sourceParameter));
        return changes;
    }

    const auto* layerParameter = qobject_cast<const LayerParameter*>(parameter);
    if (layerParameter != nullptr) {
        changes.push_back(std::make_unique<StyleAddLayer>(layerParameter, before));
        return changes;
    }

    const auto* imageParameter = qobject_cast<const ImageParameter*>(parameter);
    if (imageParameter != nullptr) {
        changes.push_back(std::make_unique<StyleAddImage>(imageParameter));
        return changes;
    }

    const auto* filterParameter = qobject_cast<const FilterParameter*>(parameter);
    if (filterParameter != nullptr) {
        changes.push_back(std::make_unique<StyleSetFilter>(filterParameter));
        return changes;
    }

    return changes;
}

std::vector<std::unique_ptr<StyleChange>> StyleChange::removeParameter(const StyleParameter* parameter) {
    std::vector<std::unique_ptr<StyleChange>> changes;

    const auto* sourceParameter = qobject_cast<const SourceParameter*>(parameter);
    if (sourceParameter != nullptr) {
        changes.push_back(std::make_unique<StyleRemoveSource>(sourceParameter));
        return changes;
    }

    const auto* layerParameter = qobject_cast<const LayerParameter*>(parameter);
    if (layerParameter != nullptr) {
        changes.push_back(std::make_unique<StyleRemoveLayer>(layerParameter));
        return changes;
    }

    const auto* imageParameter = qobject_cast<const ImageParameter*>(parameter);
    if (imageParameter != nullptr) {
        changes.push_back(std::make_unique<StyleRemoveImage>(imageParameter));
        return changes;
    }

    const auto* filterParameter = qobject_cast<const FilterParameter*>(parameter);
    if (filterParameter != nullptr) {
        changes.push_back(std::make_unique<StyleSetFilter>(filterParameter->styleId(), QVariantList()));
        return changes;
    }

    return changes;
}

/*! \endcond */

} // namespace QMapLibre
