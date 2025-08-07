
// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: MIT

import QtQuick 6.5
import QtQuick.Window 6.5
import QtLocation 6.5
import QtPositioning 6.5

// import MapLibre 3.0

Window {
    id: window
    width: Qt.platform.os === "android" ? Screen.width : 512
    height: Qt.platform.os === "android" ? Screen.height : 512
    visible: true

    property bool fullWindow: false  // toggle full map with the 'F' key
    property var coordinate: QtPositioning.coordinate(41.874, -75.789)
    
    Component.onCompleted: {
        console.log("Available location plugins:", Plugin.availableServiceProviders)
        var plugins = Plugin.availableServiceProviders
        debugText.text = "Plugins: " + (plugins.length > 0 ? plugins.join(", ") : "NONE")
        console.log("Number of plugins found:", plugins.length)
        for (var i = 0; i < plugins.length; i++) {
            console.log("Plugin " + i + ": " + plugins[i])
        }
    }

    Rectangle {
        color: "blue"
        anchors.fill: parent
        focus: true
        
        Text {
            id: debugText
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.margins: 10
            color: "white"
            font.pixelSize: 12
            text: "Debug Info Loading..."
            z: 1000
        }

        Shortcut {
            sequence: 'F'
            onActivated: function() {
                console.log('F')
                if (window.fullWindow) {
                    window.fullWindow = false
                } else {
                    window.fullWindow = true
                }
                mapView.map.center = window.coordinate
            }
        }
    }

    Plugin {
        id: mapPlugin
        name: "maplibre"  // Use MapLibre plugin
        
        // Configure MapLibre plugin to use demo tiles
        PluginParameter {
            name: "maplibre.map.styles"
            value: "https://demotiles.maplibre.org/style.json"
        }
        
        PluginParameter {
            name: "maplibre.api_key"
            value: ""  // No API key needed for demo tiles
        }
        
        Component.onCompleted: {
            console.log("Plugin loaded, available services:", availableServiceProviders)
            console.log("Plugin name:", name)
            console.log("Plugin is valid:", isAttached)
            debugText.text += "\nPlugin: " + name + " Valid: " + isAttached
            debugText.text += "\nUsing: MapLibre (vector tiles)"
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
            
            Component.onCompleted: {
                console.log("MapView loaded")
                console.log("Map plugin:", map.plugin)
                console.log("Map error:", map.errorString)
                console.log("Map ready:", map.mapReady)
                debugText.text += "\nMap Error: " + map.errorString
                debugText.text += "\nMap Ready: " + map.mapReady
            }

            // Temporarily comment out the MapLibre.style to test basic map
            /*
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
            */
        }
    }
}
