// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include "export_core.hpp"
#include "source_parameter.hpp"
#include "style_change_p.hpp"
#include "types.hpp"

#include <QtCore/QString>
#include <QtCore/QVariantMap>

namespace QMapLibre {

class Map;

class Q_MAPLIBRE_CORE_EXPORT StyleAddSource : public StyleChange {
public:
    explicit StyleAddSource(const Feature &feature);
    explicit StyleAddSource(const SourceParameter *parameter);

    void apply(Map *map) override;

private:
    QString m_id;
    QVariantMap m_params;
};

class Q_MAPLIBRE_CORE_EXPORT StyleRemoveSource : public StyleChange {
public:
    explicit StyleRemoveSource(QString id);
    explicit StyleRemoveSource(const Feature &feature);
    explicit StyleRemoveSource(const SourceParameter *parameter);

    void apply(Map *map) override;

private:
    QString m_id;
};

} // namespace QMapLibre
