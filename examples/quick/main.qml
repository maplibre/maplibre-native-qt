// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: MIT

import QtQuick 6.5
import QtQuick.Window 6.5
import MapLibre.Quick 1.0

Window {
    id: root
    width: 800
    height: 600
    visible: true

    MapLibreView {
        anchors.fill: parent
    }
}
