//! [Attaching a Style]
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
//! [Attaching a Style]

//! [Adding a parameter to a style]
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
//! [Adding a parameter to a style]

//! [Source parameter]
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
//! [Source parameter]

//! [Layer parameter]
LayerParameter {
    id: radarLayerParam
    styleId: "radar-layer"
    type: "raster"
    property string source: "radar"

    paint: {
        "raster-opacity": 0.9
    }
}
//! [Layer parameter]
