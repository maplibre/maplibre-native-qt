# Build instructions (iOS Metal example)

Prerequisites similar to macOS Metal build, with iOS-specific requirements:
- Xcode with iOS SDK
- Qt 6 for iOS
- CMake 3.19+

## Build MapLibre Native Qt for iOS

From the root of maplibre-native-qt:

```bash
# Create iOS build directory
mkdir -p qmaplibre-build-ios && cd qmaplibre-build-ios

# Configure for iOS with Metal
qt-cmake ../ -G Xcode \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_OSX_ARCHITECTURES="arm64" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0 \
  -DCMAKE_INSTALL_PREFIX="../qmaplibre-install-ios" \
  -DMLN_WITH_METAL=ON \
  -DMLN_WITH_OPENGL=OFF \
  -DMLN_QT_WITH_LOCATION=OFF \
  -DMLN_WITH_CORE_ONLY=ON \
  -DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM="YOUR_TEAM_ID"

# Build
cmake --build . --config Release

# Install
cmake --install . --config Release
```

### Install MapLibre.Quick Plugin

Create the plugin structure for iOS:

```bash
# Create the expected MapLibre/Quick folder in build tree
mkdir -p qmaplibre-build-ios/MapLibre/Quick

# Copy plugin files
cp qmaplibre-build-ios/src/quick/qmldir \
   qmaplibre-build-ios/MapLibre/Quick/

cp qmaplibre-build-ios/src/quick/libmaplibre_quickplugin.a \
   qmaplibre-build-ios/MapLibre/Quick/
```

## Build the Quick Example for iOS

### Configure the example

```bash
cd examples/quick-standalone
mkdir build-ios && cd build-ios

# Configure for iOS
qt-cmake .. -G Xcode \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_OSX_ARCHITECTURES="arm64" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0 \
  -DQMapLibre_DIR="$(pwd)/../../../qmaplibre-install-ios/lib/cmake/QMapLibre" \
  -DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM="YOUR_TEAM_ID" \
  -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY="iPhone Developer"

# Build
cmake --build . --config Release
```

### Deploy to iOS Device/Simulator

#### For Simulator:
```bash
# Set to simulator SDK
cmake .. -DCMAKE_OSX_SYSROOT=iphonesimulator

# Build and run
cmake --build . --config Release

# Install on simulator
xcrun simctl install booted Release-iphonesimulator/QMapLibreExampleQuickStandalone.app
xcrun simctl launch booted com.maplibre.QMapLibreExampleQuickStandalone
```

#### For Device:
1. Open the Xcode project:
```bash
open QMapLibreExampleQuickStandalone.xcodeproj
```

2. Select your development team in project settings
3. Select your iOS device as target
4. Build and run from Xcode

### Environment Setup for iOS

The Metal backend should be automatically selected on iOS. If needed, you can verify:

```bash
# In Info.plist, add if needed:
<key>QSG_RHI_BACKEND</key>
<string>metal</string>
```

## Troubleshooting

### Code Signing Issues
- Ensure you have a valid Apple Developer account
- Set DEVELOPMENT_TEAM_ID in CMake configuration
- For testing on simulator, you can disable signing:
  ```bash
  -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED="NO"
  ```

### Bundle Resources
- Ensure the MapLibre.bundle is properly included
- Check that Assets.car is generated for iOS icons

### Metal Validation
- Enable Metal validation in Xcode scheme for debugging
- Check console for Metal-related errors

## Notes

- iOS requires arm64 architecture for devices
- Minimum deployment target is iOS 14.0 for Metal support
- The Quick plugin must be statically linked for iOS
- Resource bundles need special handling in iOS apps