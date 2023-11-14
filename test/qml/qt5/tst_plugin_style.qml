// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

import QtQuick 2.15
import QtQuick.Window 2.15
import QtLocation 5.15
import QtPositioning 5.15

import QtTest 1.0

Item {
    id: window
    width: 512
    height: 512

    Plugin {
        id: mapPlugin
        name: "maplibre"

        PluginParameter {
            name: "maplibre.map.styles"
            value: [
                {
                    "url": "https://demotiles.maplibre.org/style.json",
                    "name": "Demo Tiles",
                    "description": "MapLibre Demo Tiles",
                    "type": MapType.PedestrianMap
                },
                {
                    "url": "https://demotiles.maplibre.org/style.json",
                    "name": "Demo Tiles Night",
                    "description": "MapLibre Demo Tiles for night usage",
                    "night": true,
                    "style": MapType.SatelliteMapNight
                },
                "http://test.com/style.json",
            ]
        }
    }

    Map {
        id: map
        anchors.fill: parent
        plugin: mapPlugin
    }

    TestCase {
        id: tc1
        name: "Run"

        function test_plugin_style() {
            compare(map.supportedMapTypes.length, 3)
            wait(500)
        }

        function test_plugin_style_values() {
            compare(map.supportedMapTypes[0].name, "Demo Tiles")
            compare(map.supportedMapTypes[0].description, "MapLibre Demo Tiles")
            compare(map.supportedMapTypes[0].night, false)
            compare(map.supportedMapTypes[0].style, MapType.PedestrianMap)
            compare(map.supportedMapTypes[0].metadata["url"], "https://demotiles.maplibre.org/style.json")

            compare(map.supportedMapTypes[1].name, "Demo Tiles Night")
            compare(map.supportedMapTypes[1].description, "MapLibre Demo Tiles for night usage")
            compare(map.supportedMapTypes[1].night, true)
            compare(map.supportedMapTypes[1].style, MapType.SatelliteMapNight)
            compare(map.supportedMapTypes[1].metadata["url"], "https://demotiles.maplibre.org/style.json")

            compare(map.supportedMapTypes[2].name, "Style 3")
            compare(map.supportedMapTypes[2].description, "")
            compare(map.supportedMapTypes[2].night, false)
            compare(map.supportedMapTypes[2].style, MapType.CustomMap)
            compare(map.supportedMapTypes[2].metadata["url"], "http://test.com/style.json")
        }
    }
}
