# Usage

[TOC]

MapLibre Native can be used with Qt either with C++ or with QML.
The following sections will guide you through the process of setting up
a project and using MapLibre Native with it.

## Examples

Two example apps are provided for Qt 6 in the `examples` directory
of the repository:

- `quick` is a QML app that uses the `MapView` QML type.
- `widgets` is a C++ app that uses the `QMapLibre::GLWidget` widget.

```shell
export QMapLibre_DIR="<absolute-path-to-install>"
cmake --workflow --preset default
```

or alternatively

```shell
mkdir build-example && cd build-example
qt-cmake ../maplibre-native-qt/examples/<example> -GNinja \
  -DCMAKE_C_COMPILER_LAUNCHER="ccache" \
  -DCMAKE_CXX_COMPILER_LAUNCHER="ccache" \
  -DCMAKE_PREFIX_PATH="<absolute-path-to-install>"
ninja
```

## Basic CMake setup

To use MapLibre Native in a Qt project built with CMake you can
use the `find_package` command to locate the package `QMapLibre` and then link
the target to the `QMapLibre::Core`, `QMapLibre::Location`
or `QMapLibre::Widgets` targets. Only specific components can be looked for
by using the `COMPONENTS` option. For example, to use the `Widgets` component
you need to add the following lines to your `CMakeLists.txt`:

```cmake
find_package(QMapLibre COMPONENTS Widgets REQUIRED)

target_link_libraries(
    MyApplication
    PRIVATE
        QMapLibre::Widgets
)
```

There are multiple options to make CMake find the package:

- set the `QMapLibre_DIR` variable to the directory where the package
  is installed ending with `<path>/lib/cmake/QMapLibre`
- set the `CMAKE_PREFIX_PATH` variable to the directory where the package
  is installed

## Widgets setup

No special additional setup is needed. `QMapLibre` and `QMapLibreWidgets`
libraries need to be provided with your application.

## QML setup

To use MapLibre Native in a QML project you need to ensure that the plugins
are installed with your application:

- `plugins/geoservices` for the mapping engine itself
- `qml/MapLibre` for the QML types

Of course also `QMapLibre` and `QMapLibreLocation`
libraries need to be provided with your application.

This can be ensured automatically using the helper function
`qmaplibre_location_setup_plugins(<target>)` which is available once
MapLibre Native is found by CMake.

@warning Only OpenGL backend is supported for now!

For example, to use MapLibre in a CMake-based QML project you need to add

```cmake
find_package(QMapLibre COMPONENTS Location REQUIRED)

target_link_libraries(
    MyApplication
    PRIVATE
        QMapLibre::Location
)

qmaplibre_location_setup_plugins(MyApplication)
```

## Development specifics

Once your application is deployed there should be no special environment
or other overrides needed to run the application.
However, during development you may need to set some environment variables:

- `QSG_RHI_BACKEND=opengl` to force the OpenGL backend
- `DYLD_LIBRARY_PATH` to find libraries on macOS
- `LD_LIBRARY_PATH` to find libraries on Linux
- `QML_IMPORT_PATH=<installation-path>/qml` to find the QML types
  (`QML2_IMPORT_PATH` for Qt 5)
- `QT_PLUGIN_PATH=<installation-path>plugins` to find the plugins

## Platform specifics

### Android

For Android multi-ABI builds you need to ensure that the correct build
is picked-up. In CMake this can be done like

```cmake
if(ANDROID AND DEFINED ENV{QMapLibre_Android_DIR})
    message(STATUS "Setting QMapLibre_DIR to $ENV{QMapLibre_Android_DIR}/${ANDROID_ABI}/lib/cmake/QMapLibre")
    set(QMapLibre_DIR "$ENV{QMapLibre_Android_DIR}/${ANDROID_ABI}/lib/cmake/QMapLibre")
endif()
```

<div class="section_buttons">

| Previous                    |                  Next |
|:----------------------------|----------------------:|
| [How to build](Building.md) | [Topics](topics.html) |

</div>
