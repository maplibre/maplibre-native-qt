// Copyright (C) 2024 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#include <QtQml/qqmlextensionplugin.h>

extern void qml_register_types_MapLibre();

class MapLibreQmlModule : public QQmlEngineExtensionPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlEngineExtensionInterface_iid)

public:
    MapLibreQmlModule(QObject *parent = nullptr)
        : QQmlEngineExtensionPlugin(parent) {
        volatile auto registration = &qml_register_types_MapLibre;
        Q_UNUSED(registration)
    }
};

#include "qml_module.moc"
