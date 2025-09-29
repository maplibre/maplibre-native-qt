// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#include "image_style_change_p.hpp"

#include <QMapLibre/Map>

namespace QMapLibre {

/*! \cond PRIVATE */

// StyleAddImage
StyleAddImage::StyleAddImage(QString id, const QString& spriteUrl)
    : m_id(std::move(id)),
      m_sprite(QImage(spriteUrl)) {}

StyleAddImage::StyleAddImage(QString id, QImage sprite)
    : m_id(std::move(id)),
      m_sprite(std::move(sprite)) {}

StyleAddImage::StyleAddImage(const ImageParameter* parameter)
    : m_id(parameter->styleId()),
      m_sprite(QImage(parameter->source())) {}

void StyleAddImage::apply(Map* map) {
    if (map == nullptr) {
        return;
    }

    map->addImage(m_id, m_sprite);
}

// StyleRemoveImage
StyleRemoveImage::StyleRemoveImage(QString id)
    : m_id(std::move(id)) {}

StyleRemoveImage::StyleRemoveImage(const ImageParameter* parameter)
    : m_id(parameter->styleId()) {}

void StyleRemoveImage::apply(Map* map) {
    if (map == nullptr) {
        return;
    }

    map->removeImage(m_id);
}

/*! \endcond */

} // namespace QMapLibre
