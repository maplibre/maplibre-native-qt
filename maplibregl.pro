TARGET = qtgeoservices_maplibregl

QT += \
    quick-private \
    location-private \
    positioning-private \
    network \
    sql \
    sql-private

HEADERS += \
    src/qgeoserviceproviderpluginmaplibregl.h \
    src/qgeomappingmanagerenginemaplibregl.h \
    src/qgeomapmaplibregl.h \
    src/qgeomapmaplibregl_p.h \
    src/qmaplibreglstylechange_p.h \
    src/qsgmaplibreglnode.h

SOURCES += \
    src/qgeoserviceproviderpluginmaplibregl.cpp \
    src/qgeomappingmanagerenginemaplibregl.cpp \
    src/qgeomapmaplibregl.cpp \
    src/qmaplibreglstylechange.cpp \
    src/qsgmaplibreglnode.cpp

# QMapLibreGL Native is always a static
# library linked to this plugin
QMAKE_CXXFLAGS += -DQT_MAPLIBREGL_STATIC

RESOURCES += resources/maplibregl.qrc

OTHER_FILES += maplibregl_plugin.json

# zlib dependency satisfied by bundled 3rd party zlib or system zlib
qtConfig(system-zlib) {
    QMAKE_USE_PRIVATE += zlib
} else {
    QT_PRIVATE += zlib-private
}

load(qt_build_paths)

!isEmpty(QMAPLIBREGL_PATH) {
    message("QMapLibreGL path: $$QMAPLIBREGL_PATH")

    INCLUDEPATH += $$QMAPLIBREGL_PATH/include
    LIBS += -L$$QMAPLIBREGL_PATH/lib
    linux:!android {
        LIBS += -L$$QMAPLIBREGL_PATH/lib64
    }
} else {
    message("QMapLibreGL installed with Qt")
}
LIBS += -lQMapLibreGL$$qtPlatformTargetSuffix()

qtConfig(icu) {
    QMAKE_USE_PRIVATE += icu
}

PLUGIN_TYPE = geoservices
PLUGIN_CLASS_NAME = QGeoServiceProviderFactoryMapLibreGL
load(qt_plugin)
