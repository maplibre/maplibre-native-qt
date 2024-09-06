// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#include "source_style_change_p.hpp"
#include "style/source_parameter.hpp"

#include <QMapLibre/Map>

#include <QtCore/QDebug>
#include <QtCore/QFile>

namespace QMapLibre {

/*! \cond PRIVATE */

// StyleAddSource
StyleAddSource::StyleAddSource(const Feature &feature)
    : m_id(feature.id.toString()) {
    m_params[QStringLiteral("type")] = QStringLiteral("geojson");
    m_params[QStringLiteral("data")] = QVariant::fromValue<Feature>(feature);
}

StyleAddSource::StyleAddSource(const SourceParameter *parameter)
    : m_id(parameter->styleId()) {
    static const QStringList acceptedSourceTypes = QStringList()
                                                   << QStringLiteral("vector") << QStringLiteral("raster")
                                                   << QStringLiteral("raster-dem") << QStringLiteral("geojson")
                                                   << QStringLiteral("image");

    m_params[QStringLiteral("type")] = parameter->type();

    switch (acceptedSourceTypes.indexOf(parameter->type())) {
        case -1:
            qWarning() << "Invalid value for property 'type': " + parameter->type();
            m_valid = false;
            break;
        case 0: // vector
        case 1: // raster
        case 2: // raster-dem
            if (parameter->hasProperty("url")) {
                m_params[QStringLiteral("url")] = parameter->property("url");
            }
            if (parameter->hasProperty("tiles")) {
                m_params[QStringLiteral("tiles")] = parameter->property("tiles");
            }
            if (parameter->hasProperty("tileSize")) {
                m_params[QStringLiteral("tileSize")] = parameter->property("tileSize");
            }
            break;
        case 3: { // geojson
            const QString data = parameter->parsedProperty("data").toString();
            if (data.startsWith(':')) {
                QFile geojson(data);
                geojson.open(QIODevice::ReadOnly);
                m_params[QStringLiteral("data")] = geojson.readAll();
            } else {
                m_params[QStringLiteral("data")] = data.toUtf8();
            }
        } break;
        case 4: // image
            m_params[QStringLiteral("url")] = parameter->property("url");
            m_params[QStringLiteral("coordinates")] = parameter->property("coordinates");
            break;
        default:
            break;
    }
}

void StyleAddSource::apply(Map *map) {
    if (map == nullptr) {
        return;
    }

    map->updateSource(m_id, m_params);
}

// StyleRemoveSource
StyleRemoveSource::StyleRemoveSource(QString id)
    : m_id(std::move(id)) {}

StyleRemoveSource::StyleRemoveSource(const Feature &feature)
    : m_id(feature.id.toString()) {}

StyleRemoveSource::StyleRemoveSource(const SourceParameter *parameter)
    : m_id(parameter->styleId()) {
    Q_UNUSED(parameter)
}

void StyleRemoveSource::apply(Map *map) {
    if (map == nullptr) {
        return;
    }

    map->removeSource(m_id);
}

/*! \endcond */

} // namespace QMapLibre
