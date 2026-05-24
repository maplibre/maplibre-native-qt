//! [Map plugin]
Plugin {
    id: mapPlugin
    name: "maplibre"

    PluginParameter {
        name: "maplibre.map.styles"
        value: "https://demotiles.maplibre.org/style.json"
    }
}
//! [Map plugin]

//! [Style attachment]
MapView {
    id: mapView
    anchors.fill: parent
    map.plugin: mapPlugin

    map.zoomLevel: 5
    map.center: QtPositioning.coordinate(41.874, -75.789)

    MapLibre.style: Style {
        id: style
    }
}
//! [Style attachment]
