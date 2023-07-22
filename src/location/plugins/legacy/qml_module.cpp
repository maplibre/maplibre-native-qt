// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#include <QtQml/qqmlextensionplugin.h>

extern void qml_register_types_QtLocation_MapLibre();

class QtLocationMapLibreQmlModule : public QQmlEngineExtensionPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlEngineExtensionInterface_iid)

public:
    QtLocationMapLibreQmlModule(QObject *parent = nullptr)
        : QQmlEngineExtensionPlugin(parent) {
        volatile auto registration = &qml_register_types_QtLocation_MapLibre;
        Q_UNUSED(registration)
    }
};

#include "qml_module.moc"
