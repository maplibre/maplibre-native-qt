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

See below for platform-specific instructions.

@note It is recommended to use [CMake workflows](#using-cmake-workflows) as they
are always up-to-date and cover all supported platforms.

### Linux

Release binaries are build with `-DCMAKE_BUILD_TYPE="Release"`.

Note that when using the standalone Qt installation the system version
of the ICU library is still used. You should make sure that the system ICU
is not too new as it may prevent your app from running on older
versions of Linux. Alternatively you can use internally bundled ICU with the
`-DMLN_QT_WITH_INTERNAL_ICU=ON` CMake option.

### macOS

Release binaries contain debug symbols.
Additionally both Intel and ARM versions are supported and included.
OS deployment target version is set to 11.0 for Qt 6 and 10.13 for Qt 5.

To replicate run:

```shell
cmake ../maplibre-native-qt -G Ninja \
  -DCMAKE_BUILD_TYPE="RelWithDebInfo" \
  -DCMAKE_C_COMPILER_LAUNCHER="ccache" \
  -DCMAKE_CXX_COMPILER_LAUNCHER="ccache" \
  -DCMAKE_INSTALL_PREFIX="../install" \
  -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET="11.0"
```

### Windows

Two separate release binaries are provided, one with release build and one
with debug build. To achieve that `Ninja Multi-Config` generator is used.

To replicate, run:

```shell
cmake ../maplibre-native-qt -G "Ninja Multi-Config" \
  -DCMAKE_CONFIGURATION_TYPES="Release;Debug" \
  -DCMAKE_C_COMPILER_LAUNCHER="ccache" \
  -DCMAKE_CXX_COMPILER_LAUNCHER="ccache" \
  -DCMAKE_INSTALL_PREFIX="../install"
```

### iOS

Two separate release binaries are provided, one with release build and one
with debug build. To achieve that `Ninja Multi-Config` generator is used.
Both device and simulator builds are supported.
OS deployment target version is set to 14.0.

To replicate, run:

```shell
cmake ../maplibre-native-qt -G "Ninja Multi-Config" \
  -DCMAKE_CONFIGURATION_TYPES="Release;Debug" \
  -DCMAKE_C_COMPILER_LAUNCHER="ccache" \
  -DCMAKE_CXX_COMPILER_LAUNCHER="ccache" \
  -DCMAKE_INSTALL_PREFIX="../install" \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET="14.0"
```

### Android

Release binaries contain debug symbols. Each ABI is built separately.

To replicate, run:

```shell
cmake ../maplibre-native-qt -G Ninja \
  -DCMAKE_BUILD_TYPE="RelWithDebInfo" \
  -DCMAKE_C_COMPILER_LAUNCHER="ccache" \
  -DCMAKE_CXX_COMPILER_LAUNCHER="ccache" \
  -DCMAKE_INSTALL_PREFIX="../install" \
  -DANDROID_ABI="arm64-v8a"
```

### WebAssembly (WASM)

No official binaries are provided for WebAssembly.
You can build it yourself using the Emscripten toolchain.
The Qt Location module has to be disabled as it is not supported.

To replicate, run:

```shell
cmake ../maplibre-native-qt -G Ninja \
  -DCMAKE_BUILD_TYPE="RelWithDebInfo" \
  -DCMAKE_C_COMPILER_LAUNCHER="ccache" \
  -DCMAKE_CXX_COMPILER_LAUNCHER="ccache" \
  -DCMAKE_INSTALL_PREFIX="../install" \
  -DMLN_QT_LOCATION=OFF
```

## Using CMake workflows

CMake workflow presets are provided for all supported platforms.
They can be simply used by running in the root directory of the repository:

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

For Android, `ANDROID_ABI` environment variable should be set.

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

| Platform | Workflow           | Description                                      |
|----------|--------------------|--------------------------------------------------|
| Linux    | `Linux-coverage`   | Linux build with Qt6, `ccache` and code coverage |
| macOS    | `macOS-clang-tidy` | macOS build with Qt6, `ccache` and `clang-tidy`  |

<div class="section_buttons">

| Previous                          |              Next |
|:----------------------------------|------------------:|
| [Documentation](Documentation.md) | [Usage](Usage.md) |

</div>
