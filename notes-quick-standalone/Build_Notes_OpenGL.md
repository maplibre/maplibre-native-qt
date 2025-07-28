The files from `/home/birks/repos/maplibre-native-qt/vendor/maplibre-native/platform/qt` have been moved to `/home/birks/repos/maplibre-native-qt/src/platform`

Setup path variables for emsripten and qt (like qt-cmake)

```sh
export PATH="/home/birks/repos/emsdk/upstream/emscripten:$PATH"
export PATH=/home/birks/Qt/6.10.0/gcc_arm64/bin:$PATH
```


In order to run an example, first, the QMapLibre project must be build. Use the build dir `/home/birks/repos/maplibre-native-qt/qmaplibre-build-opengl`, and the install dir `/home/birks/repos/maplibre-native-qt/qmaplibre-install-opengl`


The cmake command is

```sh
cd /home/birks/repos/maplibre-native-qt/qmaplibre-build-opengl && rm -rf * && cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DMLN_WITH_OPENGL=ON \
  -DMLN_WITH_QT=ON \
  -DMLN_QT_WITH_LOCATION=OFF \
  -DMLN_WITH_WERROR=OFF \
  -DCMAKE_INSTALL_PREFIX="../qmaplibre-install-opengl" \
  -DCMAKE_TOOLCHAIN_FILE=/home/birks/Qt/6.10.0/gcc_arm64/lib/cmake/Qt6/qt.toolchain.cmake \
  -G Ninja
```

Then run
`ninja` and `ninja install`



# Quick example  (metal, vulkan, opengl)

Build example


cd /home/birks/repos/maplibre-native-qt/examples/quick-standalone && ls -la

mkdir -p /home/birks/repos/maplibre-native-qt/examples/quick-standalone/build && cd /home/birks/repos/maplibre-native-qt/examples/quick-standalone/build && cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/home/birks/Qt/6.10.0/gcc_arm64 -DQMapLibre_DIR=/home/birks/repos/maplibre-native-qt/qmaplibre-install-opengl/lib64/cmake/QMapLibre


Manually installing MapLibre.Quick

mkdir -p /home/birks/repos/maplibre-native-qt/qmaplibre-install-opengl/qml/MapLibre/Quick && cp src/quick/qmldir src/quick/libmaplibre_quickplugin.so src/quick/libmaplibre_quick.so src/quick/maplibre_quick.qmltypes src/quick/plugin.json /home/birks/repos/maplibre-native-qt/qmaplibre-install-opengl/qml/MapLibre/Quick/


And then run the example

cd /home/birks/repos/maplibre-native-qt/examples/quick-standalone/build && QML_IMPORT_PATH=/home/birks/repos/maplibre-native-qt/qmaplibre-install-opengl/qml QSG_RHI_BACKEND=opengl ./QMapLibreExampleQuick



# Widgets example  (opengl only)

Build the widgets example:

```sh
mkdir -p /home/birks/repos/maplibre-native-qt/examples/widgets/build && cd /home/birks/repos/maplibre-native-qt/examples/widgets/build && cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/home/birks/Qt/6.10.0/gcc_arm64 -DQMapLibre_DIR=/home/birks/repos/maplibre-native-qt/qmaplibre-install-opengl/lib64/cmake/QMapLibre -DQT_VERSION_MAJOR=6

cd /home/birks/repos/maplibre-native-qt/examples/widgets/build && make -j$(nproc)
```

Run the widgets example

```sh
/home/birks/repos/maplibre-native-qt/examples/widgets/build/QMapLibreExampleWidgets
```

## Troubleshooting

### "No such file or directory" error
If you get `bash: ./QMapLibreExampleWidgets: No such file or directory`, make sure you're in the correct directory:

- **Quick example**: Must be run from `/home/birks/repos/maplibre-native-qt/examples/quick-standalone/build/`
- **Widgets example**: Must be run from `/home/birks/repos/maplibre-native-qt/examples/widgets/build/`
