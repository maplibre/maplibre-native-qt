// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

import QtQuick 2.15
import QtQuick.Window 2.15
import QtLocation 6.5
import QtLocation.MapLibre 3.0
import QtPositioning 5.15

import QtTest 1.0

Item {
    id: window
    width: 512
    height: 512

    property var coordinate: QtPositioning.coordinate(59.91, 10.75)  // Oslo
    property bool fullWindow: false

    Rectangle {
        color: "blue"
        anchors.fill: parent
    }

    Plugin {
        id: mapPlugin
        name: "maplibre"
        // specify plugin parameters if necessary
        PluginParameter {
            name: "maplibre.map.style_urls"
            value: "https://demotiles.maplibre.org/style.json"
        }
    }

    Rectangle {
        color: "red"
        anchors.fill: parent
        anchors.topMargin: fullWindow ? 0 : Math.round(parent.height / 3)

        MapView {
            id: mapView
            anchors.fill: parent
            anchors.topMargin: fullWindow ? 0 : Math.round(parent.height / 6)
            anchors.leftMargin: fullWindow ? 0 : Math.round(parent.width / 6)
            map.plugin: mapPlugin
            map.center: window.coordinate
            map.zoomLevel: 5

            map.MapLibre.style: "foo"
        }
    }

    TestCase {
        id: tc1
        name: "Run"

        function test_map() {
            wait(2000)
            window.fullWindow = true
            mapView.map.center = window.coordinate
            wait(2000)
        }
    }
}
