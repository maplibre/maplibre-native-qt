TEMPLATE = app
TARGET = MapLibreQuickiOS

QT += core gui quick qml positioning network sql

CONFIG += c++20 sdk_no_version_check

ios {
    QMAKE_INFO_PLIST = Info.plist.ios
    QMAKE_IOS_DEPLOYMENT_TARGET = 14.0
    QMAKE_TARGET_BUNDLE_PREFIX = com.maplibre
    QMAKE_BUNDLE = MapLibreQuickiOS
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

# Link against our built libraries (device build)
LIBS += -L$$PWD/../../qmaplibre-build-ios-device/src/core -lQMapLibre
LIBS += -L$$PWD/../../qmaplibre-build-ios-device/src/quick -lquick_maplibre
LIBS += -L$$PWD/../../qmaplibre-build-ios-device/src/core/MapLibreCore/vendor/freetype -lfreetype
LIBS += -L$$PWD/../../qmaplibre-build-ios-device/src/core/MapLibreCore/vendor/harfbuzz -lharfbuzz

# Add plugin object files
OBJECTS += $$PWD/../../qmaplibre-build-ios-device/src/quick/CMakeFiles/quick_maplibreplugin.dir/quick_maplibreplugin_MapLibreQuickModule.cpp.o
OBJECTS += $$PWD/../../qmaplibre-build-ios-device/src/quick/CMakeFiles/quick_maplibreplugin.dir/quick_maplibreplugin_autogen/mocs_compilation.cpp.o
OBJECTS += $$PWD/../../qmaplibre-build-ios-device/src/quick/CMakeFiles/quick_maplibreplugin_init.dir/quick_maplibreplugin_init.cpp.o
OBJECTS += $$PWD/../../qmaplibre-build-ios-device/src/quick/CMakeFiles/quick_maplibreplugin_init.dir/quick_maplibreplugin_init_autogen/mocs_compilation.cpp.o

# Add QML resource objects
OBJECTS += $$PWD/../../qmaplibre-build-ios-device/src/quick/CMakeFiles/quick_maplibre_resources_1.dir/.qt/rcc/qrc_qmake_MapLibre_Quick_init.cpp.o
OBJECTS += $$PWD/../../qmaplibre-build-ios-device/src/quick/CMakeFiles/quick_maplibre_resources_2.dir/.qt/rcc/qrc_quick_maplibre_raw_qml_0_init.cpp.o

# iOS frameworks
ios {
    LIBS += -framework Metal -framework MetalKit -framework QuartzCore 
    LIBS += -framework CoreLocation -framework CoreGraphics -framework CoreText
    LIBS += -framework ImageIO -framework MobileCoreServices -framework UIKit
    LIBS += -framework Foundation -framework CoreFoundation -framework Security
    LIBS += -framework SystemConfiguration -framework CFNetwork
    LIBS += -lbz2 -lz -lc++
}