// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#include "image_style_change_p.hpp"

#include <QMapLibre/Map>

namespace QMapLibre {

// StyleAddImage
StyleAddImage::StyleAddImage(const StyleParameter *parameter)
    : m_id(parameter->property("id").toString()),
      m_sprite(QImage(parameter->property("sprite").toString())) {}

void StyleAddImage::apply(Map *map) {
    if (map == nullptr) {
        return;
    }

    map->addImage(m_id, m_sprite);
}

// StyleRemoveImage
StyleRemoveImage::StyleRemoveImage(QString id)
    : m_id(std::move(id)) {}

StyleRemoveImage::StyleRemoveImage(const StyleParameter *parameter)
    : m_id(parameter->property("id").toString()) {}

void StyleRemoveImage::apply(Map *map) {
    if (map == nullptr) {
        return;
    }

    map->removeImage(m_id);
}

} // namespace QMapLibre
