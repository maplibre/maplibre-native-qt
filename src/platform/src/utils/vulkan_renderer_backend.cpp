#if defined(MLN_RENDER_BACKEND_VULKAN)

#include "vulkan_renderer_backend.hpp"

#include <mbgl/gfx/backend_scope.hpp>
#include <mbgl/gfx/context.hpp>
#include <mbgl/gfx/offscreen_texture.hpp>
#include <mbgl/vulkan/renderable_resource.hpp>
#include <mbgl/vulkan/texture2d.hpp>

#include <QDebug>
#include <QVulkanInstance>
#include <vulkan/vulkan.hpp>

#include <cassert>

namespace QMapLibre {

namespace {
class QtVulkanRenderableResource final : public mbgl::vulkan::RenderableResource {
public:
    QtVulkanRenderableResource(VulkanRendererBackend& backend_)
        : mbgl::vulkan::RenderableResource(backend_),
          backend(backend_) {}

    void setBackendSize(mbgl::Size size_) {
        size = size_;
        extent.width = size.width;
        extent.height = size.height;

        if (offscreenTexture) {
            // Recreate offscreen texture with new size
            offscreenTexture.reset();
            offscreenTexture = backend.getContext().createOffscreenTexture(
                size, mbgl::gfx::TextureChannelDataType::UnsignedByte);

            // Set the current drawable to the new texture
            if (offscreenTexture) {
                auto texture = offscreenTexture->getTexture();
                if (texture) {
                    texture->create();
                    // Store the texture as the current drawable (Qt Quick will handle VkImage access)
                    backend._q_setCurrentDrawable(texture.get());
                }
            }
        }
    }

    mbgl::Size getSize() const { return size; }

    // Override bind to ensure we have an offscreen texture
    void bind() override {
        if (!offscreenTexture && !size.isEmpty()) {
            offscreenTexture = backend.getContext().createOffscreenTexture(
                size, mbgl::gfx::TextureChannelDataType::UnsignedByte);

            if (offscreenTexture) {
                auto texture = offscreenTexture->getTexture();
                if (texture) {
                    texture->create();
                    // Store the texture as the current drawable (Qt Quick will handle VkImage access)
                    backend._q_setCurrentDrawable(texture.get());
                }
            }
        }

        // Create render pass and framebuffer if needed
        if (offscreenTexture && !renderPass) {
            createRenderPass();
        }
    }

    const vk::UniqueFramebuffer& getFramebuffer() const override { return framebuffer; }

private:
    void createRenderPass() {
        if (!offscreenTexture) return;

        auto texture = offscreenTexture->getTexture();
        if (!texture) return;

        auto& vulkanTexture = static_cast<mbgl::vulkan::Texture2D&>(*texture);

        const auto colorAttachment = vk::AttachmentDescription(vk::AttachmentDescriptionFlags())
                                         .setFormat(vulkanTexture.getVulkanFormat())
                                         .setSamples(vk::SampleCountFlagBits::e1)
                                         .setLoadOp(vk::AttachmentLoadOp::eClear)
                                         .setStoreOp(vk::AttachmentStoreOp::eStore)
                                         .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
                                         .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
                                         .setInitialLayout(vk::ImageLayout::eUndefined)
                                         .setFinalLayout(vk::ImageLayout::eShaderReadOnlyOptimal);

        const vk::AttachmentReference colorAttachmentRef(0, vk::ImageLayout::eColorAttachmentOptimal);

        const auto subpass = vk::SubpassDescription(vk::SubpassDescriptionFlags())
                                 .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
                                 .setColorAttachments(colorAttachmentRef);

        const auto renderPassCreateInfo =
            vk::RenderPassCreateInfo().setAttachments(colorAttachment).setSubpasses(subpass);

        auto& qtBackend = static_cast<VulkanRendererBackend&>(backend);
        renderPass = qtBackend.getDevice()->createRenderPassUnique(renderPassCreateInfo);

        // Create framebuffer
        const auto& imageView = vulkanTexture.getVulkanImageView();
        const auto framebufferCreateInfo = vk::FramebufferCreateInfo()
                                               .setRenderPass(renderPass.get())
                                               .setAttachments(imageView.get())
                                               .setWidth(extent.width)
                                               .setHeight(extent.height)
                                               .setLayers(1);

        framebuffer = qtBackend.getDevice()->createFramebufferUnique(framebufferCreateInfo);
    }

    VulkanRendererBackend& backend;
    std::unique_ptr<mbgl::gfx::OffscreenTexture> offscreenTexture;
    vk::UniqueFramebuffer framebuffer;
    mbgl::Size size{0, 0};
};
} // namespace

VulkanRendererBackend::VulkanRendererBackend(QWindow* window)
    : VulkanRendererBackend(window ? window->vulkanInstance() : nullptr) {}

VulkanRendererBackend::VulkanRendererBackend(QVulkanInstance* qtInstance)
    : mbgl::vulkan::RendererBackend(mbgl::gfx::ContextMode::Unique),
      mbgl::gfx::Renderable(mbgl::Size{0, 0}, std::make_unique<QtVulkanRenderableResource>(*this)) {
    assert(qtInstance);

    // Use Qt's existing Vulkan instance - wrap it for MapLibre
    VkInstance rawInstance = qtInstance->vkInstance();
    if (rawInstance == VK_NULL_HANDLE) {
        throw std::runtime_error("Qt Vulkan instance handle is null");
    }

    // Initialize the dispatch loader
    VULKAN_HPP_DEFAULT_DISPATCHER.init();

    // Wrap Qt's instance
    vk::Instance vkInstance(rawInstance);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkInstance);

    // Set the wrapped instance in the base class
    instance = vk::UniqueInstance(vkInstance, vk::ObjectDestroy<vk::NoParent, vk::DispatchLoaderDynamic>());
}

// Convenience constructor that creates its own QVulkanInstance. Used when only a
// ContextMode is available (e.g. MapRenderer's default path).
VulkanRendererBackend::VulkanRendererBackend(mbgl::gfx::ContextMode)
    : VulkanRendererBackend([]() {
          auto* qtInstance = new QVulkanInstance();
          qtInstance->setApiVersion(QVersionNumber(1, 0));
          if (!qtInstance->create()) {
              throw std::runtime_error("Failed to create QVulkanInstance");
          }
          return qtInstance;
      }()) {}

VulkanRendererBackend::~VulkanRendererBackend() = default;

void VulkanRendererBackend::setSize(mbgl::Size size_) {
    this->getResource<QtVulkanRenderableResource>().setBackendSize(size_);
}

mbgl::Size VulkanRendererBackend::getSize() const {
    return this->getResource<QtVulkanRenderableResource>().getSize();
}

mbgl::vulkan::Texture2D* VulkanRendererBackend::getOffscreenTexture() const {
    // Try to return the current drawable if it's a Texture2D
    if (m_currentDrawable) {
        return static_cast<mbgl::vulkan::Texture2D*>(m_currentDrawable);
    }
    return nullptr;
}

} // namespace QMapLibre

#endif // MLN_RENDER_BACKEND_VULKAN
