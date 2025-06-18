// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: MIT

import QtQuick 6.5
import QtQuick.Window 6.5

Window {
    id: win
    width: 800
    height: 600
    visible: true

    property var map   // MapLibre::Map instance created after SG init

    /**
     * Qt Quick's scene-graph (and thus the Metal RHI backend) is ready when
     * this signal fires.  Create the MapLibre renderer at that point.
     */
    onSceneGraphInitialized: Qt.callLater(initMapLibre)

    function initMapLibre() {
        const ri = win.rendererInterface;               // property – no ()
        if (!ri) {
            console.error("No rendererInterface – not running with RHI");
            return;
        }

        // QSGRendererInterface::MetalLayerResource = 6
        const layer = ri.getResource(win, ri.MetalLayerResource);
        if (!layer) {
            console.error("Qt Quick is NOT using the Metal backend");
            return;
        }

        // Expose MapLibre Core C++ API to JS
        const mlib = require("QMapLibre");

        const backend = new mlib.MetalRendererBackend(layer);
        map = new mlib.Map(
                    null,                 // parent QObject
                    {},                   // default Settings{}
                    { width: win.width, height: win.height },
                    Screen.devicePixelRatio,
                    backend);

        map.setStyleUrl("https://demotiles.maplibre.org/style.json");
        map.setCoordinateZoom([40.7128, -74.0060], 10);  // New York
    }

    /** Render the map before Qt Quick swaps the Metal drawable */
    onBeforeRendering: if (map) map.render()
}
