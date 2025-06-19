#include <QQmlExtensionPlugin>
#include <qqml.h>
#include "maplibre_quick_item.hpp"

class MapLibreQuickPlugin : public QQmlExtensionPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)
public:
    void registerTypes(const char *) override {}
};

#include "plugin.moc" 