import QtQuick                      6.5
import QtQuick.Window               6.5
import QtCore                       6.5
import QtQml                        6.5

QtObject {
    // settings for the demo
    property string styleUrl: "https://demotiles.maplibre.org/style.json"

    Component.onCompleted: {
        // create a Window that Qt Quick renders with Metal
        const win = Qt.createQmlObject(`
            import QtQuick 6.5
            Window { visible: true; width: 800; height: 600 }
        `, Qt.application, "Win");

        // when the window is ready, retrieve its Metal layer
        const ri    = win.rendererInterface;
        const layer = ri.getResource(win, 6);  // 6 == MetalLayerResource


        // load the MapLibre C++ API that is exposed via QJSEngine
        const QML = Qt.createQmlObject(`
            import QtQml 6.5
            QtObject { signal ready(var mlib) }
        `, win, "Bootstrap");

        QML.ready.connect(mlib => {
            // mlib exports the C++ classes registered by the library
            const backend = new mlib.MetalRendererBackend(layer);
            const map     = new mlib.Map(null, {}, { width: 800, height: 600 },
                                         Screen.devicePixelRatio, backend);

            map.setStyleUrl(styleUrl);

            map.setZoom(2);

            // render before every Qt Quick frame
            win.beforeRendering.connect(() => { map.render(); });
        });

        // import the MapLibre module; this works if QML_IMPORT_PATH is set
        QML.ready(require("QMapLibre"));
    }
}