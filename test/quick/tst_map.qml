// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

import QtQuick 2.15

import MapLibre 3.0

import QtTest 1.0

Item {
    id: root
    width: 512
    height: 512

    property bool fullView: false

    Rectangle {
        color: "blue"
        anchors.fill: parent
    }

    Rectangle {
        color: "red"
        anchors.fill: parent
        anchors.topMargin: fullView ? 0 : Math.round(parent.height / 3)

        MapLibre {
            id: map

            style: "https://demotiles.maplibre.org/style.json"
            zoomLevel: 4
            coordinate: [59.91, 10.75]

            anchors.fill: parent
            anchors.topMargin: fullView ? 0 : Math.round(parent.height / 6)
            anchors.leftMargin: fullView ? 0 : Math.round(parent.width / 6)
        }
    }

    TestCase {
        id: tc1
        name: "Run"

        function test_map() {
            wait(2000)
            root.fullView = true
            // map.center = root.coordinate
            wait(2000)
        }
    }
}
