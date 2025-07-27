// Copyright (C) 2024 MapLibre contributors
// SPDX-License-Identifier: BSD-2-Clause

#include "texture_node_base.hpp"
#include "qgeomap.hpp"

namespace QMapLibre {

static const QSize minTextureSize = QSize(64, 64);

TextureNodeBase::TextureNodeBase(const Settings &settings, const QSize &size, qreal pixelRatio, QGeoMapMapLibre *geoMap)
    : m_size(size.expandedTo(minTextureSize)),
      m_pixelRatio(pixelRatio) {
    setTextureCoordinatesTransform(QSGSimpleTextureNode::MirrorVertically);
    setFiltering(QSGTexture::Linear);

    m_map = std::make_unique<Map>(nullptr, settings, m_size, pixelRatio);
    QObject::connect(m_map.get(), &Map::needsRendering, geoMap, &QGeoMap::sgNodeChanged);
}

} // namespace QMapLibre
