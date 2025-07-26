// Copyright (C) 2024 MapLibre contributors
// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include <QtCore/QSize>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGSimpleTextureNode>
#include <QtQuick/QSGTextureProvider>

#include <memory>

QT_BEGIN_NAMESPACE
class QQuickWindow;
QT_END_NAMESPACE

// Need QNativeInterface::QSGMetalTexture from Qt headers.
#include <QtQuick/qsgtexture_platform.h>

namespace QMapLibre {

// Minimal QRhi-based texture node that will later alias a MapLibre render
// target.  For now it only fulfils the interfaces so the project builds.
class RhiTextureNode final : public QSGTextureProvider, public QSGSimpleTextureNode {
public:
    explicit RhiTextureNode(QQuickWindow* win);

    // Update with the native texture from MapLibre (MTLTexture*, VkImage*, …).
    void syncWithNativeTexture(const void* nativeTex, int w, int h);

    // QSGTextureProvider interface
    QSGTexture* texture() const override { return m_qsgTexture.get(); }

private:
    QQuickWindow* m_window{nullptr};
    QSize m_size;
    std::unique_ptr<QSGTexture> m_qsgTexture;
};

} // namespace QMapLibre
