// Copyright (C) 2023 MapLibre contributors
// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include "export_quick_p.hpp"

#include <QMapLibre/Map>

#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGSimpleTextureNode>

namespace QMapLibre {

// Base class for backend-specific texture nodes
class Q_MAPLIBRE_QUICKPRIVATE_EXPORT TextureNodeBase : public QSGSimpleTextureNode {
public:
    TextureNodeBase(const Settings &settings, const QSize &size, qreal pixelRatio);
    ~TextureNodeBase() override = default;

    [[nodiscard]] Map *map() const { return m_map.get(); }

    virtual void resize(const QSize &size, qreal pixelRatio, QQuickWindow *window) = 0;
    virtual void render(QQuickWindow *window) = 0;

protected:
    std::unique_ptr<Map> m_map;
    QSize m_size;
    qreal m_pixelRatio{1.0};
};

} // namespace QMapLibre
