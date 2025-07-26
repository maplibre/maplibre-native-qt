#pragma once

#if defined(MLN_RENDER_BACKEND_VULKAN)

#include <QtCore/QtGlobal>
#include <QtGui/QVulkanInstance>
#include <QtGui/QWindow>
#include <mbgl/gfx/context.hpp>
#include <mbgl/gfx/texture2d.hpp>
#include <mbgl/vulkan/renderable_resource.hpp>
#include <mbgl/vulkan/renderer_backend.hpp>

class QVulkanWindow;

namespace mbgl {
namespace vulkan {
class Texture2D;
}
} // namespace mbgl

namespace QMapLibre {

class VulkanRendererBackend final : public mbgl::vulkan::RendererBackend, public mbgl::vulkan::Renderable {
public:
    explicit VulkanRendererBackend(QWindow* window);
    explicit VulkanRendererBackend(QVulkanInstance* instance);
    // Constructor that uses Qt's Vulkan device
    VulkanRendererBackend(QWindow* window,
                          VkPhysicalDevice qtPhysicalDevice,
                          VkDevice qtDevice,
                          uint32_t qtGraphicsQueueIndex);
    // Fallback ctor used by MapRenderer when only a ContextMode is provided.
    explicit VulkanRendererBackend(mbgl::gfx::ContextMode /*mode*/);
    ~VulkanRendererBackend() override;

    // mbgl::gfx::RendererBackend ------------------------------------------------
    mbgl::gfx::Renderable& getDefaultRenderable() override { return static_cast<mbgl::gfx::Renderable&>(*this); }
    void activate() override {}
    void deactivate() override {}

    // Qt-specific --------------------------------------------------------------
    void setSize(mbgl::Size size_);
    mbgl::Size getSize() const;

    // Returns the color texture of the drawable rendered in the last frame.
    void* currentDrawable() const { return m_currentDrawable; }

    void _q_setCurrentDrawable(void* tex) { m_currentDrawable = static_cast<mbgl::gfx::Texture2D*>(tex); }

    // Qt Widgets path still expects this hook even though Vulkan doesn't use an
    // OpenGL FBO. Update the size for Vulkan rendering.
    void updateFramebuffer(quint32 /*fbo*/, const mbgl::Size& newSize);

    // Helper method to get the texture object for pixel data extraction
    mbgl::vulkan::Texture2D* getOffscreenTexture() const;

protected:
    // Override base class methods to use external Vulkan resources
    void init();
    void initInstance() override;
    void initSurface() override;
    void initDevice() override;
    void initSwapchain() override;
    std::vector<const char*> getDeviceExtensions() override;

private:
    mbgl::gfx::Texture2D* m_currentDrawable{nullptr};
    QVulkanInstance* m_qtInstance{nullptr};
    QVulkanInstance* m_ownedInstance{nullptr}; // Instance we created and own
    QWindow* m_window{nullptr};                // Qt Quick window

    // Qt device info
    VkPhysicalDevice m_qtPhysicalDevice{VK_NULL_HANDLE};
    VkDevice m_qtDevice{VK_NULL_HANDLE};
    uint32_t m_qtGraphicsQueueIndex{0};
    bool m_useQtDevice{false};

    void initializeWithQtInstance(QVulkanInstance* qtInstance);

    VulkanRendererBackend(const VulkanRendererBackend&) = delete;
    VulkanRendererBackend& operator=(const VulkanRendererBackend&) = delete;
};

} // namespace QMapLibre

#endif // MLN_RENDER_BACKEND_VULKAN
