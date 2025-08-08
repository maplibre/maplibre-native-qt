# Build instructions (iOS Metal example)

Prerequisites:
- Xcode with iOS SDK
- Qt 6.9.1+ for iOS
- CMake 3.19+
- ios-deploy (for device deployment): `brew install ios-deploy`

## Build MapLibre Native Qt Core for iOS

From the root of maplibre-native-qt:

```bash
# Configure for iOS device with Metal using preset
cmake --preset default-ios-device

# Build
cmake --build --preset default-ios-device

# The build output will be in qmaplibre-build-ios-device/
```

Alternative manual configuration:
```bash
# Create iOS build directory
mkdir -p qmaplibre-build-ios-device && cd qmaplibre-build-ios-device

# Configure for iOS with Metal
~/Qt/6.9.1/ios/bin/qt-cmake ../ \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_OSX_ARCHITECTURES="arm64" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0 \
  -DMLN_WITH_METAL=ON \
  -DMLN_QT_WITH_LOCATION=OFF

# Build
cmake --build . --config Release
```

## Build the Quick Standalone Example for iOS

The quick-standalone example uses MapLibre.Quick 3.0 directly (without Qt Location).

### Using qmake (Recommended)

```bash
cd examples/quick-standalone

# Generate Xcode project using qmake
~/Qt/6.9.1/ios/bin/qmake quick-ios.pro \
  -spec ~/Qt/6.9.1/ios/mkspecs/macx-ios-clang \
  CONFIG+=sdk_no_version_check

# Build for Release
xcodebuild -project MapLibreQuickiOS.xcodeproj \
  -scheme MapLibreQuickiOS \
  -configuration Release \
  -sdk iphoneos build

# Or build for Debug
xcodebuild -project MapLibreQuickiOS.xcodeproj \
  -scheme MapLibreQuickiOS \
  -configuration Debug \
  -sdk iphoneos build
```

### Deploy to iOS Device

```bash
# Deploy using ios-deploy
ios-deploy --bundle Release-iphoneos/MapLibreQuickiOS.app --no-wifi

# Or for Debug build
ios-deploy --bundle Debug-iphoneos/MapLibreQuickiOS.app --no-wifi
```

### Testing on Mac (Designed for iPad)

For debugging without a physical device:

1. Open the project in Xcode:
```bash
open MapLibreQuickiOS.xcodeproj
```

2. Select "My Mac (Designed for iPad)" as the destination
3. Click Run to build and debug on your Mac

Note: The app runs as an iPad app on macOS, which is useful for development and debugging.

## Project Structure

Key files for iOS build:
- `quick-ios.pro` - Main qmake project file
- `plugin_import.cpp` - Static plugin imports for iOS
- `Info.plist.ios` - iOS app configuration
- `main.cpp` - Application entry point with Metal setup
- `main.qml` - QML interface using MapLibre.Quick

## Important Implementation Details

### Static Plugin Linking

For iOS, the MapLibre Quick plugin must be statically linked. The `quick-ios.pro` file includes:

```qmake
# Link libraries
LIBS += -L$$PWD/../../qmaplibre-build-ios-device/src/core -lQMapLibre
LIBS += -L$$PWD/../../qmaplibre-build-ios-device/src/quick -lquick_maplibre

# Include plugin object files for static linking
OBJECTS += $$PWD/../../qmaplibre-build-ios-device/src/quick/CMakeFiles/quick_maplibreplugin.dir/*.o
OBJECTS += $$PWD/../../qmaplibre-build-ios-device/src/quick/CMakeFiles/quick_maplibre_resources_*.dir/.qt/rcc/*.o
```

### QML Module Registration

In `main.cpp`, register the QML types:

```cpp
extern void qml_register_types_MapLibre_Quick();

int main(int argc, char *argv[]) {
    // Set Metal as graphics API
    QQuickWindow::setGraphicsApi(QSGRendererInterface::Metal);
    
    QGuiApplication app(argc, argv);
    
    // Register MapLibre Quick QML types
    qml_register_types_MapLibre_Quick();
    
    QQmlApplicationEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/"));
    // ...
}
```

## Troubleshooting

### QML Module Not Found
If you get "module MapLibre.Quick is not installed":
- Ensure QML types are registered in main.cpp
- Check that plugin object files are linked
- Verify resource files are included

### Code Signing Issues
- Xcode automatically handles signing for "Designed for iPad" builds
- For device deployment, ensure you have a valid development team set in Xcode

### Metal Validation
- Metal is automatically selected on iOS
- Enable Metal validation in Xcode scheme for debugging
- Check console output for Metal-related errors

## Notes

- iOS requires arm64 architecture for devices
- Minimum deployment target is iOS 14.0 for Metal support
- The Quick plugin must be statically linked for iOS
- All temporary build files are ignored via .gitignore