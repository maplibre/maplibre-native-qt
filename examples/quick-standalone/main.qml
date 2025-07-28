// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: MIT

import QtQuick 6.5
import QtQuick.Window 6.5
import QtPositioning 6.5
import MapLibre.Quick 1.0

Window {
    id: root
    width: 800
    height: 600
    visible: true // Force show window
    title: "MapLibre Native Qt - Quick Example"
    color: "black" // Set window background to black

    // Additional window flags for better visibility on Wayland
    flags: Qt.Window | Qt.WindowTitleHint | Qt.WindowSystemMenuHint | Qt.WindowMinMaxButtonsHint | Qt.WindowCloseButtonHint

    // Ensure the window is properly shown
    Component.onCompleted: {

        // Force show the window
        visible = true;
        show();
    }

    MapLibreView {
        id: mapView
        anchors.fill: parent
        focus: true

        // Ensure the map view is properly configured for Vulkan
        Component.onCompleted: {
            console.log("MapLibreView initialized");
        }
    }
}
