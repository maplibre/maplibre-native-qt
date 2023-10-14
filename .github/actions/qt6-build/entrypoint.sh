#!/bin/bash -l

set -e
set -x

export CCACHE_DIR="$GITHUB_WORKSPACE/.ccache"
export PATH="$Qt6_DIR/bin:$PATH"
qmake --version

mkdir build && cd build
qt-cmake ../source/ \
  -G Ninja \
  -DCMAKE_BUILD_TYPE="Release" \
  -DCMAKE_C_COMPILER_LAUNCHER="ccache" \
  -DCMAKE_CXX_COMPILER_LAUNCHER="ccache" \
  -DCMAKE_INSTALL_PREFIX="../install"
ninja
# ninja test
ninja install
cd ..

export PREFIX_PATH="$(pwd)/install"

# QtQuick example
mkdir build-example-quick && cd build-example-quick
qt-cmake ../source/examples/quick/ \
  -G Ninja \
  -DCMAKE_BUILD_TYPE="Release" \
  -DCMAKE_C_COMPILER_LAUNCHER="ccache" \
  -DCMAKE_CXX_COMPILER_LAUNCHER="ccache" \
  -DCMAKE_PREFIX_PATH="$PREFIX_PATH"
ninja
cd ..

# QtWidgets example
mkdir build-example-widgets && cd build-example-widgets
qt-cmake ../source/examples/widgets/ \
  -G Ninja \
  -DCMAKE_BUILD_TYPE="Release" \
  -DCMAKE_C_COMPILER_LAUNCHER="ccache" \
  -DCMAKE_CXX_COMPILER_LAUNCHER="ccache" \
  -DCMAKE_PREFIX_PATH="$PREFIX_PATH"
ninja
cd ..
