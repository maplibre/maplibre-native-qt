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
               ../../vendor/maplibre-native/include \
               ../../vendor/maplibre-native/vendor/maplibre-native-base/include \
               ../../vendor/maplibre-native/vendor/maplibre-native-base/deps/geometry.hpp/include \
               ../../vendor/maplibre-native/vendor/maplibre-native-base/deps/variant/include \
               ../../vendor/maplibre-native/vendor/metal-cpp

# Link against our built libraries
LIBS += -L$$PWD/../../qmaplibre-build-ios/src/core -lQMapLibre
LIBS += -L$$PWD/../../qmaplibre-build-ios/src/location -lQMapLibreLocation
LIBS += -L$$PWD/../../qmaplibre-build-ios/src/location/plugins -lqtgeoservices_maplibre
LIBS += -L$$PWD/../../qmaplibre-build-ios/src/location/plugins/MapLibre -ldeclarative_locationplugin_maplibre
LIBS += -L$$PWD/../../qmaplibre-build-ios/src/quick -lquick_maplibre
LIBS += -L$$PWD/../../qmaplibre-build-ios/src/core/MapLibreCore/vendor/freetype -lfreetype
LIBS += -L$$PWD/../../qmaplibre-build-ios/src/core/MapLibreCore/vendor/harfbuzz -lharfbuzz

# iOS frameworks
ios {
    LIBS += -framework Metal -framework MetalKit -framework QuartzCore 
    LIBS += -framework CoreLocation -framework CoreGraphics -framework CoreText
    LIBS += -framework ImageIO -framework MobileCoreServices -framework UIKit
    LIBS += -framework Foundation -framework CoreFoundation -framework Security
    LIBS += -framework SystemConfiguration -framework CFNetwork
    LIBS += -lbz2 -lz -lc++  # Add bzip2 for freetype and other system libs
}

# Bundle QML plugin files (disabled for now)
# QML_PLUGIN.files = $$PWD/../../qmaplibre-build-ios/src/quick/MapLibre
# QML_PLUGIN.path = Resources/qml
# QMAKE_BUNDLE_DATA += QML_PLUGIN