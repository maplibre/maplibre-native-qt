![MapLibre Logo](https://maplibre.org/img/maplibre-logo-big.svg)

# qt-geoservices-maplibre-gl

Qt Location MapLibre GL Plugin

## How to build?

### Qt 6

Both plugin and MapLibre build in one step. Ninja is recommended.

```shell
qt-cmake ../qt-geoservices-maplibre-gl -GNinja
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
qmake ../qt-geoservices-maplibre-g QMAPLIBREGL_PATH=../install-qmaplibregl
make
make install
```

## Copyright

Copyright (C) 2022 MapLibre contributors

This project may be used under the terms of either GNU General Public License version 2.0,
GNU General Public License version 3.0 or GNU Lesser General Public License version 3.0.
Examples are licensed under MIT.
