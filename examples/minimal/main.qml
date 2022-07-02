// Copyright (C) 2022 MapLibre contributors

// SPDX-License-Identifier: MIT

import QtQuick 2.15
import QtQuick.Window 2.15
import QtLocation 5.15
import QtPositioning 5.15

Window {
    id: window
    width: Qt.platform.os === "android" ? Screen.width : 512
    height: Qt.platform.os === "android" ? Screen.height : 512
    visible: true

    property bool fullWindow: false  // toggle full map with the 'F' key
    property var coordinate: QtPositioning.coordinate(59.91, 10.75)  // Oslo

    Rectangle {
        color: "blue"
        anchors.fill: parent
        focus: true

        Shortcut {
            sequence: 'F'
            onActivated: function() {
                console.log('F')
                if (window.fullWindow) {
                    window.fullWindow = false
                } else {
                    window.fullWindow = true
                }
                map.center = window.coordinate
            }
        }
    }

    Plugin {
        id: mapPlugin
        name: "maplibregl"
        // specify plugin parameters if necessary
        PluginParameter {
            name: "maplibregl.mapping.additional_style_urls"
            value: "https://demotiles.maplibre.org/style.json"
        }
    }

    Rectangle {
        color: "red"
        anchors.fill: parent
        anchors.topMargin: fullWindow ? 0 : Math.round(parent.height / 3)

        Map {
            id: map
            anchors.fill: parent
            anchors.topMargin: fullWindow ? 0 : Math.round(parent.height / 6)
            anchors.leftMargin: fullWindow ? 0 : Math.round(parent.width / 6)
            plugin: mapPlugin
            center: window.coordinate
            zoomLevel: 5
        }
    }
}
