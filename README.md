![MapLibre Logo](https://maplibre.org/img/maplibre-logo-big.svg)

# maplibre-native-qt

MapLibre Native Qt bindings and Qt Location MapLibre Plugin

**Important notice:** Repository is being reorganised for Qt 6.5 support.
Some features will go away temporarily and API will change also for older
releases in the upcoming version 3.0 of the Qt bindings and Qt Location plugin.

## Qt support

This library fully supports Qt 6.5 and newer.
Qt 5.15 is fully supported only on desktop platforms, previous Qt 5 versions
down to 5.6 only support widgets but not Qt Location.

## How to build?

Both plugin and MapLibre build in one step. `ninja` and `ccache` are recommended.
For Qt 6 using the `qt-cmake` wrapper is recommended.

```shell
mkdir build && cd build
cmake ../maplibre-native-qt -GNinja \
  -DCMAKE_C_COMPILER_LAUNCHER="ccache" \
  -DCMAKE_CXX_COMPILER_LAUNCHER="ccache" \
  -DCMAKE_INSTALL_PREFIX="../install"
ninja
ninja install
```

### Linux

Note that when using the system ICU library standalone Qt installation using
installer is ignored. If you want to use that you need to make sure that your
system ICU is not too new as it may prevent your app from running on older
versions of Linux. Alternatively you can use internally bundled ICU with the
`-DMLN_QT_WITH_INTERNAL_ICU=ON` CMake option.

### macOS

Add the following arguments to the CMake call:
`-GNinja -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"`

### iOS

Add the following arguments to the CMake call:
`-G"Ninja Multi-Config" -DCMAKE_CONFIGURATION_TYPES="Release;Debug"`

## How to use?

Once installed `QMapLibre` can be used in any Qt and CMake project.
Two example projects based on Qt 6 are available in the
[examples](examples) directory.

To build an example, run the following commands:

```shell
mkdir build-example && cd build-example
qt-cmake ../maplibre-native-qt/examples/<example> -GNinja \
  -DCMAKE_C_COMPILER_LAUNCHER="ccache" \
  -DCMAKE_CXX_COMPILER_LAUNCHER="ccache" \
  -DCMAKE_PREFIX_PATH="<absolute-path-to-install>"
ninja
```

For macOS a deployment target `deploy` is provided for convenience.

## Copyright

Copyright (C) 2023 MapLibre contributors

The core library may be used under 2-Clause BSD License.
QML bindings may be used under the terms of either GNU General Public License version 2.0,
GNU General Public License version 3.0 or GNU Lesser General Public License version 3.0.
Examples are licensed under MIT.
Full texts of the licenses can be found in the [LICENSES](LICENSES) directory.

Each file contains corresponding license information with SPDX license identifiers
to clarify how it can be used.
