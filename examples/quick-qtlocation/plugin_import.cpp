// Static plugin imports for iOS
#include <QtPlugin>

// Import iOS platform plugin
Q_IMPORT_PLUGIN(QIOSIntegrationPlugin)

// Import the MapLibre geoservices plugin
Q_IMPORT_PLUGIN(QGeoServiceProviderFactoryMapLibre)

// Import the MapLibre QML Location plugin  
Q_IMPORT_PLUGIN(MapLibreQmlModule)

// Import the MapLibre Quick plugin (for MapLibre.Quick 3.0)
// Q_IMPORT_PLUGIN(MapLibreQuickModule)

// Also import standard Qt Location plugins that might be needed
Q_IMPORT_PLUGIN(QGeoServiceProviderFactoryOsm)
Q_IMPORT_PLUGIN(QGeoServiceProviderFactoryItemsOverlay)