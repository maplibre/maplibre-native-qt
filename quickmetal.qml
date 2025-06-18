// quickmetal.qml  –  Metal backend, no Qt-Location plug-in
import QtQuick          6.5
import QtQuick.Window   6.5

Window {
    id: win
    width: 800
    height: 600
    visible: true

    property var map          // keep a reference so we can call render()

    /* 1.  This fires when the scene-graph is ready (Metal RHI created) */
    onSceneGraphInitialized: {
        // property (no parentheses!)
        const ri = win.rendererInterface;
        if (!ri) {
            console.error("Window has no rendererInterface -- not running with RHI");
            return;
        }

        /* 2.  Obtain Qt Quick’s CAMetalLayer */
        const layer =
                ri.getResource(win, ri.MetalLayerResource /* enum value 6 */);
        if (!layer) {
            console.error("Qt Quick is NOT using the Metal backend");
            return;
        }

        /* 3.  Pull in MapLibre’s C++ API */
        const mlib = require("QMapLibre");

        const backend = new mlib.MetalRendererBackend(layer);
        map = new mlib.Map(
                  null, {},                      // parent, Settings{}
                  { width: win.width, height: win.height },
                  Screen.devicePixelRatio, backend);

        map.setStyleUrl("https://demotiles.maplibre.org/style.json");
        map.setCoordinateZoom([40.7128, -74.0060], 10);   // NYC
    }

    /* 4.  Render every frame just before Qt Quick swaps */
    beforeRendering: { if (map) map.render() }
}