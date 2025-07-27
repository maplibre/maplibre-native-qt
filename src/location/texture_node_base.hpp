// Copyright (C) 2024 MapLibre contributors
// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include <QtQuick/QSGSimpleTextureNode>
#include <QtQuick/QQuickWindow>
#include <QMapLibre/Map>

namespace QMapLibre {

class QGeoMapMapLibre;

// Base class for backend-specific texture nodes
class TextureNodeBase : public QSGSimpleTextureNode {
public:
    TextureNodeBase(const Settings &settings, const QSize &size, qreal pixelRatio, QGeoMapMapLibre *geoMap);
    virtual ~TextureNodeBase() = default;

    [[nodiscard]] Map *map() const { return m_map.get(); }

    virtual void resize(const QSize &size, qreal pixelRatio, QQuickWindow *window) = 0;
    virtual void render(QQuickWindow *window) = 0;

protected:
    std::unique_ptr<Map> m_map{};
    QSize m_size;
    qreal m_pixelRatio{1.0};
};

} // namespace QMapLibre