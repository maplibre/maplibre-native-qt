#!/bin/bash -l

set -e
set -x

curl -LO https://github.com/mozilla/sccache/releases/download/v0.12.0/sccache-v0.12.0-x86_64-unknown-linux-musl.tar.gz
tar -xzf sccache-v0.12.0-x86_64-unknown-linux-musl.tar.gz
mv sccache-v0.12.0-x86_64-unknown-linux-musl/sccache /usr/bin/sccache
chmod +x /usr/bin/sccache
rm -rf sccache-v0.12.0-x86_64-unknown-linux-musl*

export PATH="$QT_ROOT_DIR/bin:$PATH"
qmake --version

# Main project
pushd source
cmake --workflow --preset "$1"
popd

mkdir install && pushd install
tar xf "../build/qt6-Linux-$2/"maplibre-native-qt_*.tar.bz2
mv maplibre-native-qt_* maplibre-native-qt
popd

export QMapLibre_DIR="$(pwd)/install/maplibre-native-qt"

# QtQuick example
pushd source/examples/quick
cmake --workflow --preset default
popd

# QtQuick Standalone example
pushd source/examples/quick-standalone
cmake --workflow --preset default
popd

# QtWidgets example
pushd source/examples/widgets
cmake --workflow --preset default
popd

sccache --show-stats
