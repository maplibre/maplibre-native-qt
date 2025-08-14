// Static plugin imports for iOS
#include <QtPlugin>

// Import iOS platform plugin
Q_IMPORT_PLUGIN(QIOSIntegrationPlugin)

// MapLibre Quick plugin is imported via the linked object files
// We just need to ensure the QML types are registered
extern void qml_register_types_MapLibre_Quick();
