#pragma on /**                                                            \
            * @brief Metal backend implementation for MapLibre Quick item \
            */
class MapLibreQuickItemMetal : public MapLibreQuickItemBase {
    Q_OBJECT
    QML_NAMED_ELEMENT(MapLibreView)

public:
    MapLibreQuickItemMetal();
    ~MapLibreQuickItemMetal() override;
    ude "maplibre_quick_item_base.hpp"
#if defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#endif

        namespace QMapLibreQuick {

        /**
         * @brief Metal backend implementation for MapLibre Quick item (Apple platforms)
         */
        class MapLibreQuickItemMetal : public MapLibreQuickItemBase {
            Q_OBJECT

        public:
            MapLibreQuickItemMetal();
            ~MapLibreQuickItemMetal() override;

        protected:
            QSGNode* renderFrame(QSGNode* oldNode) override;
            void initializeBackend() override;
            void cleanupBackend() override;

        private:
#if defined(__APPLE__)
            void* m_layerPtr = nullptr;        // persistent CAMetalLayer pointer we pass to MapLibre
            bool m_ownsLayer = false;          // true when we created the fallback CAMetalLayer
            void* m_currentDrawable = nullptr; // retained CAMetalDrawable until frame ends
#endif
        };

        // Type alias for the actual MapLibreQuickItem on Apple platforms
        using MapLibreQuickItem = MapLibreQuickItemMetal;

    } // namespace QMapLibreQuick
