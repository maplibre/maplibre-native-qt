![MapLibre Logo](https://maplibre.org/img/maplibre-logo-big.svg)

# maplibre-native-qt

MapLibre Native Qt bindings and Qt Location MapLibre Plugin

**Important notice:** Repository is being reorganised for Qt 6.5 support.
Some features will go away temporarily and API will change also for older
releases in the upcoming version 3.0 of the Qt bindings and Qt Location plugin.

## How to build?

### Qt 6

Both plugin and MapLibre build in one step. Ninja is recommended.

```shell
qt-cmake ../maplibre-native-qt -GNinja
ninja
ninja install
```

_Note_: This will overwrite any upstream plugin version included with Qt.

#### macOS

Add the following arguments to the CMake call:
`-GNinja -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"`

#### iOS

Add the following arguments to the CMake call:
`-G"Ninja Multi-Config" -DCMAKE_CONFIGURATION_TYPES="Release;Debug"`

### Qt 5

A static version of QMapLibreGL needs to be built first and installed to a (temporary) location.
Then the plugin is built separately.

To build the plugin run in a separate build directory:

```shell
qmake ../maplibre-native-qt MAPLIBRE_PATH=../install-maplibre
make
make install
```

## Copyright

Copyright (C) 2023 MapLibre contributors

This project may be used under the terms of either GNU General Public License version 2.0,
GNU General Public License version 3.0 or GNU Lesser General Public License version 3.0.
Examples are licensed under MIT.
