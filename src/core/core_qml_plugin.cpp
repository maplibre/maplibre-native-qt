// Copyright (C) 2024 MapLibre contributors
// SPDX-License-Identifier: BSD-2-Clause

#include <QtQml/qqmlextensionplugin.h>
#include <QtQml/qqmlengine.h>

#include "map.hpp"
#include "utils/metal_renderer_backend.hpp"

namespace {

class MapLibreCoreQmlPlugin final : public QQmlExtensionPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    void registerTypes(const char* uri) override {
        // Expose the low-level C++ API for power users.  We intentionally make
        // the types uncreatable from QML; they are meant to be created from
        // JavaScript or C++ once the Metal layer is available.
        qmlRegisterUncreatableType<QMapLibre::Map>(uri, 3, 0, "Map",
                                                   "Create via JavaScript only");
        qmlRegisterUncreatableType<QMapLibre::MetalRendererBackend>(uri, 3, 0,
                                                   "MetalRendererBackend",
                                                   "Create via JavaScript only");
    }
};

} // namespace

// #include "core_qml_plugin.moc" 