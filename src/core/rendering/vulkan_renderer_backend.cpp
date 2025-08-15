#include "vulkan_renderer_backend_p.hpp"

// Define storage for the global Vulkan dispatcher
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

#include <mbgl/gfx/backend_scope.hpp>
#include <mbgl/gfx/context.hpp>
#include <mbgl/gfx/offscreen_texture.hpp>
#include <mbgl/vulkan/context.hpp>
#include <mbgl/vulkan/renderable_resource.hpp>
#include <mbgl/vulkan/texture2d.hpp>

#include <QVulkanFunctions>
#include <QVulkanInstance>
#include <QVulkanWindow>
#include <vulkan/vulkan.hpp>

#include <algorithm>
#include <cassert>

namespace QMapLibre {

namespace {
// Custom renderable resource for Qt integration supporting zero-copy GPU rendering
class QtVulkanRenderableResource final : public mbgl::vulkan::SurfaceRenderableResource {
public:
    QtVulkanRenderableResource(VulkanRendererBackend& backend_, mbgl::Size initialSize)
        : mbgl::vulkan::SurfaceRenderableResource(backend_),
          backend(backend_),
          size(initialSize) {
        extent.width = size.width;
        extent.height = size.height;
    }

    void setBackendSize(mbgl::Size size_) {
        // If we're going from invalid to valid size, mark for recreation
        if (size.isEmpty() && !size_.isEmpty()) {
            needsRecreation = true;
        }

        size = size_;
        extent.width = size.width;
        extent.height = size.height;

        if (offscreenTexture && !size.isEmpty()) {
            // Force recreation on next bind if size is now valid
            if (!needsRecreation) {
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

                // Reset render pass and framebuffer to force recreation
                framebuffer.reset();
                renderPass.reset();
            }
        }
    }

    mbgl::Size getSize() const { return size; }

    mbgl::Size getBackendSize() const { return static_cast<const mbgl::gfx::Renderable&>(backend).getSize(); }

