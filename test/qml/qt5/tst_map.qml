// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

import QtQuick 2.15
import QtLocation 5.15
import QtLocation.MapLibre 3.0
import QtPositioning 5.15

import QtTest 1.0

Rectangle {
    id: root
    width: 512
    height: 512
    focus: true
    color: "black"

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

        Map {
            id: map
            anchors.fill: parent
            anchors.topMargin: fullView ? 0 : Math.round(parent.height / 6)
            anchors.leftMargin: fullView ? 0 : Math.round(parent.width / 6)
            plugin: mapPlugin
            center: root.coordinate
            zoomLevel: 5

            MapLibre.style: Style {}
        }
    }

    TestCase {
        id: tc1
        name: "Map"
        when: windowShown

        function test_map() {
            wait(2000)
            root.fullView = true
            map.center = root.coordinate
            wait(500)
        }

        function test_styles() {
            compare(map.supportedMapTypes.length, 2)
        }
    }
}
