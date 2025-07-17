#pragma once

#if defined(MLN_RENDER_BACKEND_VULKAN)

#include <mbgl/gfx/renderable.hpp>
#include <mbgl/util/size.hpp>
#include <mbgl/vulkan/renderable_resource.hpp>
#include <mbgl/vulkan/renderer_backend.hpp>
#include <mbgl/vulkan/context.hpp>

#include <QtGui/qvulkaninstance.h>
#include <QtGui/QWindow>

namespace QMapLibre {

class VulkanRendererBackend;

// Renderable resource that integrates Qt's Vulkan surface with MapLibre Native
class QtVulkanRenderableResource final : public mbgl::vulkan::SurfaceRenderableResource {
public:
    explicit QtVulkanRenderableResource(VulkanRendererBackend& backend_);

    std::vector<const char*> getDeviceExtensions() override;
    void createPlatformSurface() override;
    void bind() override;
    
    // Override to provide custom framebuffer for offscreen rendering
    const vk::UniqueFramebuffer& getFramebuffer() const override { return framebuffer; }
    
private:
    void setupQtTextureRendering(void* qtTexture);
    void createDummySwapchain();
    void createOffscreenRenderPass();
    void createOffscreenFramebuffer();
    
    // For offscreen rendering
    vk::UniqueFramebuffer framebuffer;
};

class VulkanRendererBackend : public mbgl::vulkan::RendererBackend, public mbgl::vulkan::Renderable {
public:
    explicit VulkanRendererBackend(QWindow* window);
    // Fallback ctor used by MapRenderer when only a ContextMode is provided.
    explicit VulkanRendererBackend(mbgl::gfx::ContextMode /*mode*/);
    ~VulkanRendererBackend() override;

    // mbgl::gfx::RendererBackend implementation
    mbgl::gfx::Renderable& getDefaultRenderable() override { return *this; }
    void activate() override {}
    void deactivate() override {}

    // Size helpers
    mbgl::Size getSize() const { return size; }
    void setSize(const mbgl::Size newSize);
    
    // Qt integration helpers
    void updateFramebuffer(uint32_t fbo, const mbgl::Size& newSize);

    // Expose the Vk instance to the renderable resource
    vk::Instance const& getInstance() const { return mbgl::vulkan::RendererBackend::getInstance().get(); }
    QWindow* getWindow() const { return window; }
    QVulkanInstance* getQtVulkanInstance() const { return vulkanInstance; }

    // Instance extensions
    std::vector<const char*> getInstanceExtensions() override;

    // Qt-specific texture integration methods (similar to Metal backend)
    void* currentDrawable() const;
    void _q_setCurrentDrawable(void* tex) { m_currentDrawable = tex; }

    // Initialization management
    void ensureInitialized();
    
    // Override init to integrate Qt Vulkan instance
    void init();
    
    // Shadow initSwapchain to skip swapchain creation for Qt Quick integration
    void initSwapchain();

protected:
        // Override createContext to provide standard context
    std::unique_ptr<mbgl::gfx::Context> createContext() override;

private:
    void createOffscreenTexture(const mbgl::Size& size);
    void setupQtVulkanInstance();  // Helper to setup Qt Vulkan instance integration
    
public:
    // Helper method to get the texture object for pixel data extraction
    mbgl::vulkan::Texture2D* getOffscreenTexture() const;
    
    QWindow* window{nullptr};
    QVulkanInstance* vulkanInstance{nullptr};
    mutable void* m_currentDrawable{nullptr};  // Current drawable texture for Qt integration
    mutable std::unique_ptr<mbgl::gfx::OffscreenTexture> m_offscreenTexture;  // Offscreen texture for rendering
    bool m_isInitialized{false};  // Track initialization state
    
    friend class QtVulkanRenderableResource;
};

} // namespace QMapLibre

#endif // MLN_RENDER_BACKEND_VULKAN