    // Override bind to ensure we have an offscreen texture
    void bind() override {
        // Use a minimal size if the actual size is invalid
        mbgl::Size textureSize = size;
        if (size.isEmpty()) {
            textureSize = mbgl::Size{256, 256};
            // Mark that we need to recreate when valid size is available
            needsRecreation = true;
        }

        if (!offscreenTexture || needsRecreation) {
            // Reset existing resources if recreating
            if (needsRecreation) {
                framebuffer.reset();
                renderPass.reset();
                offscreenTexture.reset();
                needsRecreation = false;
            }

            // Create offscreen texture with proper format for zero-copy sharing
            offscreenTexture = backend.getContext().createOffscreenTexture(
                textureSize, mbgl::gfx::TextureChannelDataType::UnsignedByte);

            if (offscreenTexture) {
                auto texture = offscreenTexture->getTexture();
                if (texture) {
                    // Ensure texture is created and ready for GPU operations
                    texture->create();

                    // Store the texture as the current drawable
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

    void swap() override {
        // For offscreen rendering, we need to ensure commands are submitted
        // but we don't present to a swapchain
        mbgl::vulkan::SurfaceRenderableResource::swap();

        // Add barrier to transition image layout to shader read-only optimal
        if (offscreenTexture) {
            auto texture = offscreenTexture->getTexture();
            if (texture) {
                // The barrier will be added by the texture itself when needed
                (void)static_cast<mbgl::vulkan::Texture2D&>(*texture);
            }
        }

        // No need for waitIdle as synchronization is handled by barriers
    }

private:
    void createRenderPass() {
        if (!offscreenTexture) return;

        auto texture = offscreenTexture->getTexture();
        if (!texture) return;

        auto& vulkanTexture = static_cast<mbgl::vulkan::Texture2D&>(*texture);
        auto& qtBackend = static_cast<VulkanRendererBackend&>(backend);

        // Initialize depth/stencil resources if needed
        initDepthStencil();

        const auto colorAttachment = vk::AttachmentDescription(vk::AttachmentDescriptionFlags())
                                         .setFormat(vulkanTexture.getVulkanFormat())
                                         .setSamples(vk::SampleCountFlagBits::e1)
                                         .setLoadOp(vk::AttachmentLoadOp::eClear)
                                         .setStoreOp(vk::AttachmentStoreOp::eStore)
                                         .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
                                         .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
                                         .setInitialLayout(vk::ImageLayout::eUndefined)
                                         .setFinalLayout(vk::ImageLayout::eShaderReadOnlyOptimal);

        const auto depthAttachment = vk::AttachmentDescription(vk::AttachmentDescriptionFlags())
                                         .setFormat(depthFormat != vk::Format::eUndefined ? depthFormat
                                                                                          : vk::Format::eD24UnormS8Uint)
                                         .setSamples(vk::SampleCountFlagBits::e1)
                                         .setLoadOp(vk::AttachmentLoadOp::eClear)
                                         .setStoreOp(vk::AttachmentStoreOp::eDontCare)
                                         .setStencilLoadOp(vk::AttachmentLoadOp::eClear)
                                         .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
                                         .setInitialLayout(vk::ImageLayout::eUndefined)
                                         .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

        const std::array<vk::AttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};

        const vk::AttachmentReference colorAttachmentRef(0, vk::ImageLayout::eColorAttachmentOptimal);
        const vk::AttachmentReference depthAttachmentRef(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);

        const auto subpass = vk::SubpassDescription(vk::SubpassDescriptionFlags())
                                 .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
                                 .setColorAttachments(colorAttachmentRef)
                                 .setPDepthStencilAttachment(&depthAttachmentRef);

        const auto renderPassCreateInfo = vk::RenderPassCreateInfo().setAttachments(attachments).setSubpasses(subpass);

        renderPass = qtBackend.getDevice()->createRenderPassUnique(
            renderPassCreateInfo, nullptr, qtBackend.getDispatcher());

        // Create framebuffer with both color and depth attachments
        const auto& colorImageView = vulkanTexture.getVulkanImageView();
        const std::array<vk::ImageView, 2> imageViews = {colorImageView.get(), depthAllocation->imageView.get()};

        // Use texture actual size for framebuffer (which might be 1x1 if size was 0x0)
        auto textureSize = vulkanTexture.getSize();
        const auto framebufferCreateInfo = vk::FramebufferCreateInfo()
                                               .setRenderPass(renderPass.get())
                                               .setAttachments(imageViews)
                                               .setWidth(textureSize.width)
                                               .setHeight(textureSize.height)
                                               .setLayers(1);

        framebuffer = qtBackend.getDevice()->createFramebufferUnique(
            framebufferCreateInfo, nullptr, qtBackend.getDispatcher());

        // Update extent to match actual texture size
        extent.width = textureSize.width;
        extent.height = textureSize.height;
    }

    // Override from SurfaceRenderableResource
    void createPlatformSurface() override {
        // No surface needed for offscreen rendering
    }

    VulkanRendererBackend& backend;
    std::unique_ptr<mbgl::gfx::OffscreenTexture> offscreenTexture;
    vk::UniqueFramebuffer framebuffer;
    mbgl::Size size{800, 600};
    bool needsRecreation{false};
};
} // namespace

VulkanRendererBackend::VulkanRendererBackend(QWindow* window)
    : mbgl::vulkan::RendererBackend(mbgl::gfx::ContextMode::Shared),
      mbgl::vulkan::Renderable(mbgl::Size{800, 600},
                               std::make_unique<QtVulkanRenderableResource>(*this, mbgl::Size{800, 600})),
      m_window(window) {
    if (!window) {
        throw std::runtime_error("Window is null");
    }

    QVulkanInstance* qtInstance = window->vulkanInstance();

    if (!qtInstance) {
        // Create our own instance for Qt Quick windows
        qtInstance = createQVulkanInstance();
        m_ownedInstance = qtInstance;
    }

    initializeWithQtInstance(qtInstance);
}

VulkanRendererBackend::VulkanRendererBackend(QVulkanInstance* qtInstance)
    : mbgl::vulkan::RendererBackend(mbgl::gfx::ContextMode::Unique),
      mbgl::vulkan::Renderable(mbgl::Size{800, 600},
                               std::make_unique<QtVulkanRenderableResource>(*this, mbgl::Size{800, 600})) {
    assert(qtInstance);
    initializeWithQtInstance(qtInstance);
}

// Constructor that reuses Qt's Vulkan device for zero-copy texture sharing
VulkanRendererBackend::VulkanRendererBackend(QWindow* window,
                                             VkPhysicalDevice qtPhysicalDevice,
                                             VkDevice qtDevice,
                                             uint32_t qtGraphicsQueueIndex)
    : mbgl::vulkan::RendererBackend(mbgl::gfx::ContextMode::Shared),
      mbgl::vulkan::Renderable(mbgl::Size{800, 600},
                               std::make_unique<QtVulkanRenderableResource>(*this, mbgl::Size{800, 600})),
      m_window(window),
      m_qtPhysicalDevice(qtPhysicalDevice),
      m_qtDevice(qtDevice),
      m_qtGraphicsQueueIndex(qtGraphicsQueueIndex),
      m_useQtDevice(true) {
    if (!window || !qtPhysicalDevice || !qtDevice) {
        throw std::runtime_error("Invalid Qt Vulkan resources");
    }

    QVulkanInstance* qtInstance = window->vulkanInstance();
    if (!qtInstance) {
        throw std::runtime_error("Window does not have a Vulkan instance");
    }

    initializeWithQtInstance(qtInstance);
}

// Convenience constructor that creates its own QVulkanInstance.
// Used when only a ContextMode is available (e.g. MapRenderer's default path).
VulkanRendererBackend::VulkanRendererBackend(mbgl::gfx::ContextMode)
    : VulkanRendererBackend(createQVulkanInstance()) {}

VulkanRendererBackend::~VulkanRendererBackend() {
    // Clean up owned instance if we created one
    if (m_ownedInstance) {
        delete m_ownedInstance;
        m_ownedInstance = nullptr;
    }
}

void VulkanRendererBackend::initializeWithQtInstance(QVulkanInstance* qtInstance) {
    m_qtInstance = qtInstance;

    VkInstance rawInstance = qtInstance->vkInstance();
    if (rawInstance == VK_NULL_HANDLE) {
        throw std::runtime_error("Qt Vulkan instance handle is null");
    }

    init();
}

void VulkanRendererBackend::init() {
    if (!m_qtInstance) {
        mbgl::vulkan::RendererBackend::init();
        return;
    }

    // Initialize dispatcher with Qt's function resolution
    QVulkanFunctions* f = m_qtInstance->functions();
    if (f) {
        auto vkGetInstanceProcAddr = m_qtInstance->getInstanceProcAddr("vkGetInstanceProcAddr");
        if (vkGetInstanceProcAddr) {
            VULKAN_HPP_DEFAULT_DISPATCHER.init(reinterpret_cast<PFN_vkGetInstanceProcAddr>(vkGetInstanceProcAddr));
            dispatcher.init(reinterpret_cast<PFN_vkGetInstanceProcAddr>(vkGetInstanceProcAddr));
        }
    } else {
        dynamicLoader = vk::DynamicLoader();
        dispatcher.init(dynamicLoader);
    }

    // Call base class init which handles most initialization
    mbgl::vulkan::RendererBackend::init();

    // Only do Qt-specific initialization
    initSwapchain();
    mbgl::vulkan::RendererBackend::initCommandPool();
}

void VulkanRendererBackend::initInstance() {
    if (!m_qtInstance) {
        mbgl::vulkan::RendererBackend::initInstance();
        return;
    }

    // Reuse Qt's existing instance
    usingSharedContext = true;

    VkInstance rawInstance = m_qtInstance->vkInstance();
    if (!rawInstance) {
        throw std::runtime_error("Qt VkInstance is null");
    }

    vk::Instance vkInstance(rawInstance);
    instance = vk::UniqueInstance(vkInstance,
                                  vk::ObjectDestroy<vk::NoParent, vk::DispatchLoaderDynamic>(nullptr, dispatcher));

    // Check if debug utils extension is available
    const auto& extensions = m_qtInstance->supportedExtensions();
    debugUtilsEnabled = std::any_of(extensions.begin(), extensions.end(), [](const auto& ext) {
        return ext.name == VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    });
}

void VulkanRendererBackend::initSurface() {
    // For offscreen rendering, we don't need a surface
}

std::vector<const char*> VulkanRendererBackend::getDeviceExtensions() {
    std::vector<const char*> extensions;

#ifdef __APPLE__
    extensions.push_back("VK_KHR_portability_subset");
#endif

    return extensions;
}

void VulkanRendererBackend::initDevice() {
    if (m_useQtDevice && m_qtDevice && m_qtPhysicalDevice) {
        // Reuse Qt's existing Vulkan device for zero-copy texture sharing
        physicalDevice = vk::PhysicalDevice(m_qtPhysicalDevice);

        vk::Device vkDevice(m_qtDevice);
        device = vk::UniqueDevice(vkDevice,
                                  vk::ObjectDestroy<vk::NoParent, vk::DispatchLoaderDynamic>(nullptr, dispatcher));

        graphicsQueueIndex = m_qtGraphicsQueueIndex;
        presentQueueIndex = m_qtGraphicsQueueIndex;

        dispatcher.init(vkDevice);

        physicalDeviceProperties = physicalDevice.getProperties(dispatcher);
        physicalDeviceFeatures = physicalDevice.getFeatures(dispatcher);

        graphicsQueue = device->getQueue(graphicsQueueIndex, 0, dispatcher);
        presentQueue = graphicsQueue;

        return;
    }

    // Use base class implementation for device creation
    mbgl::vulkan::RendererBackend::initDevice();
}

void VulkanRendererBackend::initSwapchain() {
    // For offscreen rendering, we don't need a swapchain
    maxFrames = 1;
}

void VulkanRendererBackend::setSize(mbgl::Size size_) {
    // Propagate size to the Renderable base class
    mbgl::vulkan::Renderable::setSize(size_);

    // Also update our custom resource
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

void VulkanRendererBackend::updateFramebuffer(quint32 /*fbo*/, const mbgl::Size& newSize) {
    setSize(newSize);
}

// Helper function to create a QVulkanInstance
QVulkanInstance* VulkanRendererBackend::createQVulkanInstance() {
    auto* qtInstance = new QVulkanInstance();
    qtInstance->setApiVersion(QVersionNumber(1, 0));

#ifndef NDEBUG
    qtInstance->setLayers({"VK_LAYER_KHRONOS_validation"});
#endif

    if (!qtInstance->create()) {
        delete qtInstance;
        throw std::runtime_error("Failed to create QVulkanInstance");
    }

    return qtInstance;
}

} // namespace QMapLibre
