// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

import QtQuick 2.15
import QtLocation 6.5
import QtPositioning 5.15

import MapLibre 3.0

import QtTest 1.0

Item {
    id: root
    width: 512
    height: 512

    property var coordinate: QtPositioning.coordinate(59.91, 10.75)  // Oslo
    property bool fullView: false

    Rectangle {
        color: "blue"
        anchors.fill: parent
    }

    Plugin {
        id: mapPlugin
        name: "maplibre"
        // specify plugin parameters if necessary
        PluginParameter {
            name: "maplibre.map.styles"
            value: "https://demotiles.maplibre.org/style.json,https://demotiles.maplibre.org/style2.json"
        }
    }

    Rectangle {
        color: "red"
        anchors.fill: parent
        anchors.topMargin: fullView ? 0 : Math.round(parent.height / 3)

        MapView {
            id: mapView
            anchors.fill: parent
            anchors.topMargin: fullView ? 0 : Math.round(parent.height / 6)
            anchors.leftMargin: fullView ? 0 : Math.round(parent.width / 6)
            map.plugin: mapPlugin
            map.center: root.coordinate
            map.zoomLevel: 5

            map.MapLibre.style: Style {}
        }
    }

    TestCase {
        id: tc1
        name: "Run"

        function test_map() {
            wait(2000)
            root.fullView = true
            mapView.map.center = root.coordinate
            wait(500)
        }

        function test_styles() {
            compare(mapView.map.supportedMapTypes.length, 2)
        }
    }
}
