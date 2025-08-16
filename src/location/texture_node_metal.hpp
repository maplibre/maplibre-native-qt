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
    bool m_rendererBound{};
    void *m_layerPtr{};
    const void *m_currentDrawable{};
    bool m_ownsLayer{};
};

} // namespace QMapLibre
