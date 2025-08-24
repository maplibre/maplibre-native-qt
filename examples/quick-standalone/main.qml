// SPDX-License-Identifier: MIT

import QtQuick 6.5

import MapLibre 3.0

Window {
    id: window
    width: Qt.platform.os === "android" ? Screen.width : 512
    height: Qt.platform.os === "android" ? Screen.height : 512
    visible: true

    property bool fullWindow: false  // toggle full map with the 'F' key

    Rectangle {
        color: "blue"
        anchors.fill: parent
        focus: true

        Shortcut {
            sequence: 'F'
            onActivated: function() {
                console.log('F')
                if (window.fullWindow) {
                    window.fullWindow = false
                } else {
                    window.fullWindow = true
                }
            }
        }
    }

    Rectangle {
        color: "red"
        anchors.fill: parent
        anchors.topMargin: fullWindow ? 0 : Math.round(parent.height / 3)

        MapLibre {
            id: map
            anchors.fill: parent
            anchors.topMargin: fullWindow ? 0 : Math.round(parent.height / 6)
            anchors.leftMargin: fullWindow ? 0 : Math.round(parent.width / 6)
            focus: true
        }
    }
}
