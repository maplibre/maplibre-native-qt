#!/bin/bash -l

source scl_source enable devtoolset-8 rh-git218

set -e
set -x

export CCACHE_DIR="$GITHUB_WORKSPACE/.ccache"
export PATH=$Qt5_Dir/bin:$PATH
qmake --version

if [[ "$1" = "library" ]]; then
  mkdir build && cd build
  cmake ../source/dependencies/maplibre-gl-native/ \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=../install-qmaplibregl \
    -DMBGL_WITH_QT=ON \
    -DMBGL_QT_LIBRARY_ONLY=ON \
    -DMBGL_QT_STATIC=ON
  ninja
  ninja install
elif [[ "$1" = "plugin" ]]; then
  mkdir build && cd build
  qmake ../source/ QMAPLIBREGL_PATH=../install-qmaplibregl
  make -j2
  INSTALL_ROOT=../install make install
else
  exit 1
fi
