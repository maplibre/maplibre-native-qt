![MapLibre Logo](https://maplibre.org/img/maplibre-logo-big.svg)

# maplibre-native-qt

MapLibre Native Qt bindings and Qt Location MapLibre Plugin

**Important notice:** the repository and code have been reorganised for Qt 6.5 support.
Version 4.0 is a major release that includes major rendering backend changes.
Thus it does not maintain backward compatibility with version 3.x.
It is still in active development and API may change before the stable release.

## Qt support

This library fully supports Qt 6.5 and newer.
Qt 5.15 is fully supported only on desktop platforms, previous Qt 5 versions
down to 5.6 only support widgets but not Qt Location.

## Get and build

The project uses submodules, so you need to clone it with the following commands:

```shell
git clone https://github.com/maplibre/maplibre-native-qt.git
cd maplibre-native-qt
git submodule update --init --recursive
```

(On some file systems, the submodule update may yield a `Filename too long` error
which can be ignored.)

For more details on building the project, see [How to build](docs/Building.md).

## How to use?

Once installed `QMapLibre` can be used in any Qt and CMake project.
Two example projects based on Qt 6 are available in the
[examples](examples) directory.

To build an example, run the following commands:

```shell
export QMapLibre_DIR="<absolute-path-to-install>"
cmake --workflow --preset default
```

or alternatively

```shell
mkdir build-example && cd build-example
qt-cmake ../maplibre-native-qt/examples/<example> -GNinja \
  -DCMAKE_C_COMPILER_LAUNCHER="ccache" \
  -DCMAKE_CXX_COMPILER_LAUNCHER="ccache" \
  -DCMAKE_PREFIX_PATH="<absolute-path-to-install>"
ninja
```

For more details on using the library, see [Usage](docs/Usage.md).

## Copyright

Copyright (C) 2023 MapLibre contributors

The core library may be used under 2-Clause BSD License.
QML bindings may be used under the terms of either GNU General Public License version 2.0,
GNU General Public License version 3.0 or GNU Lesser General Public License version 3.0.
Examples are licensed under MIT.
Full texts of the licenses can be found in the [LICENSES](LICENSES) directory.

Each file contains corresponding license information with SPDX license identifiers
to clarify how it can be used.
