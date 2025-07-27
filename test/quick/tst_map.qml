// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

import QtQuick 2.15
import QtLocation 6.5
import QtPositioning 5.15

import MapLibre.Quick 3.0

import QtTest 1.0

Item {
    id: root
    width: 512
    height: 512

    property var coordinate: QtPositioning.coordinate(59.91, 10.75)  // Oslo
    property bool fullView: false

    Rectangle {
        color: "blue"
        anchors.fill: parent
    }

    Rectangle {
        color: "red"
        anchors.fill: parent
        anchors.topMargin: fullView ? 0 : Math.round(parent.height / 3)

        MapLibreView {
            id: mapView
            anchors.fill: parent
            anchors.topMargin: fullView ? 0 : Math.round(parent.height / 6)
            anchors.leftMargin: fullView ? 0 : Math.round(parent.width / 6)

            // map.center: root.coordinate
            // map.zoomLevel: 5
        }
    }

    TestCase {
        id: tc1
        name: "Run"

        function test_map() {
            wait(5000)
            root.fullView = true
            // mapView.map.center = root.coordinate
            wait(5000)
        }
    }
}
