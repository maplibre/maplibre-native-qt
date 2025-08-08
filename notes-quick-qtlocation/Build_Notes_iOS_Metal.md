# Build instructions for quick-qtlocation example on iOS with Metal

This guide covers building and running the quick-qtlocation example with Qt Location integration on iOS using Metal rendering.

## Prerequisites

- macOS with Xcode 15+ installed (tested with Xcode 16.4)
- Qt 6.5+ for iOS (installed via Qt Installer) - Qt 6.9.1 recommended
- CMake 3.25+ (tested with 3.31.5)
- Valid Apple Developer account (for device deployment)
- iOS device running iOS 16.0+ (tested with iOS 18.5)

## Quick Start (Device Deployment)

The simplest approach is to use the pre-configured Xcode project after building the core libraries.

### 1. Build MapLibre Native Qt for iOS

From repository root:

```bash
# Initialize all submodules
git submodule update --init --recursive
cd vendor/maplibre-native
git submodule update --init --recursive
cd ../..

# Set Qt root for iOS
export QT_ROOT_DIR=/Users/$USER/Qt/6.9.1/ios

# Configure using iOS preset
cmake --preset iOS

# Build all libraries
cmake --build qmaplibre-build-ios --config Release -j8
```

### 2. Generate and Build Xcode Project

```bash
cd examples/quick-qtlocation

# Generate Xcode project with qmake
/Users/$USER/Qt/6.9.1/ios/bin/qmake ios-maplibre.pro \
    CONFIG+=iphoneos \
    CONFIG+=device \
    CONFIG+=sdk_no_version_check

# Build for device
xcodebuild -project MapLibreiOS.xcodeproj \
    -scheme MapLibreiOS \
    -sdk iphoneos \
    -configuration Release \
    clean build
```

### 3. Deploy to Device

```bash
# List connected devices
xcrun xctrace list devices

# Install on device (replace with your device ID)
xcrun devicectl device install app --device "DEVICE_ID" Release-iphoneos/MapLibreiOS.app

# Launch app
xcrun devicectl device process launch --device "DEVICE_ID" com.maplibre.MapLibreiOS
```

## Required Files

The quick-qtlocation example only needs these essential files:

```
examples/quick-qtlocation/
├── ios-maplibre.pro       # Main project file
├── main.cpp                # Application entry point
├── main.qml                # QML interface
├── qml.qrc                 # Resource file
├── plugin_import.cpp       # Static plugin imports
└── Info.plist.ios         # iOS app configuration
```

All other files (.xcode/, Makefile, generated code) are temporary and will be regenerated.

## Project Configuration

### iOS Project File (ios-maplibre.pro)

```qmake
TEMPLATE = app
TARGET = MapLibreiOS

QT += core gui quick qml positioning location network sql

CONFIG += c++20 sdk_no_version_check

ios {
    QMAKE_INFO_PLIST = Info.plist.ios
    QMAKE_IOS_DEPLOYMENT_TARGET = 14.0
    QMAKE_TARGET_BUNDLE_PREFIX = com.maplibre
    QMAKE_BUNDLE = MapLibreiOS
}

SOURCES += main.cpp plugin_import.cpp
RESOURCES += qml.qrc

# Include paths  
INCLUDEPATH += ../../src/core \
               ../../src/quick \
               ../../vendor/maplibre-native/include

# Link against built libraries
LIBS += -L$$PWD/../../qmaplibre-build-ios/src/core -lQMapLibre
LIBS += -L$$PWD/../../qmaplibre-build-ios/src/location -lQMapLibreLocation
LIBS += -L$$PWD/../../qmaplibre-build-ios/src/quick -lquick_maplibre
LIBS += -L$$PWD/../../qmaplibre-build-ios/vendor/maplibre-native -lmbgl-core

# iOS frameworks for Metal
ios {
    LIBS += -framework Metal -framework MetalKit -framework QuartzCore 
    LIBS += -framework CoreLocation -framework CoreGraphics -framework CoreText
    LIBS += -framework UIKit -framework Foundation
}
```

### Main QML File (main.qml)

```qml
import QtQuick 6.5
import QtQuick.Window 6.5
import QtLocation 6.5
import QtPositioning 6.5

Window {
    width: Qt.platform.os === "ios" ? Screen.width : 512
    height: Qt.platform.os === "ios" ? Screen.height : 512
    visible: true

    Plugin {
        id: mapPlugin
        name: "maplibre"
        
        // MapLibre demo tiles configuration
        PluginParameter {
            name: "maplibre.map.styles"
            value: "https://demotiles.maplibre.org/style.json"
        }
        
        PluginParameter {
            name: "maplibre.mapping.additional_style_urls"
            value: ""
        }
    }

    MapView {
        anchors.fill: parent
        map.plugin: mapPlugin
        map.center: QtPositioning.coordinate(41.874, -75.789)
        map.zoomLevel: 5
    }
}
```

