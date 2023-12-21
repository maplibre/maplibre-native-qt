// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

import QtQuick 2.15
import QtLocation 6.5
import QtPositioning 6.5

import QtTest 1.0

Item {
    width: 512
    height: 512

    Plugin {
        id: mapPlugin
        name: "maplibre"
    }

    MapView {
        id: mapView
        anchors.fill: parent
        map.plugin: mapPlugin
        map.zoomLevel: 3
    }

    TestCase {
        id: tc1
        name: "Plugin"
        when: windowShown

        function test_plugin_no_parameters() {
            compare(mapView.map.supportedMapTypes.length, 0)
            wait(500)
        }
    }
}
