![MapLibre Logo](https://maplibre.org/img/maplibre-logo-big.svg)

# maplibre-native-qt

MapLibre Native Qt bindings and Qt Location MapLibre Plugin

**Important notice:** Repository is being reorganised for Qt 6.5 support.
Some features will go away temporarily and API will change also for older
releases in the upcoming version 3.0 of the Qt bindings and Qt Location plugin.

## Qt support

This library fully supports Qt 6.5 and newer.
Qt 5.15 is fully supported only on desktop platforms.

## How to build?

Both plugin and MapLibre build in one step. Ninja is recommended.
For Qt 6 using the `qt-cmake` wrapper is recommended.

```shell
cmake ../maplibre-native-qt -GNinja
ninja
ninja install
```

### macOS

Add the following arguments to the CMake call:
`-GNinja -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"`

### iOS

Add the following arguments to the CMake call:
`-G"Ninja Multi-Config" -DCMAKE_CONFIGURATION_TYPES="Release;Debug"`

## Copyright

Copyright (C) 2023 MapLibre contributors

The core library may be used under 2-Clause BSD License.
QML bindings may be used under the terms of either GNU General Public License version 2.0,
GNU General Public License version 3.0 or GNU Lesser General Public License version 3.0.
Examples are licensed under MIT.
Full texts of the licenses can be found in the [LICENSES](LICENSES) directory.

Each file contains corresponding license information with SPDX license identifiers
to clarify how it can be used.
