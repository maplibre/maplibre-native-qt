# Build instructions (Vulkan backend)

The Vulkan backend provides high-performance rendering using the Vulkan graphics API. This guide covers building and running MapLibre Native Qt with Vulkan support on Linux.

## Prerequisites

- Qt 6.10.0 or later with Vulkan support
- Vulkan SDK installed
- Vulkan-capable graphics drivers
- CMake 3.19 or later
- Ninja build system

## Setup Environment

Setup path variables for Qt:

```sh
export PATH=/home/birks/Qt/6.10.0/gcc_arm64/bin:$PATH
```

Verify Vulkan support:

```sh
vulkaninfo --summary
```

## Build QMapLibre with Vulkan Backend

In order to run examples, first build the QMapLibre project with Vulkan support. Use the build dir `/home/birks/repos/maplibre-native-qt/qmaplibre-build-vulkan` and install dir `/home/birks/repos/maplibre-native-qt/qmaplibre-install-vulkan`.

### 1. Configure with CMake

```sh
mkdir -p /home/birks/repos/maplibre-native-qt/qmaplibre-build-vulkan
cd /home/birks/repos/maplibre-native-qt && cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DMLN_WITH_VULKAN=ON \
  -DMLN_WITH_OPENGL=OFF \
  -DMLN_WITH_QT=ON \
  -DMLN_QT_WITH_LOCATION=OFF \
  -DMLN_WITH_WERROR=OFF \
  -DCMAKE_INSTALL_PREFIX="qmaplibre-install-vulkan" \
  -DCMAKE_TOOLCHAIN_FILE=/home/birks/Qt/6.10.0/gcc_arm64/lib/cmake/Qt6/qt.toolchain.cmake \
  -G Ninja \
  -B qmaplibre-build-vulkan
```

### 2. Build and Install

```sh
cd /home/birks/repos/maplibre-native-qt
cmake --build qmaplibre-build-vulkan --parallel 4
cmake --install qmaplibre-build-vulkan
```

### 3. Manually Install MapLibre.Quick Plugin

```sh
mkdir -p /home/birks/repos/maplibre-native-qt/qmaplibre-build-vulkan/src/quick/MapLibre/Quick && cd /home/birks/repos/maplibre-native-qt/qmaplibre-build-vulkan/src/quick && cp qmldir libmaplibre_quickplugin.so maplibre_quick.qmltypes MapLibre/Quick/
```

## Qt Quick Example with Vulkan

## Running the Qt Quick Example

**⚠️ CRITICAL: You MUST build the Quick example with the MLN_WITH_VULKAN flag to enable Vulkan support.**

### Why MLN_WITH_VULKAN is Essential

The `MLN_WITH_VULKAN` CMake flag is **not just a build optimization** - it's a **required conditional compilation flag** that:

1. **Enables Vulkan Code Paths**: Without this flag, the application will **not** use the Vulkan backend, even if Qt is configured for Vulkan RHI
2. **Sets Graphics API**: Controls whether Qt Quick uses `QSGRendererInterface::VulkanRhi` or `QSGRendererInterface::OpenGLRhi`
3. **Defines Preprocessor Macros**: Required for `#if defined(MLN_WITH_VULKAN)` conditional compilation blocks

### Build Configuration Comparison

| Build Configuration | Graphics API | Result |
|---------------------|-------------|--------|
| `cmake -DMLN_WITH_VULKAN=ON ..` | VulkanRhi | ✅ **Works**: Shows blue square, uses Vulkan backend |
| `cmake ..` (without flag) | OpenGLRhi | ❌ **Fails**: No window surface, OpenGL context issues |

### Building the Quick Example with Vulkan

```sh
# Create build directory with Vulkan support
mkdir -p /home/birks/repos/maplibre-native-qt/examples/quick/build-vulkan
cd /home/birks/repos/maplibre-native-qt/examples/quick/build-vulkan

# CRITICAL: Configure with MLN_WITH_VULKAN flag
cmake -DMLN_WITH_VULKAN=ON -DMLN_RENDER_BACKEND_VULKAN=1 ..

# Build
make
```

### Running the Application

Now that the Vulkan backend is successfully integrated, you can run the Qt Quick example:

```sh
cd /home/birks/repos/maplibre-native-qt/examples/quick/build-vulkan
QML2_IMPORT_PATH=/home/birks/repos/maplibre-native-qt/qmaplibre-build-vulkan/src/quick QSG_RHI_BACKEND=vulkan /home/birks/repos/maplibre-native-qt/examples/quick/build-vulkan/QMapLibreExampleQuick
```


### Working Solutions

The Qt Quick integration now works properly with:

#### Current Approach: Qt Quick with Vulkan RHI (Working)
The Vulkan backend now works with Qt Quick applications using the Vulkan RHI backend:

1. **Set Qt Quick to use Vulkan**: `QSG_RHI_BACKEND=vulkan`
2. **Set plugin path**: `QML2_IMPORT_PATH=/path/to/maplibre-build-vulkan/src/quick`
3. Configure with the MLN_WITH_VULKAN cmake flag
4. **Run the example**: The application will create a Qt Quick window with Vulkan rendering

#### Option 2: Use with Qt Widgets (Alternative)
The Vulkan backend works better with Qt Widgets applications where you have more control over the rendering surface.

