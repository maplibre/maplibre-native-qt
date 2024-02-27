#!/bin/bash -l

source scl_source enable devtoolset-8 rh-git218

set -e
set -x

export CCACHE_DIR="$GITHUB_WORKSPACE/.ccache"
export PATH="$Qt5_Dir/bin:$PATH"
qmake --version

# Main project
pushd source
cmake --workflow --preset Linux-legacy-CI
popd
