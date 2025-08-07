
# Build instructions (metal example)

Similar prerequisites as here, except `macos` instead of the `wasm_multithread`

https://birkskyum.github.io/maplibre-native-wasm/qt-for-webassembly/webgl1-from-opengl2-legacy-renderer.html#prerequisites

update maplibre-native to the `main` branch and update submodules

## Build the maplibre-native-qt

From the root of maplibre-native-qt run:

```
mkdir -p qmaplibre-build-macos && cd qmaplibre-build-macos
qt-cmake ../ -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
  -DCMAKE_INSTALL_PREFIX="../qmaplibre-install-macos" \
  -DMLN_WITH_METAL=ON  -DMLN_WITH_OPENGL=OFF \
  -DMLN_QT_WITH_LOCATION=ON \
  -DMLN_WITH_CORE_ONLY=ON
ninja
ninja install
```

### Install MapLibre QML Plugin
Since we're using Qt Location (`-DMLN_QT_WITH_LOCATION=ON`), the MapLibre QML plugin is built as part of the location module.

The plugin files are located at:
- `qmaplibre-build-macos/src/location/plugins/MapLibre/qmldir`
- `qmaplibre-build-macos/src/location/plugins/MapLibre/libdeclarative_locationplugin_maplibre.dylib`

No additional copying is needed as the plugin will be found through the QML import path.
##  quick-qtlocation example

#### Build the quick-qtlocation example

```sh
cd examples/quick-qtlocation
mkdir build && cd build
qt-cmake .. -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
  -DQMapLibre_DIR="$(pwd)/../../../qmaplibre-install-macos/lib/cmake/QMapLibre"
ninja
```

#### Run the quick-qtlocation example

First, copy the MapLibre QML plugin to the build directory:
```sh
cp -r /Users/admin/repos/maplibre-native-qt/qmaplibre-build-macos/src/location/plugins/MapLibre ./
```

Then run the example:
```sh
export QML_IMPORT_PATH="$PWD:/Users/admin/repos/maplibre-native-qt/qmaplibre-build-macos"
export QSG_RHI_BACKEND=metal
./QMapLibreExampleQuick.app/Contents/MacOS/QMapLibreExampleQuick
```