#### Option 3: Build Qt with Vulkan Support (Advanced)
Ensure your Qt installation includes Vulkan support by building Qt with:
```sh
./configure -vulkan
```

#### Option 4: Use QVulkanWindow Directly (Advanced)
For applications that need direct Vulkan control, use QVulkanWindow instead of Qt Quick.

### Future Development

To optimize the Qt Quick integration, the following improvements are planned:

1. **Rendering Optimization**: Reduce excessive render calls and improve performance
2. **Texture Integration**: Replace placeholder rectangle with actual map texture rendering
3. **Cleanup Improvements**: Fix segmentation fault on application exit
4. **Memory Management**: Optimize Vulkan resource management for better stability

### Alternative: Test with Widgets Backend

For immediate testing, you can use the widgets backend which has better Vulkan support:

```sh
# Build widgets example
cd /home/birks/repos/maplibre-native-qt/examples/widgets
mkdir -p build-vulkan
cd build-vulkan
cmake .. -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=/home/birks/Qt/6.10.0/gcc_arm64 \
  -DQMapLibre_DIR=/home/birks/repos/maplibre-native-qt/qmaplibre-install-vulkan/lib64/cmake/QMapLibre
make -j$(nproc)
```


## Troubleshooting

### Vulkan Validation Layers

For debugging, enable Vulkan validation layers:

```sh
export VK_LAYER_PATH=/usr/share/vulkan/explicit_layer.d
export VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation
```

### Common Issues

1. **No window available for Vulkan surface creation**: This occurs when the Qt Quick window is not properly configured for Vulkan. The window needs explicit Vulkan setup.

2. **Qt Scene Graph uses OpenGL instead of Vulkan**: This happens when Qt was not built with Vulkan support or the application doesn't request Vulkan properly.

3. **No Vulkan support**: Ensure your graphics drivers support Vulkan and the Vulkan SDK is installed

4. **Qt Vulkan headers missing**: Make sure Qt was compiled with Vulkan support

5. **Validation layer errors**: Check that your Vulkan drivers are up to date

### Graphics Driver Requirements

- **NVIDIA**: Driver 367.27 or later
- **AMD**: Driver 16.50 or later
- **Intel**: Driver 100.4169 or later (Linux), 15.45.23.4860 or later (Windows)

### Environment Variables

Useful environment variables for debugging:

```sh
export VK_LOADER_DEBUG=all           # Vulkan loader debug output
export QSG_INFO=1                    # Qt Scene Graph debug info
export QT_LOGGING_RULES="qt.scenegraph.vulkan.debug=true"  # Qt Vulkan debug
```

## Performance Notes

- Vulkan backend provides lower CPU overhead compared to OpenGL
- Best performance on systems with dedicated graphics cards
- Consider using multiple queues for advanced performance optimization
- Monitor memory usage as Vulkan gives more direct control over GPU memory

## Backend Comparison

| Feature | Vulkan | OpenGL | Metal |
|---------|---------|---------|---------|
| Performance | High | Medium | High |
| CPU Overhead | Low | Medium | Low |
| Platform Support | Linux, Windows, Android | Cross-platform | macOS, iOS |
| Driver Maturity | Good | Excellent | Excellent |
| Debugging Tools | Good | Excellent | Good |

## Quick Example Configuration

When building the Qt Quick example with Vulkan support, it's **crucial** to set the `MLN_WITH_VULKAN` CMake flag. This flag enables the Vulkan-specific code paths in the application.

### Why MLN_WITH_VULKAN is Essential

The Qt Quick example uses conditional compilation with `#if defined(MLN_WITH_VULKAN)` to determine which graphics API to use:

- **With MLN_WITH_VULKAN=ON**: Creates Vulkan instance and sets Qt Quick to use Vulkan RHI
- **Without MLN_WITH_VULKAN**: Falls back to OpenGL or Metal (on macOS)

### Configure Qt Quick Example with Vulkan

```sh
cd /home/birks/repos/maplibre-native-qt/examples/quick
mkdir -p build-vulkan
cd build-vulkan

# IMPORTANT: Include MLN_WITH_VULKAN=ON
cmake .. -DCMAKE_BUILD_TYPE=Release \
  -DMLN_WITH_VULKAN=ON \
  -DMLN_RENDER_BACKEND_VULKAN=1 \
  -DCMAKE_PREFIX_PATH=/home/birks/Qt/6.10.0/gcc_arm64 \
  -DQMapLibre_DIR=/home/birks/repos/maplibre-native-qt/qmaplibre-install-vulkan/lib64/cmake/QMapLibre

make -j$(nproc)
```

### Verify Vulkan is Active

When running with Vulkan properly configured, you should see:

```
Set Qt Quick graphics API to Vulkan RHI
```

Instead of:

```
Set Qt Quick graphics API to OpenGL RHI
```

### Common Configuration Mistakes

1. **Forgetting MLN_WITH_VULKAN=ON**: Results in OpenGL fallback
2. **Wrong QMapLibre_DIR**: Points to OpenGL build instead of Vulkan build
3. **Missing Vulkan drivers**: Causes runtime failures


# Other relevant files for vulkan, (and other backends), that are upstream to the quick item

src/core/* , in particularly the

src/core/map.cpp
src/core/map.hpp
src/core/map_p.hpp
src/core/map_renderer.cpp
src/core/map_renderer_p.hpp
