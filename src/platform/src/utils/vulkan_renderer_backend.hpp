#pragma once

#if defined(MLN_RENDER_BACKEND_VULKAN)

#include <QtCore/QtGlobal>
#include <QtGui/QVulkanInstance>
#include <QtGui/QWindow>
#include <mbgl/gfx/context.hpp>
#include <mbgl/gfx/renderable.hpp>
#include <mbgl/vulkan/renderer_backend.hpp>

namespace mbgl { namespace vulkan { class Texture2D; } }

namespace QMapLibre {

class VulkanRendererBackend final : public mbgl::vulkan::RendererBackend, public mbgl::gfx::Renderable {
public:
    explicit VulkanRendererBackend(QWindow* window);
    explicit VulkanRendererBackend(QVulkanInstance* instance);
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

    void _q_setCurrentDrawable(void* tex) { m_currentDrawable = tex; }

    // Qt Widgets path still expects this hook even though Vulkan doesn't use an
    // OpenGL FBO. Provide a no-op so code that is agnostic of the backend can
    // compile unmodified.
    void updateFramebuffer(quint32 /*fbo*/, const mbgl::Size& /*size*/) {}

    // Helper method to get the texture object for pixel data extraction
    mbgl::vulkan::Texture2D* getOffscreenTexture() const;

private:
    void* m_currentDrawable{nullptr}; // VkImage or Texture2D*
    VulkanRendererBackend(const VulkanRendererBackend&) = delete;
    VulkanRendererBackend& operator=(const VulkanRendererBackend&) = delete;
};

} // namespace QMapLibre

#endif // MLN_RENDER_BACKEND_VULKAN
