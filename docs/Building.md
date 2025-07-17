# How to build

[TOC]

## Qt prerequisites

This library fully supports Qt 6.5 and newer.
Qt 5.15 is fully supported only on desktop platforms,
previous Qt 5 versions down to 5.6 only support widgets but not Qt Location.

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
| Linux    | `Linux-CI`           | Linux build with Qt6 and ccache for CI                              |
| Linux    | `Linux-legacy-CI`    | Linux build with Qt5 and ccache for CI                              |
| macOS    | `macOS-clang-tidy`   | macOS build with Qt6, `ccache` and `clang-tidy`                     |

## Platform specific build instructions

### Linux

#### Installing Qt6 prerequisites
##### Alpine

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
##### Archlinux & Manjaro

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

##### Fedora

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

##### openSUSE

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

##### Debian / Ubuntu

_Debian `13 Trixie` / Ubuntu `24.10 Oracular` or newer:_
These releases have the minimum required Qt version pre-packaged, which can be used directly,
but the `qt6-location-dev` package does not include the private headers and there is no `qt6-location-private-dev` package,
which needs a workaround:

```bash
sudo apt update && sudo apt install -y \
    build-essential \
    ccache \
    cmake \
    g++ \
    libicu-dev \
    libxcb-xkb-dev \
    ninja-build \
    qt6-base-dev \
    qt6-base-private-dev \
    qt6-location-dev \
```

Install the private headers from the apropriate source files:

```bash
[ 'ubuntu' = "$(awk -F '=' '/^ID=/{print $2}' /etc/os-release)" ] \
  && OS_URL='http://archive.ubuntu.com/ubuntu/pool/universe' \
  || OS_URL='http://deb.debian.org/debian/pool/main/'
QT_LOCATION_VERSION=$(dpkg -s qt6-location-dev | awk -F '[ -]' '/Version: /{print $2}')
QT_LOCATION_PRIVATE_HEADERS_DIR="/usr/include/x86_64-linux-gnu/qt6/QtLocation/${QT_LOCATION_VERSION}/QtLocation/private/"
sudo mkdir -p "${QT_LOCATION_PRIVATE_HEADERS_DIR}"
mkdir /tmp/location
cd !$
wget "${OS_URL}/q/qt6-location/qt6-location_${QT_LOCATION_VERSION}.orig.tar.xz"
tar -xf qt6-location_${QT_LOCATION_VERSION}.orig.tar.xz
cd qtlocation-everywhere-src-${QT_LOCATION_VERSION}
cmake -B build .
cd build
sudo find  -name \*_p.h -exec cp {} "${QT_LOCATION_PRIVATE_HEADERS_DIR}" \;
cd ../src
sudo find  -name \*_p.h -exec cp {} "${QT_LOCATION_PRIVATE_HEADERS_DIR}" \;
```

##### Generic Linux installation instructions

On Linux distribution versions where the minimum required Qt version is not met,
or the private headers are not packaged the `aqt` tool can be used to install
the right version of Qt with the needed private headers, but first the distribution
specific package manager needs to be used to resolve dependencies of Qt.

###### Installing Qt dependencies

_Debian `12 Bookworm` /  Linux Mint `22.1 Xia` / Ubuntu `24.04 Noble` or older:_
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
   python3-virtualenv
```

Installing Qt via `aqt`:

```
python3 -m venv /tmp/qtvenv
source /tmp/qtvenv/bin/activate
pip install 'setuptools>=70.1.0' 'py7zr==0.22.*'
pip install 'aqtinstall==3.1.*'
aqt install-qt linux desktop 6.5.3 --autodesktop --outputdir "${PWD}/Qt" --modules qtlocation qtpositioning
```

#### Building maplibre-native-qt

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

#### Building with Vulkan support

MapLibre Native Qt supports Vulkan as a rendering backend on Linux. This provides
better performance and is the recommended approach for Qt Quick applications.

##### Prerequisites

In addition to the regular Qt build dependencies, you'll need:

- Vulkan development headers (`libvulkan-dev` on Debian/Ubuntu, `vulkan-devel` on Fedora)
- Vulkan ICD (Installable Client Driver) for your graphics hardware
- Vulkan validation layers for debugging (optional, `vulkan-validationlayers` on Debian/Ubuntu)

##### Building with Vulkan backend

To build MapLibre Native Qt with Vulkan support, use the `-DMLN_WITH_VULKAN=ON` CMake option:

```shell
mkdir build-vulkan && cd build-vulkan
cmake ../maplibre-native-qt -G Ninja \
  -DCMAKE_BUILD_TYPE="Release" \
  -DCMAKE_C_COMPILER_LAUNCHER="ccache" \
  -DCMAKE_CXX_COMPILER_LAUNCHER="ccache" \
  -DCMAKE_INSTALL_PREFIX="../install-vulkan" \
  -DMLN_WITH_VULKAN=ON
ninja
ninja install
```

##### Running examples with Vulkan

When running Qt Quick examples with Vulkan support, you need to set the appropriate
environment variables:

```shell
# Set the QML import path to find the MapLibre plugin
export QML2_IMPORT_PATH="path/to/install-vulkan/qml"

# Tell Qt Quick to use Vulkan RHI backend
export QSG_RHI_BACKEND=vulkan

# Run the example
./QMapLibreExampleQuick
```

For the quick example specifically:

```shell
mkdir examples/quick/build-vulkan && cd examples/quick/build-vulkan
cmake .. -G Ninja \
  -DCMAKE_C_COMPILER_LAUNCHER="ccache" \
  -DCMAKE_CXX_COMPILER_LAUNCHER="ccache" \
  -DQMapLibre_DIR="path/to/install-vulkan"
ninja

# Run with Vulkan
QML2_IMPORT_PATH="path/to/install-vulkan/qml" QSG_RHI_BACKEND=vulkan ./QMapLibreExampleQuick
```

##### Troubleshooting Vulkan

If you encounter issues with Vulkan:

1. **Check Vulkan support**: Use `vulkaninfo` to verify your system supports Vulkan
2. **Verify drivers**: Ensure your graphics drivers support Vulkan
3. **Validation layers**: Enable validation layers for debugging:
   ```shell
   export VK_LAYER_PATH=/usr/share/vulkan/explicit_layer.d
   ```
4. **Debug output**: Run with debug logging:
   ```shell
   QT_LOGGING_RULES="qt.vulkan.debug=true" ./QMapLibreExampleQuick
   ```

### macOS

Release binaries contain debug symbols.
Additionally both Intel and ARM versions are supported and included.
OS deployment target version is set to 12.0 for Qt 6 and 10.13 for Qt 5.

To replicate run:

```shell
mkdir build && cd build
cmake ../maplibre-native-qt -G Ninja \
  -DCMAKE_BUILD_TYPE="RelWithDebInfo" \
  -DCMAKE_C_COMPILER_LAUNCHER="ccache" \
  -DCMAKE_CXX_COMPILER_LAUNCHER="ccache" \
  -DCMAKE_INSTALL_PREFIX="../install" \
  -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET="12.0"
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
OS deployment target version is set to 16.0.

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
  -DCMAKE_OSX_DEPLOYMENT_TARGET="16.0"
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