### Plugin Import Configuration (plugin_import.cpp)

```cpp
// Static plugin imports for iOS
#include <QtPlugin>

// Import iOS platform plugin
Q_IMPORT_PLUGIN(QIOSIntegrationPlugin)

// Import the MapLibre geoservices plugin
Q_IMPORT_PLUGIN(QGeoServiceProviderFactoryMapLibre)

// Import the MapLibre QML Location plugin  
Q_IMPORT_PLUGIN(MapLibreQmlModule)

// Note: MapLibreQuickModule may cause linking errors - comment out if needed
// Q_IMPORT_PLUGIN(MapLibreQuickModule)

// Also import standard Qt Location plugins that might be needed
Q_IMPORT_PLUGIN(QGeoServiceProviderFactoryOsm)
Q_IMPORT_PLUGIN(QGeoServiceProviderFactoryItemsOverlay)
```

## Troubleshooting

### Common Issues and Solutions

1. **'ghc/filesystem.hpp' file not found**
   - Add filesystem include path to mbgl-core in `vendor/maplibre-native/CMakeLists.txt`:
   ```cmake
   target_include_directories(
       mbgl-core
       PRIVATE ${PROJECT_SOURCE_DIR}/src
               ${PROJECT_SOURCE_DIR}/vendor/maplibre-native-base/extras/filesystem/include
   )
   ```

2. **GL_TEXTURE_WIDTH undefined (iOS builds)**
   - These OpenGL constants don't exist in OpenGL ES
   - Wrap with platform checks in `src/widgets/gl_widget.cpp`:
   ```cpp
   #if !defined(Q_OS_IOS) && !defined(Q_OS_ANDROID)
       GLint texWidth, texHeight, texFormat;
       f->glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &texWidth);
       // ...
   #endif
   ```

3. **Undefined symbol: qt_static_plugin_MapLibreQuickModule()**
   - Comment out `Q_IMPORT_PLUGIN(MapLibreQuickModule)` in plugin_import.cpp
   - This plugin may cause linking errors on iOS

4. **CMake policy warnings (CMP0148)**
   - Add to main CMakeLists.txt after cmake_minimum_required:
   ```cmake
   if(POLICY CMP0148)
       cmake_policy(SET CMP0148 NEW)
   endif()
   ```

5. **Code signing errors**
   - Set up development team in Xcode project settings
   - Ensure valid provisioning profile for device deployment

### Verifying Metal Support

Check console output for Metal initialization:
```
Platform: "ios"
Graphics API: 6  // Should be 6 for Metal
```

## Working Configuration (Tested Aug 2025)

Successfully tested with:
- macOS Sequoia 15.6 (Apple Silicon M-series)
- Xcode 16.4
- Qt 6.9.1 for iOS
- CMake 3.31.5
- iOS 18.5 device
- MapLibre demo tiles (https://demotiles.maplibre.org/style.json)

## Key Points

- The build creates arm64 binaries for iOS devices
- Static linking is mandatory for iOS apps
- All Qt plugins must be imported statically via Q_IMPORT_PLUGIN
- The .gitignore is configured to exclude all temporary build files
- Only 6 source files are needed; everything else is generated

## Testing Without a Physical Device

### Running on Mac (Designed for iPad)

On Apple Silicon Macs, you can run the iOS app directly on macOS using the "Designed for iPad" compatibility mode. This is the simplest way to test without a physical iOS device:

1. **Open the project in Xcode:**
   ```bash
   cd examples/quick-qtlocation
   open MapLibreiOS.xcodeproj
   ```

2. **Select the Mac as destination:**
   - In Xcode's toolbar, click the scheme/device selector
   - Choose "My Mac (Designed for iPad)"
   
3. **Run the app:**
   - Click the Run button (▶️) or press Cmd+R
   - The app will launch as an iPad app on your Mac with full Metal support

**Advantages:**
- No need to build for different architectures
- Uses the same arm64 binaries as iOS devices
- Full Metal rendering support
- Native performance on Apple Silicon

**Note:** This method requires an Apple Silicon Mac (M1/M2/M3 etc.). Traditional iOS Simulator support would require building libraries for x86_64 architecture due to Rosetta 2 emulation.