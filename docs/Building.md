# How to build

[TOC]

## Qt prerequisites

This library fully supports Qt 6.5 and newer.
Qt 5.15 is fully supported only on desktop platforms,
previous Qt 5 versions down to 5.6 only support widgets but not Qt Location.

### Distros packaging the necessary Qt6 private headers

On these distros every dependency is pre-packaged and can be installed via the package manager.

#### Alpine

```bash
sudo apk add \
    ccache \
    cmake \
    g++ \
    icu-dev \
    ninja-build \
    qt6-qtbase-dev \
    qt6-qtlocation-dev \
    samurai
```

#### Archlinux & Manjaro

```bash
sudo pacman -S \
    ccache \
    cmake \
    gcc \
    icu \
    ninja \
    qt6-base \
    qt6-location
```

#### Fedora

```bash
sudo dnf install \
    ccache \
    cmake \
    gcc-c++ \
    libicu-devel \
    ninja-build \
    qt6-qtbase-devel \
    qt6-qtlocation-devel
```

#### openSUSE

```bash
sudo zypper in \
    ccache \
    cmake \
    gcc-c++ \
    libicu-devel \
    ninja \
    qt6-base-private-devel \
    qt6-location-private-devel \
    qt6-quicktest-private-devel
```

### Distros not packaging the necessary Qt6 private headers

On these distros the minimum required Qt version might be pre-packaged,
but some of the private headers are not packaged, so it needs to be installed via alternative means,
in this case the `aqtinstall` PyPI package. But before that can start some dependencies still need to be installed.

#### Debian / Linux Mint / Ubuntu

```bash
sudo apt update && sudo apt install -y \
   build-essential \
   ccache \
   cmake \
   g++ \
   libgl1-mesa-dev \
   libgstreamer-gl1.0-0 \
   libicu-dev \
   libpulse-dev \
   libxcb-glx0 \
   libxcb-icccm4 \
   libxcb-image0 \
   libxcb-keysyms1 \
   libxcb-randr0 \
   libxcb-render-util0 \
   libxcb-render0 \
   libxcb-shape0 \
   libxcb-shm0 \
   libxcb-sync1 \
   libxcb-util1 \
   libxcb-xfixes0 \
   libxcb-xinerama0 \
   libxcb1 \
   libxkbcommon-dev \
   libxkbcommon-x11-0 \
   libxcb-xkb-dev \
   libxcb-cursor0 \
   ninja-build \
   python3-pip \
   python3-venv
```

#### Generic Qt installation instructions

Once the distro specific dependencies are installed you can install Qt with:

```
QT_VERSION='6.5.3'
python3 -m venv myvenv
source myvenv/bin/activate
python3 -m pip install 'setuptools>=70.1.0' 'py7zr==0.22.*'
python3 -m pip install 'aqtinstall==3.2.*'
python3 -m aqt install-qt linux desktop "${QT_VERSION}" --autodesktop --outputdir "${PWD}/Qt" --modules qtlocation qtpositioning
export PKG_CONFIG_PATH="${PWD}/Qt/${QT_VERSION}/gcc_64/lib/pkgconfig"
export LD_LIBRARY_PATH="${PWD}/Qt/${QT_VERSION}/gcc_64/lib"
export QT_ROOT_DIR="${PWD}/Qt/${QT_VERSION}/gcc_64"
export CMAKE_PREFIX_PATH="${PWD}/Qt/${QT_VERSION}/gcc_64/lib/cmake/Qt6"
export QT_PLUGIN_PATH="${PWD}/Qt/${QT_VERSION}/gcc_64/plugins"
export QML2_IMPORT_PATH="${PWD}/Qt/${QT_VERSION}/gcc_64/qml"
```

## Build basics

MapLibre Native for Qt uses CMake as its build system. Both the core and the
bindings build in the same step. To speed-up the build, `ninja` and `ccache`
are recommended. For Qt 6 using the `qt-cmake` wrapper instead of plain `cmake`
makes building non-desktop platforms easier.

@note To ensure that the git submodules are up to date first run:

```bash
git submodule update --init --recursive
```

@note To make sure a correct version of Qt 6 is used, use the provided toolchain file
with `-DCMAKE_TOOLCHAIN_FILE="<path-to-qt>/lib/cmake/Qt6/qt.toolchain.cmake"`

A minimal example command is:

```bash
mkdir build && cd build
cmake ../maplibre-native-qt -G Ninja \
  -DCMAKE_C_COMPILER_LAUNCHER="ccache" \
  -DCMAKE_CXX_COMPILER_LAUNCHER="ccache" \
  -DCMAKE_INSTALL_PREFIX="../install"
ninja
ninja install
```

