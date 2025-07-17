
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
  -DMLN_QT_WITH_LOCATION=OFF \
  -DMLN_WITH_CORE_ONLY=ON
ninja
ninja install
```

### Install MapLibre.Quick
Then to make sure the MapLibre.Quick can be found run the following

##### Create the expected MapLibre/Quick folder inside the build tree
`mkdir -p qmaplibre-build-macos/MapLibre/Quick`

##### Drop the qmldir file and the plugin library into that folder
```
mkdir ./qmaplibre-build-macos/MapLibre
mkdir ./qmaplibre-build-macos/MapLibre/Quick

cp qmaplibre-build-macos/src/quick/qmldir \
   qmaplibre-build-macos/MapLibre/Quick/

cp qmaplibre-build-macos/src/quick/libmaplibre_quickplugin.dylib \
   qmaplibre-build-macos/MapLibre/Quick/
```
## Qt Quick example

#### Build the quick example

```sh
cd examples/quick
mkdir build && cd build
qt-cmake .. -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
  -DQMapLibre_DIR="$(pwd)/../../../qmaplibre-install-macos/lib/cmake/QMapLibre"
ninja
```

#### Run the quick example
```sh

export QML_IMPORT_PATH="/Users/admin/repos/maplibre-native-qt/qmaplibre-build-macos:$PWD"
export QSG_RHI_BACKEND=metal
./QMapLibreExampleQuick.app/Contents/MacOS/QMapLibreExampleQuick
```