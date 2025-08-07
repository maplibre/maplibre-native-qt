import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15
import QtPositioning 5.15

ApplicationWindow {
    visible: true
    width: Screen.width
    height: Screen.height
    title: qsTr("MapLibre Quick iOS")
    
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#4A90E2" }
            GradientStop { position: 1.0; color: "#7B68EE" }
        }
        
        Column {
            anchors.centerIn: parent
            spacing: 20
            
            Text {
                text: "MapLibre Quick iOS"
                font.pixelSize: 32
                font.bold: true
                color: "white"
                anchors.horizontalCenter: parent.horizontalCenter
            }
            
            Text {
                text: "Running on iOS Simulator"
                font.pixelSize: 18
                color: "white"
                anchors.horizontalCenter: parent.horizontalCenter
            }
            
            Text {
                text: "Metal Rendering: " + (GraphicsInfo.api === GraphicsInfo.Metal ? "Yes" : "No")
                font.pixelSize: 16
                color: "white"
                anchors.horizontalCenter: parent.horizontalCenter
            }
            
            Button {
                text: "Test Button"
                anchors.horizontalCenter: parent.horizontalCenter
                onClicked: {
                    console.log("Button clicked!")
                    statusText.text = "Button was clicked!"
                }
            }
            
            Text {
                id: statusText
                text: "Ready"
                font.pixelSize: 14
                color: "white"
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
    }
    
    Component.onCompleted: {
        console.log("App started successfully")
        console.log("Screen size:", Screen.width, "x", Screen.height)
        console.log("Platform:", Qt.platform.os)
    }
}