See [below](#platform-specific-build-instructions) for platform-specific instructions.

@note It is recommended to use [CMake workflows](#using-cmake-workflows) as they
are always up-to-date and cover all supported platforms.

## Using CMake workflows

CMake workflow presets are provided for all supported platforms.
Run the following command in the root directory to use a preset:

```shell
cmake --workflow --preset <preset>
```

for example

```shell
cmake --workflow --preset macOS-ccache
```

will run the macOS build with `ccache` enabled.

It is recommended to set `QT_ROOT_DIR` environment variable as the path
to the Qt installation to be used, mainly for mobile platforms to use
the correct Qt version.

For Android, the `ANDROID_ABI` environment variable should be set.

### Supported release workflows

| Platform | Qt6       | Qt6 with ccache  | Qt5              | Qt5 with ccache         |
|----------|-----------|------------------|------------------|-------------------------|
| Linux    | `Linux`   | `Linux-ccache`   | `Linux-legacy`   | `Linux-legacy-ccache`   |
| macOS    | `macOS`   | `macOS-ccache`   | `macOS-legacy`   | `macOS-legacy-ccache`   |
| Windows  | `Windows` | `Windows-ccache` | `Windows-legacy` | `Windows-legacy-ccache` |
| iOS      | `iOS`     | `iOS-ccache`     |                  |                         |
| Android  | `Android` | `Android-ccache` |                  |                         |
| WASM     | `WASM`    | `WASM-ccache`    |                  |                         |

### Special workflows

| Platform | Workflow             | Description                                                         |
|----------|----------------------|---------------------------------------------------------------------|
| Linux    | `Linux-coverage`     | Linux build with Qt6, `ccache` and code coverage                    |
| Linux    | `Linux-internal-icu` | Linux build with Qt6 and internal ICU library (also with `-ccache`) |
| macOS    | `macOS-clang-tidy`   | macOS build with Qt6, `ccache` and `clang-tidy`                     |

## Platform specific build instructions

### Linux

Release binaries are build with `-DCMAKE_BUILD_TYPE="Release"`.

Note that when using the standalone Qt installation the system version
of the ICU library is still used. You should make sure that the system ICU
is not too new as it may prevent your app from running on older
versions of Linux. Alternatively you can use internally bundled ICU with the
`-DMLN_QT_WITH_INTERNAL_ICU=ON` CMake option.

To replicate run:

```shell
mkdir build && cd build
cmake ../maplibre-native-qt -G Ninja \
  -DCMAKE_BUILD_TYPE="Release" \
  -DCMAKE_C_COMPILER_LAUNCHER="ccache" \
  -DCMAKE_CXX_COMPILER_LAUNCHER="ccache" \
  -DCMAKE_INSTALL_PREFIX="../install"
ninja
ninja install
```

### macOS

Release binaries contain debug symbols.
Additionally both Intel and ARM versions are supported and included.
OS deployment target version is set to 11.0 for Qt 6 and 10.13 for Qt 5.

To replicate run:

```shell
mkdir build && cd build
cmake ../maplibre-native-qt -G Ninja \
  -DCMAKE_BUILD_TYPE="RelWithDebInfo" \
  -DCMAKE_C_COMPILER_LAUNCHER="ccache" \
  -DCMAKE_CXX_COMPILER_LAUNCHER="ccache" \
  -DCMAKE_INSTALL_PREFIX="../install" \
  -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET="11.0"
ninja
ninja install
```

### Windows

Two separate release binaries are provided, one with release build and one
with debug build. To achieve that `Ninja Multi-Config` generator is used.

To replicate, run:

```shell
mkdir build && cd build
cmake ../maplibre-native-qt -G "Ninja Multi-Config" \
  -DCMAKE_CONFIGURATION_TYPES="Release;Debug" \
  -DCMAKE_C_COMPILER_LAUNCHER="ccache" \
  -DCMAKE_CXX_COMPILER_LAUNCHER="ccache" \
  -DCMAKE_DEFAULT_CONFIGS="all" \
  -DCMAKE_INSTALL_PREFIX="../install"
ninja
ninja install
```

### iOS

Two separate release binaries are provided, one with release build and one
with debug build. To achieve that `Ninja Multi-Config` generator is used.
Both device and simulator builds are supported.
OS deployment target version is set to 14.0.

To replicate, run:

```shell
mkdir build && cd build
cmake ../maplibre-native-qt -G "Ninja Multi-Config" \
  -DCMAKE_CONFIGURATION_TYPES="Release;Debug" \
  -DCMAKE_C_COMPILER_LAUNCHER="ccache" \
  -DCMAKE_CXX_COMPILER_LAUNCHER="ccache" \
  -DCMAKE_DEFAULT_CONFIGS="all" \
  -DCMAKE_INSTALL_PREFIX="../install" \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET="14.0"
ninja
ninja install
```

### Android

Release binaries contain debug symbols. Each ABI is built separately.

To replicate, run:

```shell
mkdir build && cd build
cmake ../maplibre-native-qt -G Ninja \
  -DCMAKE_BUILD_TYPE="RelWithDebInfo" \
  -DCMAKE_C_COMPILER_LAUNCHER="ccache" \
  -DCMAKE_CXX_COMPILER_LAUNCHER="ccache" \
  -DCMAKE_INSTALL_PREFIX="../install" \
  -DANDROID_ABI="arm64-v8a"
ninja
ninja install
```

### WebAssembly (WASM)

No official binaries are provided for WebAssembly.
You can build it yourself using the Emscripten toolchain.
The Qt Location module has to be disabled as it is not supported.

To replicate, run:

```shell
mkdir build && cd build
cmake ../maplibre-native-qt -G Ninja \
  -DCMAKE_BUILD_TYPE="RelWithDebInfo" \
  -DCMAKE_C_COMPILER_LAUNCHER="ccache" \
  -DCMAKE_CXX_COMPILER_LAUNCHER="ccache" \
  -DCMAKE_INSTALL_PREFIX="../install" \
  -DMLN_QT_LOCATION=OFF
ninja
ninja install
```

<div class="section_buttons">

| Previous                          |              Next |
|:----------------------------------|------------------:|
| [Documentation](Documentation.md) | [Usage](Usage.md) |

</div>
