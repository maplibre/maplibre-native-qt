// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

import QtQuick 2.15
import QtQuick.Window 2.15
import QtLocation 6.5
import QtPositioning 6.5

import QtTest 1.0

Item {
    id: window
    width: 512
    height: 512

    Plugin {
        id: mapPlugin
        name: "maplibre"

        PluginParameter {
            name: "maplibre.api.provider"
            value: "maplibre"
        }
    }

    MapView {
        id: mapView
        anchors.fill: parent
        map.plugin: mapPlugin
    }

    TestCase {
        id: tc1
        name: "Run"

        function test_plugin_provider() {
            compare(mapView.map.supportedMapTypes.length, 1)
            wait(500)
        }
    }
}
