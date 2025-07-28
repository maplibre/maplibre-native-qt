#pragma once

// MapLibre Quick Item - Main header that selects the appropriate backend implementation

#if defined(MLN_RENDER_BACKEND_OPENGL)
#include "maplibre_quick_item_opengl.hpp"
#elif defined(MLN_RENDER_BACKEND_VULKAN)
#include "maplibre_quick_item_vulkan.hpp"
#elif defined(MLN_RENDER_BACKEND_METAL)
#include "maplibre_quick_item_metal.hpp"
#endif

// The MapLibreQuickItem type alias is defined in the specific backend headers
