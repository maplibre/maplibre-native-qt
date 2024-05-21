#!/bin/bash -l

set -e
set -x

export CCACHE_DIR="$GITHUB_WORKSPACE/.ccache"
export PATH="$QT_ROOT_DIR/bin:$PATH"
qmake --version

# Main project
pushd source
cmake --workflow --preset Linux-CI
popd

mkdir install && pushd install
tar xf ../build/qt6-Linux/maplibre-native-qt_*.tar.bz2
mv maplibre-native-qt_* maplibre-native-qt
popd

export QMapLibre_DIR="$(pwd)/install/maplibre-native-qt"

# QtQuick example
pushd source/examples/quick
cmake --workflow --preset default
popd

# QtWidgets example
pushd source/examples/widgets
cmake --workflow --preset default
popd
