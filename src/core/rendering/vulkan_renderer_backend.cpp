// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#include "vulkan_renderer_backend_p.hpp"

// Define storage for the global Vulkan dispatcher
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

#include <mbgl/gfx/backend_scope.hpp>
#include <mbgl/gfx/context.hpp>
#include <mbgl/gfx/offscreen_texture.hpp>
#include <mbgl/vulkan/context.hpp>
#include <mbgl/vulkan/renderable_resource.hpp>
#include <mbgl/vulkan/texture2d.hpp>

#include <QtGui/QVulkanFunctions>
#include <QtGui/QVulkanInstance>
#include <QtGui/QVulkanWindow>

#include <vulkan/vulkan.hpp>

#include <algorithm>
#include <cassert>

namespace {
constexpr uint32_t DefaultSize{256};

// Custom renderable resource for Qt integration supporting zero-copy GPU rendering
class QtVulkanRenderableResource final : public mbgl::vulkan::SurfaceRenderableResource {
public:
    QtVulkanRenderableResource(QMapLibre::VulkanRendererBackend &backend_, mbgl::Size initialSize)
        : mbgl::vulkan::SurfaceRenderableResource(backend_),
          backend(backend_),
          size(initialSize) {
        extent.width = size.width;
        extent.height = size.height;
    }

    void setExternalImage(VkImage image, mbgl::Size imageSize) {
        // Only recreate if the image or size has changed
        bool imageChanged = (externalImage != image);
        bool sizeChanged = (size.width != imageSize.width || size.height != imageSize.height);

        if (imageChanged || sizeChanged) {
            externalImage = image;
            size = imageSize;
            extent.width = imageSize.width;
            extent.height = imageSize.height;

            // Force recreation of render pass and framebuffer with new external image
            framebuffer.reset();
            renderPass.reset();
            needsRecreation = true;
        }
    }

    void setBackendSize(mbgl::Size size_) {
        // If we have an external image, don't override its size
        if (externalImage != VK_NULL_HANDLE) {
            return;
        }

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
                        backend.setCurrentDrawable(texture.get());
                    }
                }

                // Reset render pass and framebuffer to force recreation
                framebuffer.reset();
                renderPass.reset();
            }
        }
    }

    [[nodiscard]] mbgl::Size getSize() const { return size; }
    [[nodiscard]] mbgl::Size getBackendSize() const {
        return static_cast<const mbgl::gfx::Renderable &>(backend).getSize();
    }

    // Override bind to ensure we have an offscreen texture or external image
    void bind() override {
        // If we have an external image from QRhiWidget, use that instead of creating our own
        if (externalImage != VK_NULL_HANDLE) {
            if (!renderPass || !framebuffer || needsRecreation) {
                createRenderPassForExternalImage();
                needsRecreation = false;
            }

            // IMPORTANT: MapLibre will call getFramebuffer() and getRenderPass() after this bind()
            // Our framebuffer and renderPass members are now set up correctly for the external image
            // No need to call base class bind() as it's designed for swapchain surfaces

            // The framebuffer and renderPass created in createRenderPassForExternalImage()
            // will be used by MapLibre's RenderPass when it calls resource.getFramebuffer()
            // and resource.getRenderPass()

            return;
        }

        // Fallback to creating our own offscreen texture
        // Use a minimal size if the actual size is invalid
        mbgl::Size textureSize = size;
        if (size.isEmpty()) {
            textureSize = mbgl::Size{DefaultSize, DefaultSize};
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
                    backend.setCurrentDrawable(texture.get());
                }
            }
        }

        // Create render pass and framebuffer if needed
        if (offscreenTexture && !renderPass) {
            createRenderPass();
        }
    }

    [[nodiscard]] const vk::UniqueFramebuffer &getFramebuffer() const override { return framebuffer; }

    // Note: getRenderPass() is inherited from RenderableResource and returns the renderPass member
    // which should be set by createRenderPassForExternalImage() or createRenderPass()
    // We can't override it because the base class method isn't virtual

    void swap() override {
        // For offscreen rendering, we need to ensure commands are submitted
        // but we don't present to a swapchain

        if (externalImage != VK_NULL_HANDLE) {
            // Get rendering context
            auto &context = static_cast<mbgl::vulkan::Context &>(backend.getContext());

            // Submit the frame - this should contain all the MapLibre rendering commands
            context.submitFrame();

            // Wait for frame fence to ensure command buffer has been processed
            context.waitFrame();

            return;
        }

        // Only call base class swap if we're not using an external image
        mbgl::vulkan::SurfaceRenderableResource::swap();

        // Add barrier to transition image layout to shader read-only optimal
        if (offscreenTexture) {
            auto texture = offscreenTexture->getTexture();
            if (texture) {
                // The barrier will be added by the texture itself when needed
                (void)static_cast<mbgl::vulkan::Texture2D &>(*texture);
            }
        }

        // No need for waitIdle as synchronization is handled by barriers
    }

