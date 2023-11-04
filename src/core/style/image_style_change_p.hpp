// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include "export_core.hpp"
#include "style_change_p.hpp"
#include "style_parameter.hpp"

#include <QtGui/QImage>

namespace QMapLibre {

class Map;

class Q_MAPLIBRE_CORE_EXPORT StyleAddImage : public StyleChange {
public:
    explicit StyleAddImage(const StyleParameter *parameter);

    void apply(Map *map) override;

private:
    QString m_id;
    QImage m_sprite;
};

class Q_MAPLIBRE_CORE_EXPORT StyleRemoveImage : public StyleChange {
public:
    explicit StyleRemoveImage(QString id);
    explicit StyleRemoveImage(const StyleParameter *parameter);

    void apply(Map *map) override;

private:
    QString m_id;
};

} // namespace QMapLibre
