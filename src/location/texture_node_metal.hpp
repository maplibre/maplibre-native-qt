// Copyright (C) 2023 MapLibre contributors
// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include "texture_node_base.hpp"

namespace QMapLibre {

class TextureNodeMetal final : public TextureNodeBase {
public:
    using TextureNodeBase::TextureNodeBase;
    ~TextureNodeMetal() final;

    void resize(const QSize &size, qreal pixelRatio, QQuickWindow *window) final;
    void render(QQuickWindow *window) final;

private:
    bool m_rendererBound = false;
    void *m_layerPtr = nullptr;
    void *m_currentDrawable = nullptr;
    bool m_ownsLayer = false;
};

} // namespace QMapLibre