private:
    void createRenderPass() {
        if (offscreenTexture == nullptr) {
            return;
        }

        auto texture = offscreenTexture->getTexture();
        if (texture == nullptr) {
            return;
        }

        auto &vulkanTexture = static_cast<mbgl::vulkan::Texture2D &>(*texture);

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

        renderPass = backend.getDevice()->createRenderPassUnique(
            renderPassCreateInfo, nullptr, backend.getDispatcher());

        // Create framebuffer with both color and depth attachments
        const auto &colorImageView = vulkanTexture.getVulkanImageView();
        const std::array<vk::ImageView, 2> imageViews = {colorImageView.get(), depthAllocation->imageView.get()};

        // Use texture actual size for framebuffer (which might be 1x1 if size was 0x0)
        auto textureSize = vulkanTexture.getSize();
        const auto framebufferCreateInfo = vk::FramebufferCreateInfo()
                                               .setRenderPass(renderPass.get())
                                               .setAttachments(imageViews)
                                               .setWidth(textureSize.width)
                                               .setHeight(textureSize.height)
                                               .setLayers(1);

        framebuffer = backend.getDevice()->createFramebufferUnique(
            framebufferCreateInfo, nullptr, backend.getDispatcher());

        // Update extent to match actual texture size
        extent.width = textureSize.width;
        extent.height = textureSize.height;
    }

    // Override from SurfaceRenderableResource
    void createPlatformSurface() override {
        // No surface needed for offscreen rendering
    }

    void createRenderPassForExternalImage() {
        if (externalImage == VK_NULL_HANDLE) {
            return;
        }

        // Initialize depth/stencil resources if needed
        initDepthStencil();

        // Create image view for external image
        // Qt RHI format 1 corresponds to RGBA8 (R8G8B8A8Unorm)
        const auto imageViewCreateInfo = vk::ImageViewCreateInfo()
                                             .setImage(externalImage)
                                             .setViewType(vk::ImageViewType::e2D)
                                             .setFormat(vk::Format::eR8G8B8A8Unorm) // RGBA format that Qt RHI uses
                                             .setComponents(vk::ComponentMapping()) // Identity swizzle
                                             .setSubresourceRange(vk::ImageSubresourceRange()
                                                                      .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                                                      .setBaseMipLevel(0)
                                                                      .setLevelCount(1)
                                                                      .setBaseArrayLayer(0)
                                                                      .setLayerCount(1));

        externalImageView = backend.getDevice()->createImageViewUnique(
            imageViewCreateInfo, nullptr, backend.getDispatcher());

        // Qt RHI provides the image already in a render pass with COLOR_ATTACHMENT_OPTIMAL layout
        // We need to work within Qt's existing render pass constraints
        const auto colorAttachment = vk::AttachmentDescription(vk::AttachmentDescriptionFlags())
                                         .setFormat(vk::Format::eR8G8B8A8Unorm) // Match Qt RHI's RGBA format
                                         .setSamples(vk::SampleCountFlagBits::e1)
                                         .setLoadOp(vk::AttachmentLoadOp::eClear)   // Clear to see if rendering works
                                         .setStoreOp(vk::AttachmentStoreOp::eStore) // Store back for Qt to use
                                         .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
                                         .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
                                         .setInitialLayout(vk::ImageLayout::eUndefined) // Don't assume initial layout
                                         .setFinalLayout(
                                             vk::ImageLayout::ePresentSrcKHR); // Qt needs this for presentation

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

        renderPass = backend.getDevice()->createRenderPassUnique(
            renderPassCreateInfo, nullptr, backend.getDispatcher());

        // Create framebuffer with external image view
        const std::array<vk::ImageView, 2> imageViews = {externalImageView.get(), depthAllocation->imageView.get()};

        // Use extent (which is set from imageSize) instead of size to ensure consistency
        const auto framebufferCreateInfo = vk::FramebufferCreateInfo()
                                               .setRenderPass(renderPass.get())
                                               .setAttachments(imageViews)
                                               .setWidth(extent.width)
                                               .setHeight(extent.height)
                                               .setLayers(1);

        framebuffer = backend.getDevice()->createFramebufferUnique(
            framebufferCreateInfo, nullptr, backend.getDispatcher());
    }

    QMapLibre::VulkanRendererBackend &backend;
    std::unique_ptr<mbgl::gfx::OffscreenTexture> offscreenTexture;
    vk::UniqueFramebuffer framebuffer;
    mbgl::Size size{DefaultSize, DefaultSize};
    bool needsRecreation{false};
    VkImage externalImage{VK_NULL_HANDLE};
    vk::UniqueImageView externalImageView;
};
} // namespace

