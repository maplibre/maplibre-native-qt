// SPDX-License-Identifier: MIT

import QtQuick 6.5

import MapLibre 4.0

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

        Text {
            text: "Press 'F' or tap here to toggle full map"
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 10
            color: "white"
            font.pixelSize: 16
        }

        MouseArea {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: window.fullWindow ? 0 : Math.round(parent.height / 3)
            enabled: !window.fullWindow
            onClicked: {
                window.fullWindow = !window.fullWindow
            }
        }

        Shortcut {
            sequence: 'F'
            onActivated: {
                window.fullWindow = !window.fullWindow
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

            style: "https://demotiles.maplibre.org/style.json"
            zoomLevel: 4
            coordinate: [59.91, 10.75]

            PinchHandler {
                id: pinch
                target: null
                onScaleChanged: (delta) => {
                    map.scale(delta, pinch.centroid.position)
                }
                grabPermissions: PointerHandler.TakeOverForbidden
            }

            DragHandler {
                id: drag
                target: null
                onTranslationChanged: (delta) => map.pan(delta)
            }

            WheelHandler {
                id: wheel
                acceptedDevices: Qt.platform.pluginName === "cocoa" || Qt.platform.pluginName === "wayland"
                                ? PointerDevice.Mouse | PointerDevice.TouchPad
                                : PointerDevice.Mouse
                onWheel: (event) => {
                    map.scale(Math.pow(2.0, event.angleDelta.y / 120), wheel.point.position)
                }
            }
        }
    }
}
