#pragma once

#include "maplibre_quick_item_base.hpp"

namespace QMapLibreQuick {

/**
 * @brief Vulkan backend implementation for MapLibre Quick item
 */
class MapLibreQuickItemVulkan : public MapLibreQuickItemBase {
    Q_OBJECT
    QML_NAMED_ELEMENT(MapLibreView)
    
public:
    MapLibreQuickItemVulkan();
    ~MapLibreQuickItemVulkan() override = default;

protected:
    QSGNode* renderFrame(QSGNode* oldNode) override;
    void initializeBackend() override;
    void cleanupBackend() override;

private:
    // Vulkan-specific member variables would go here
    // For now, this is a placeholder implementation
};

// Type alias for the actual MapLibreQuickItem when using Vulkan
using MapLibreQuickItem = MapLibreQuickItemVulkan;

} // namespace QMapLibreQuick