namespace QMapLibre {

VulkanRendererBackend::VulkanRendererBackend(QWindow *window)
    : mbgl::vulkan::RendererBackend(mbgl::gfx::ContextMode::Shared),
      mbgl::vulkan::Renderable(
          mbgl::Size{DefaultSize, DefaultSize},
          std::make_unique<QtVulkanRenderableResource>(*this, mbgl::Size{DefaultSize, DefaultSize})),
      m_window(window) {
    QVulkanInstance *qtInstance = nullptr;

    if (window != nullptr) {
        qtInstance = window->vulkanInstance();
    }

    if (qtInstance == nullptr) {
        // Create our own instance for windows without an instance or for widgets
        m_ownedInstance = createQVulkanInstance();
        qtInstance = m_ownedInstance.get();
    }

    initializeWithQtInstance(qtInstance);
}

VulkanRendererBackend::VulkanRendererBackend(QVulkanInstance *qtInstance)
    : mbgl::vulkan::RendererBackend(mbgl::gfx::ContextMode::Unique),
      mbgl::vulkan::Renderable(
          mbgl::Size{DefaultSize, DefaultSize},
          std::make_unique<QtVulkanRenderableResource>(*this, mbgl::Size{DefaultSize, DefaultSize})) {
    assert(qtInstance);
    initializeWithQtInstance(qtInstance);
}

// Constructor that reuses Qt's Vulkan device for zero-copy texture sharing
VulkanRendererBackend::VulkanRendererBackend(QWindow *window,
                                             VkPhysicalDevice qtPhysicalDevice,
                                             VkDevice qtDevice,
                                             uint32_t qtGraphicsQueueIndex)
    : mbgl::vulkan::RendererBackend(mbgl::gfx::ContextMode::Shared),
      mbgl::vulkan::Renderable(
          mbgl::Size{DefaultSize, DefaultSize},
          std::make_unique<QtVulkanRenderableResource>(*this, mbgl::Size{DefaultSize, DefaultSize})),
      m_window(window),
      m_qtPhysicalDevice(qtPhysicalDevice),
      m_qtDevice(qtDevice),
      m_qtGraphicsQueueIndex(qtGraphicsQueueIndex),
      m_useQtDevice(true) {
    if (window == nullptr || qtPhysicalDevice == nullptr || qtDevice == nullptr) {
        throw std::runtime_error("Invalid Qt Vulkan resources");
    }

    QVulkanInstance *qtInstance = window->vulkanInstance();
    if (qtInstance == nullptr) {
        throw std::runtime_error("Window does not have a Vulkan instance");
    }

    initializeWithQtInstance(qtInstance);
}

VulkanRendererBackend::~VulkanRendererBackend() = default;

void VulkanRendererBackend::initializeWithQtInstance(QVulkanInstance *qtInstance) {
    m_qtInstance = qtInstance;

    VkInstance rawInstance = qtInstance->vkInstance();
    if (rawInstance == VK_NULL_HANDLE) {
        throw std::runtime_error("Qt Vulkan instance handle is null");
    }

    init();
}

void VulkanRendererBackend::init() {
    if (m_qtInstance == nullptr) {
        mbgl::vulkan::RendererBackend::init();
        return;
    }

    // Initialize dispatcher with Qt's function resolution
    QVulkanFunctions *f = m_qtInstance->functions();
    if (f != nullptr) {
        auto vkGetInstanceProcAddr = m_qtInstance->getInstanceProcAddr("vkGetInstanceProcAddr");
        if (vkGetInstanceProcAddr != nullptr) {
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
    if (m_qtInstance == nullptr) {
        mbgl::vulkan::RendererBackend::initInstance();
        return;
    }

    // Reuse Qt's existing instance
    usingSharedContext = true;

    VkInstance rawInstance = m_qtInstance->vkInstance();
    if (rawInstance == nullptr) {
        throw std::runtime_error("Qt VkInstance is null");
    }

    vk::Instance vkInstance(rawInstance);
    instance = vk::UniqueInstance(vkInstance,
                                  vk::ObjectDestroy<vk::NoParent, vk::DispatchLoaderDynamic>(nullptr, dispatcher));

    // Check if debug utils extension is available
    const auto &extensions = m_qtInstance->supportedExtensions();
    debugUtilsEnabled = std::ranges::any_of(
        extensions, [](const auto &ext) { return ext.name == VK_EXT_DEBUG_UTILS_EXTENSION_NAME; });
}

void VulkanRendererBackend::initSurface() {
    // For offscreen rendering, we don't need a surface
}

std::vector<const char *> VulkanRendererBackend::getDeviceExtensions() {
    std::vector<const char *> extensions;

#ifdef __APPLE__
    extensions.push_back("VK_KHR_portability_subset");
#endif

    return extensions;
}

void VulkanRendererBackend::initDevice() {
    if (m_useQtDevice && m_qtDevice != nullptr && m_qtPhysicalDevice != nullptr) {
        // Reuse Qt's existing Vulkan device for zero-copy texture sharing
        physicalDevice = vk::PhysicalDevice(m_qtPhysicalDevice);

        vk::Device vkDevice(m_qtDevice);
        device = vk::UniqueDevice(vkDevice,
                                  vk::ObjectDestroy<vk::NoParent, vk::DispatchLoaderDynamic>(nullptr, dispatcher));

        graphicsQueueIndex = static_cast<int32_t>(m_qtGraphicsQueueIndex);
        presentQueueIndex = static_cast<int32_t>(m_qtGraphicsQueueIndex);

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

mbgl::vulkan::Texture2D *VulkanRendererBackend::getOffscreenTexture() const {
    // Try to return the current drawable if it's a Texture2D
    if (m_currentDrawable != nullptr) {
        return static_cast<mbgl::vulkan::Texture2D *>(m_currentDrawable);
    }
    return nullptr;
}

void VulkanRendererBackend::setExternalImage(VkImage image, const mbgl::Size &size) {
    // Pass the external image to our custom renderable resource
    auto &resource = this->getResource<QtVulkanRenderableResource>();

    resource.setExternalImage(image, size);
}

// Helper function to create a QVulkanInstance
std::unique_ptr<QVulkanInstance> VulkanRendererBackend::createQVulkanInstance() {
    auto qtInstance = std::make_unique<QVulkanInstance>();
    qtInstance->setApiVersion(QVersionNumber(1, 0));

#ifndef NDEBUG
    qtInstance->setLayers({"VK_LAYER_KHRONOS_validation"});
#endif

    if (!qtInstance->create()) {
        qtInstance.reset();
        throw std::runtime_error("Failed to create QVulkanInstance");
    }

    return qtInstance;
}

} // namespace QMapLibre
