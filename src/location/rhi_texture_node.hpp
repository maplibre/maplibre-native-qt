// Copyright (C) 2024 MapLibre contributors
// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include <QtQuick/QSGTextureProvider>
#include <QtQuick/QSGSimpleTextureNode>

#include <memory>

QT_BEGIN_NAMESPACE
class QQuickWindow;
class QRhiTexture;
QT_END_NAMESPACE

namespace QMapLibre {

// Minimal QRhi-based texture node that will later alias a MapLibre render
// target.  For now it only fulfils the interfaces so the project builds.
class RhiTextureNode final : public QObject,
                             public QSGTextureProvider,
                             public QSGSimpleTextureNode {
    Q_OBJECT
public:
    explicit RhiTextureNode(QQuickWindow* win);

    // Update with the native texture from MapLibre (MTLTexture*, VkImage*, …).
    void syncWithNativeTexture(const void* nativeTex, int w, int h);

    // QSGTextureProvider interface
    QSGTexture* texture() const override { return m_qsgTexture.get(); }

private:
    QQuickWindow* m_window{nullptr};
    std::unique_ptr<QRhiTexture> m_rhiTexture;
    std::unique_ptr<QSGTexture> m_qsgTexture;
};

} // namespace QMapLibre 