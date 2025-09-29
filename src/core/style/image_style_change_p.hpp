// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include "export_core.hpp"
#include "image_parameter.hpp"
#include "style_change_p.hpp"

#include <QtGui/QImage>

namespace QMapLibre {

class Map;

class Q_MAPLIBRE_CORE_EXPORT StyleAddImage : public StyleChange {
public:
    explicit StyleAddImage(QString id, const QString& spriteUrl);
    explicit StyleAddImage(QString id, QImage sprite);
    explicit StyleAddImage(const ImageParameter* parameter);

    void apply(Map* map) override;

private:
    QString m_id;
    QImage m_sprite;
};

class Q_MAPLIBRE_CORE_EXPORT StyleRemoveImage : public StyleChange {
public:
    explicit StyleRemoveImage(QString id);
    explicit StyleRemoveImage(const ImageParameter* parameter);

    void apply(Map* map) override;

private:
    QString m_id;
};

} // namespace QMapLibre
