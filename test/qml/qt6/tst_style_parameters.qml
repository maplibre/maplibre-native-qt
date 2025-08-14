// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

import QtQuick 2.15
import QtLocation 6.5
import QtPositioning 6.5

import MapLibre 3.0

import QtTest 1.0

Item {
    width: 512
    height: 512

    Plugin {
        id: mapPlugin
        name: "maplibre"

        PluginParameter {
            name: "maplibre.map.styles"
            value: "https://demotiles.maplibre.org/style.json"
        }
    }

    MapView {
        id: mapView
        anchors.fill: parent
        map.plugin: mapPlugin

        map.zoomLevel: 5
        map.center: QtPositioning.coordinate(41.874, -75.789)

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

            FilterParameter {
                id: filterParam
                styleId: "countries-fill"
                expression: ["==", "ADM0_A3", "NLD"]
            }
        }
    }

    TestCase {
        id: tc1
        name: "StyleParameters"
        when: windowShown

        function test_init() {
            compare(mapView.map.supportedMapTypes.length, 1)
            wait(500)
            compare(radarSourceParam.url, "https://maplibre.org/maplibre-gl-js/docs/assets/radar1.gif")
        }

        function test_style_1_filter_change() {
            // Test graceful handling of invalid filter
            filterParam.expression = [123, "ADM0_A3", 123]
            compare(filterParam.expression, [123, "ADM0_A3", 123])
            // Test updating of the filter
            filterParam.expression = ["==", "ADM0_A3", "USA"]
            compare(filterParam.expression, ["==", "ADM0_A3", "USA"])
            wait(500)
        }

        function test_style_2_filter_remove() {
            style.removeParameter(filterParam)
            wait(500)
        }

        function test_style_3_image_change() {
            radarSourceParam.url = "https://maplibre.org/maplibre-gl-js/docs/assets/radar2.gif"
            compare(radarSourceParam.url, "https://maplibre.org/maplibre-gl-js/docs/assets/radar2.gif")
            wait(250)
            radarSourceParam.url = "https://maplibre.org/maplibre-gl-js/docs/assets/radar3.gif"
            compare(radarSourceParam.url, "https://maplibre.org/maplibre-gl-js/docs/assets/radar3.gif")
            wait(250)
            radarSourceParam.url = "https://maplibre.org/maplibre-gl-js/docs/assets/radar4.gif"
            compare(radarSourceParam.url, "https://maplibre.org/maplibre-gl-js/docs/assets/radar4.gif")
            wait(250)
            radarSourceParam.coordinates = [
                [-85.425, 46.437],
                [-76.516, 46.437],
                [-76.516, 37.936],
                [-85.425, 37.936]
            ]
            compare(radarSourceParam.coordinates, [
                [-85.425, 46.437],
                [-76.516, 46.437],
                [-76.516, 37.936],
                [-85.425, 37.936]
            ])
            wait(250)
        }

        function test_style_4_paint_change() {
            radarLayerParam.paint = {"raster-opacity": 0.8}
            wait(250)
            radarLayerParam.paint = {"raster-opacity": 0.6}
            wait(250)
            radarLayerParam.paint = {"raster-opacity": 0.4}
            wait(250)
            radarLayerParam.paint = {"raster-opacity": 0.2}
            wait(500)
            radarLayerParam.paint = {"raster-opacity": 1.0}
            wait(500)
        }

        function test_style_5_paint_set() {
            radarLayerParam.setPaintProperty("raster-opacity", 0.8)
            wait(250)
            radarLayerParam.setPaintProperty("raster-opacity", 0.6)
            wait(250)
            radarLayerParam.setPaintProperty("raster-opacity", 0.4)
            wait(250)
            radarLayerParam.setPaintProperty("raster-opacity", 0.2)
            wait(500)
            radarLayerParam.setPaintProperty("raster-opacity", 1.0)
            wait(500)
        }

        function test_style_6_remove_image() {
            style.removeParameter(radarLayerParam)
            style.removeParameter(radarSourceParam)
            wait(500)
        }

        function test_style_7_tiles() {
            let url = "https://tiles.maps.eox.at/wms?service=wms&bbox={bbox-epsg-3857}&format=image/png&service=WMS&version=1.1.1&request=GetMap&srs=EPSG:900913&width=256&height=256&layers=s2cloudless-2021_3857"

            let sourceParam = Qt.createQmlObject(`
                import MapLibre 3.0

                SourceParameter {
                    styleId: "tileSource"
                    type: "raster"
                    property var tiles: ["${url}"]
                    property int tileSize: 512
                }
                `,
                style,
                "sourceParamSnippet"
            )
            style.addParameter(sourceParam)

            let layerParam = Qt.createQmlObject(`
                import MapLibre 3.0

                LayerParameter {
                    styleId: "tileLayer"
                    type: "raster"
                    property string source: "tileSource"
                }
                `,
                style,
                "layerParamSnippet")
            style.addParameter(layerParam)

            wait(3000)

            style.removeParameter(layerParam)
            style.removeParameter(sourceParam)
            wait(1000)
        }

        function test_style_8_image() {
            let imageParam = Qt.createQmlObject(`
                import MapLibre 3.0

                ImageParameter {
                    styleId: "locationImage"
                    source: ":/1x.png"
                }
                `,
                style,
                "imageParamSnippet")
            style.addParameter(imageParam)

            let sourceParam = Qt.createQmlObject(`
                import MapLibre 3.0

                SourceParameter {
                    styleId: "pointSource"
                    type: "geojson"
                    property var data: {
                        "type": "FeatureCollection",
                        "features": [
                            {
                                "type": "Feature",
                                "geometry": {
                                    "type": "Point",
                                    "coordinates": [-80, 45]
                                }
                            }
                        ]
                    }
                }
                `,
                style,
                "sourceParamSnippet"
            )
            style.addParameter(sourceParam)

            let layerParam = Qt.createQmlObject(`
                import MapLibre 3.0

                LayerParameter {
                    styleId: "pointLayer"
                    type: "symbol"
                    property string source: "pointSource"

                    layout: {
                        "icon-image": "locationImage",
                        "icon-size": 2.5
                    }
                }
                `,
                style,
                "layerParamSnippet")
            style.addParameter(layerParam)

            wait(1000)

            style.removeParameter(layerParam)
            style.removeParameter(sourceParam)
            style.removeParameter(imageParam)
            wait(1000)
        }
    }
}
