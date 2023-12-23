// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: MIT

import QtQuick 6.5
import QtQuick.Window 6.5
import QtLocation 6.5
import QtPositioning 6.5

import QtLocation.MapLibre 3.0

Window {
    id: window
    width: Qt.platform.os === "android" ? Screen.width : 512
    height: Qt.platform.os === "android" ? Screen.height : 512
    visible: true

    property bool fullWindow: false  // toggle full map with the 'F' key
    property var coordinate: QtPositioning.coordinate(41.874, -75.789)

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
        name: "maplibre"
        // specify plugin parameters if necessary
        PluginParameter {
            name: "maplibre.map.styles"
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

            MapLibre.style: Style {
                id: style

                SourceParameter {
                    id: radarSourceParam
                    styleId: "radar"
                    type: "image"
                    property string url: "https://maplibre.org/maplibre-gl-js/docs/assets/radar1.gif"
                    property var coordinates: [
                        [-80.425, 46.437],
                        [-71.516, 46.437],
                        [-71.516, 37.936],
                        [-80.425, 37.936]
                    ]
                }

                LayerParameter {
                    id: radarLayerParam
                    styleId: "radar-layer"
                    type: "raster"
                    property string source: "radar"

                    paint: {
                        "raster-opacity": 0.9
                    }
                }
            }
        }
    }
}
