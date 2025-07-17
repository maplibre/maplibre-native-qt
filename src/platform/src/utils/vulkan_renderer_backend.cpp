#if defined(MLN_RENDER_BACKEND_VULKAN)

#include "vulkan_renderer_backend.hpp"

#include <mbgl/gfx/backend_scope.hpp>
#include <mbgl/vulkan/context.hpp>
#include <mbgl/vulkan/renderable_resource.hpp>
#include <mbgl/vulkan/offscreen_texture.hpp>
#include <mbgl/vulkan/texture2d.hpp>

#include <QGuiApplication>
#include <QtGui/QVulkanInstance>
#include <QtGui/QWindow>
#include <QDebug>

#include <vulkan/vulkan.h>
#include <cassert>

namespace QMapLibre {

// ------------- QtVulkanRenderableResource -----------------------------------
QtVulkanRenderableResource::QtVulkanRenderableResource(VulkanRendererBackend& backend_)
    : SurfaceRenderableResource(backend_) {
    qDebug() << "QtVulkanRenderableResource constructor called";
}

std::vector<const char*> QtVulkanRenderableResource::getDeviceExtensions() {
    return {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
}

void QtVulkanRenderableResource::createPlatformSurface() {
    // For Qt Quick offscreen rendering, we don't create a platform surface
    // Instead, we render to offscreen textures that Qt Quick can display
    qDebug() << "QtVulkanRenderableResource::createPlatformSurface() - skipping surface creation for Qt Quick offscreen rendering";
    
    // Don't create a surface - this will make platformSurface null
    // which will cause Context::beginFrame() to skip swapchain operations
    qDebug() << "Surface creation skipped - Context will use offscreen rendering path";
    
    // The surface member will remain null, causing the Context to skip
    // swapchain operations in beginFrame()
}

void QtVulkanRenderableResource::createDummySwapchain() {
    // No longer needed - we rely on null surface to skip swapchain operations
    qDebug() << "QtVulkanRenderableResource::createDummySwapchain() - not needed for null surface approach";
}

void QtVulkanRenderableResource::bind() {
    qDebug() << "QtVulkanRenderableResource::bind() called";
    
    // Check if surface is null (which is what we want for offscreen rendering)
    if (surface) {
        qDebug() << "Warning: Surface is not null in bind() - this may cause issues";
        qDebug() << "Surface handle:" << surface.get();
    } else {
        qDebug() << "Surface is null as expected for offscreen rendering";
    }
    
    // For Qt Vulkan offscreen rendering, we need to create a proper render pass and framebuffer
    // similar to how OffscreenTextureResource does it
    auto& qtBackend = static_cast<VulkanRendererBackend&>(backend);
    
    // Ensure we have an offscreen texture
    if (!qtBackend.m_offscreenTexture) {
        qDebug() << "Creating offscreen texture in bind()";
        qtBackend.createOffscreenTexture(qtBackend.size);
    }
    
    if (qtBackend.m_offscreenTexture) {
        qDebug() << "Setting up render pass for offscreen texture";
        
        // Get the texture and create it if needed
        auto texture = qtBackend.m_offscreenTexture->getTexture();
        if (texture) {
            texture->create();
        }
        
        // Set up the extent for the render pass
        extent.width = qtBackend.size.width;
        extent.height = qtBackend.size.height;
        
        // Create render pass if needed
        createOffscreenRenderPass();
        
        qDebug() << "Offscreen render pass setup completed";
    } else {
        qDebug() << "Warning: No offscreen texture available in bind()";
    }
    
    qDebug() << "QtVulkanRenderableResource::bind() completed";
}

void QtVulkanRenderableResource::createOffscreenRenderPass() {
    if (renderPass) {
        qDebug() << "Render pass already exists, skipping creation";
        return;
    }
    
    qDebug() << "Creating offscreen render pass";
    
    auto& qtBackend = static_cast<VulkanRendererBackend&>(backend);
    
    if (!qtBackend.m_offscreenTexture) {
        qDebug() << "No offscreen texture available for render pass creation";
        return;
    }
    
    auto texture = qtBackend.m_offscreenTexture->getTexture();
    if (!texture) {
        qDebug() << "Failed to get texture from offscreen texture";
        return;
    }
    
    auto& vulkanTexture = static_cast<mbgl::vulkan::Texture2D&>(*texture);
    
    // Create a render pass similar to OffscreenTextureResource
    const auto colorAttachment = vk::AttachmentDescription(vk::AttachmentDescriptionFlags())
                                     .setFormat(vulkanTexture.getVulkanFormat())
                                     .setSamples(vk::SampleCountFlagBits::e1)
                                     .setLoadOp(vk::AttachmentLoadOp::eClear)
                                     .setStoreOp(vk::AttachmentStoreOp::eStore)
                                     .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
                                     .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
                                     .setInitialLayout(vk::ImageLayout::eUndefined)
                                     .setFinalLayout(vk::ImageLayout::eShaderReadOnlyOptimal);

    const auto colorAttachmentReference = vk::AttachmentReference()
                                              .setAttachment(0)
                                              .setLayout(vk::ImageLayout::eColorAttachmentOptimal);

    const auto subpass = vk::SubpassDescription(vk::SubpassDescriptionFlags())
                             .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
                             .setColorAttachmentCount(1)
                             .setPColorAttachments(&colorAttachmentReference);

    const auto renderPassCreateInfo = vk::RenderPassCreateInfo()
                                          .setAttachmentCount(1)
                                          .setPAttachments(&colorAttachment)
                                          .setSubpassCount(1)
                                          .setPSubpasses(&subpass);

    try {
        auto& qtBackend = static_cast<VulkanRendererBackend&>(backend);
        const auto& device = qtBackend.getDevice();
        renderPass = device->createRenderPassUnique(renderPassCreateInfo);
        
        // Create framebuffer
        createOffscreenFramebuffer();
        
        qDebug() << "Offscreen render pass created successfully";
    } catch (const std::exception& e) {
        qDebug() << "Failed to create offscreen render pass:" << e.what();
        throw;
    }
}

void QtVulkanRenderableResource::createOffscreenFramebuffer() {
    if (framebuffer) {
        qDebug() << "Framebuffer already exists, skipping creation";
        return;
    }
    
    qDebug() << "Creating offscreen framebuffer";
    
    auto& qtBackend = static_cast<VulkanRendererBackend&>(backend);
    
    if (!qtBackend.m_offscreenTexture) {
        qDebug() << "No offscreen texture available for framebuffer creation";
        return;
    }
    
    auto texture = qtBackend.m_offscreenTexture->getTexture();
    if (!texture) {
        qDebug() << "Failed to get texture from offscreen texture";
        return;
    }
    
    auto& vulkanTexture = static_cast<mbgl::vulkan::Texture2D&>(*texture);
    
    try {
        auto& qtBackend = static_cast<VulkanRendererBackend&>(backend);
        const auto& device = qtBackend.getDevice();
        
        const auto& imageView = vulkanTexture.getVulkanImageView();
        
        const auto framebufferCreateInfo = vk::FramebufferCreateInfo()
                                               .setRenderPass(renderPass.get())
                                               .setAttachmentCount(1)
                                               .setPAttachments(&imageView.get())
                                               .setWidth(extent.width)
                                               .setHeight(extent.height)
                                               .setLayers(1);
        
        framebuffer = device->createFramebufferUnique(framebufferCreateInfo);
        
        qDebug() << "Offscreen framebuffer created successfully";
    } catch (const std::exception& e) {
        qDebug() << "Failed to create offscreen framebuffer:" << e.what();
        throw;
    }
}

void QtVulkanRenderableResource::setupQtTextureRendering(void* qtTexture) {
    if (!qtTexture) {
        qDebug() << "No Qt texture provided for rendering";
        return;
    }
    
    auto& qtBackend = static_cast<VulkanRendererBackend&>(backend);
    
    // Store the Qt texture for later use
    qtBackend._q_setCurrentDrawable(qtTexture);
    
    qDebug() << "Qt texture rendering setup completed";
    
    // The texture will be used in the currentDrawable() method
    // This allows MapLibre to render directly to the Qt-provided texture
}

// ------------- VulkanRendererBackend ---------------------------------------
VulkanRendererBackend::VulkanRendererBackend(QWindow* window_)
    : mbgl::vulkan::RendererBackend(mbgl::gfx::ContextMode::Unique),
      mbgl::vulkan::Renderable({static_cast<uint32_t>(std::max(1, window_->width()) * window_->devicePixelRatio()),
                                static_cast<uint32_t>(std::max(1, window_->height()) * window_->devicePixelRatio())},
                               std::make_unique<QtVulkanRenderableResource>(*this)),
      window(window_) {
    
    qDebug() << "Creating VulkanRendererBackend with window:" << window_;
    qDebug() << "Window size:" << window_->width() << "x" << window_->height();
    qDebug() << "Window device pixel ratio:" << window_->devicePixelRatio();
    qDebug() << "Window surface type:" << window_->surfaceType();
    
    // Ensure window has minimum size
    if (window_->width() <= 0 || window_->height() <= 0) {
        qWarning() << "Window has invalid size, using fallback dimensions";
        window_->resize(std::max(1, window_->width()), std::max(1, window_->height()));
    }
    
    // Use the existing Vulkan instance from the window (preferred)
    QVulkanInstance* existingInstance = window_->vulkanInstance();
    if (existingInstance) {
        qDebug() << "Using existing Vulkan instance from window";
        vulkanInstance = existingInstance;
    } else {
        qDebug() << "Creating new Vulkan instance for window";
        
        // Create a new Vulkan instance compatible with Qt Quick
        vulkanInstance = new QVulkanInstance();
        vulkanInstance->setApiVersion(QVersionNumber(1, 2));
        
        // Set up basic Vulkan validation layers for development
        QByteArrayList layers;
#ifdef QT_DEBUG
        layers << "VK_LAYER_KHRONOS_validation";
#endif
        vulkanInstance->setLayers(layers);
        
        // Set required extensions
        QByteArrayList extensions;
        extensions << VK_KHR_SURFACE_EXTENSION_NAME;
        
        // Add platform-specific surface extensions
        #ifdef Q_OS_WIN
        extensions << "VK_KHR_win32_surface";
        #elif defined(Q_OS_UNIX) && !defined(Q_OS_ANDROID)
        extensions << "VK_KHR_xcb_surface";
        extensions << "VK_KHR_wayland_surface";
        #endif
        
        vulkanInstance->setExtensions(extensions);
        
        if (!vulkanInstance->create()) {
            throw std::runtime_error("Failed to create QVulkanInstance");
        }
        
        // Set the instance on the window
        window_->setVulkanInstance(vulkanInstance);
        
        // Make sure the surface type is set to Vulkan
        if (window_->surfaceType() != QSurface::VulkanSurface) {
            window_->setSurfaceType(QSurface::VulkanSurface);
        }
    }

    // Verify the setup
    if (window_->surfaceType() != QSurface::VulkanSurface) {
        qWarning() << "Window surface type is not Vulkan:" << window_->surfaceType();
    }

    // Make sure the window is created before initializing the renderer
    if (!window_->handle()) {
        qDebug() << "Creating window handle for Vulkan backend";
        window_->create();
    }
    
    // The init() call is delayed to ensureInitialized() to avoid issues during construction
    // when the surface might not be ready yet
    m_isInitialized = false;
    
    qDebug() << "VulkanRendererBackend constructor completed successfully";
}

// Fallback constructor used by MapRenderer when only a ContextMode is provided.
VulkanRendererBackend::VulkanRendererBackend(mbgl::gfx::ContextMode mode)
    : mbgl::vulkan::RendererBackend(mode),
      mbgl::vulkan::Renderable({1, 1}, std::make_unique<QtVulkanRenderableResource>(*this)),
      window(nullptr) {
    // Create a minimal Vulkan instance for headless rendering
    vulkanInstance = new QVulkanInstance();
    vulkanInstance->setApiVersion(QVersionNumber(1, 0));
    
    // No validation layers for headless mode to keep it minimal
    if (!vulkanInstance->create()) {
        throw std::runtime_error("Failed to create QVulkanInstance for headless rendering");
    }

    // Delay initialization for headless mode as well
    m_isInitialized = false;
    
    qDebug() << "VulkanRendererBackend headless constructor completed successfully";
}

void VulkanRendererBackend::ensureInitialized() {
    if (m_isInitialized) {
        return;
    }
    
    qDebug() << "Initializing Vulkan renderer backend";
    
    // Make sure the window is properly set up
    if (!window) {
        qWarning() << "Cannot initialize Vulkan backend: no window";
        return;
    }
    
    // Ensure the window is visible and fully created
    if (!window->isVisible()) {
        qDebug() << "Window is not visible, making it visible for Vulkan initialization";
        window->show();
        QCoreApplication::processEvents();
    }
    
    // Make sure the window has a handle
    if (!window->handle()) {
        qDebug() << "Creating window handle before Vulkan initialization";
        window->create();
        QCoreApplication::processEvents(); // Process any pending events
    }
    
    // Verify the window is ready
    if (!window->handle()) {
        qWarning() << "Cannot initialize Vulkan backend: window handle creation failed";
        return;
    }
    
    // Make sure the surface type is correct
    if (window->surfaceType() != QSurface::VulkanSurface) {
        qWarning() << "Window surface type is not Vulkan during initialization:" << window->surfaceType();
        qDebug() << "Attempting to set surface type to Vulkan";
        window->setSurfaceType(QSurface::VulkanSurface);
        
        // Recreate the window with the correct surface type
        if (window->handle()) {
            window->destroy();
            window->create();
            QCoreApplication::processEvents();
        }
    }
    
    // Ensure window has valid dimensions
    if (window->width() <= 0 || window->height() <= 0) {
        qWarning() << "Window has invalid dimensions during initialization:" << window->width() << "x" << window->height();
        window->resize(std::max(1, window->width()), std::max(1, window->height()));
        QCoreApplication::processEvents();
    }
    
    // Initialize the base class
    try {
        qDebug() << "Calling init() to initialize base Vulkan backend";
        
        // First, setup Qt Vulkan instance integration
        setupQtVulkanInstance();
        
        // Now call the base class init
        init();
        m_isInitialized = true;
        qDebug() << "Vulkan renderer backend initialized successfully";
    } catch (const std::exception& e) {
        qWarning() << "Failed to initialize Vulkan renderer backend:" << e.what();
        // Don't set initialized to true if initialization failed
        m_isInitialized = false;
        throw; // Re-throw the exception to prevent further use
    }
}

void VulkanRendererBackend::setupQtVulkanInstance() {
    qDebug() << "Setting up Qt Vulkan instance integration";

    // Use Qt's Vulkan instance instead of creating a new one
    if (!vulkanInstance) {
        throw std::runtime_error("Qt Vulkan instance not available");
    }

    // Get the raw Vulkan instance handle from Qt
    VkInstance rawInstance = vulkanInstance->vkInstance();
    if (rawInstance == VK_NULL_HANDLE) {
        throw std::runtime_error("Qt Vulkan instance handle is null");
    }

    qDebug() << "Using Qt Vulkan instance handle:" << (void*)rawInstance;

    // Initialize the dispatch loader using the dynamic loader first
    VULKAN_HPP_DEFAULT_DISPATCHER.init(dynamicLoader);
    
    // Then initialize with the instance - wrap in vk::Instance
    vk::Instance vkInstance(rawInstance);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkInstance);
    
    // Replace the base class instance with Qt's instance
    instance = vk::UniqueInstance(vkInstance, vk::ObjectDestroy<vk::NoParent, vk::DispatchLoaderDynamic>());
    
    qDebug() << "Qt Vulkan instance integration setup completed";
}

void VulkanRendererBackend::init() {
    qDebug() << "Initializing Vulkan renderer backend with Qt integration";
    
    // First, setup Qt instance integration
    setupQtVulkanInstance();
    
    // Now manually call the init steps, but skip initInstance since we're using Qt's
    qDebug() << "Calling initFrameCapture()";
    initFrameCapture();
    
    qDebug() << "Calling initSurface()";
    initSurface();
    
    qDebug() << "Calling initDevice()";
    initDevice();
    
    qDebug() << "Calling initAllocator()";
    initAllocator();
    
    qDebug() << "Calling initSwapchain()";
    initSwapchain();
    
    qDebug() << "Calling initCommandPool()";
    initCommandPool();
    
    qDebug() << "Vulkan renderer backend initialization completed";
}

void VulkanRendererBackend::initSwapchain() {
    qDebug() << "VulkanRendererBackend::initSwapchain() called - skipping swapchain creation for Qt Quick integration";
    
    // For Qt Quick integration, we don't create a swapchain on the window surface
    // because Qt Quick already manages the window surface and its swapchain.
    // Instead, we'll use offscreen rendering to a texture that Qt Quick can display.
    
    // Create an offscreen texture that Qt Quick can display
    if (!size.isEmpty()) {
        qDebug() << "Creating offscreen render target for Qt Quick integration";
        createOffscreenTexture(size);
    } else {
        qDebug() << "Size is empty, will create offscreen texture when size is set";
    }
    
    qDebug() << "Swapchain initialization skipped for Qt Quick compatibility";
}

std::unique_ptr<mbgl::gfx::Context> VulkanRendererBackend::createContext() {
    qDebug() << "VulkanRendererBackend::createContext() called - ensuring backend is initialized";
    
    // Ensure the backend is fully initialized before creating the context
    ensureInitialized();
    
    if (!m_isInitialized) {
        throw std::runtime_error("Failed to initialize Vulkan backend before creating context");
    }
    
    // Create the standard Vulkan context
    return mbgl::vulkan::RendererBackend::createContext();
}

VulkanRendererBackend::~VulkanRendererBackend() = default;

void VulkanRendererBackend::setSize(const mbgl::Size newSize) {
    ensureInitialized();
    
    size = newSize;
    
    // Recreate offscreen texture if size changed
    if (m_offscreenTexture && !newSize.isEmpty()) {
        qDebug() << "Recreating offscreen texture due to size change:" << newSize.width << "x" << newSize.height;
        m_offscreenTexture.reset();
        createOffscreenTexture(newSize);
    }
    
    if (context) {
        static_cast<mbgl::vulkan::Context&>(*context).requestSurfaceUpdate();
    }
}

void VulkanRendererBackend::updateFramebuffer(uint32_t fbo, const mbgl::Size& newSize) {
    // For Vulkan, we don't use OpenGL framebuffers, but we need to update the size
    setSize(newSize);
    // fbo parameter is ignored for Vulkan backend
    Q_UNUSED(fbo);
}

std::vector<const char*> VulkanRendererBackend::getInstanceExtensions() {
    auto extensions = mbgl::vulkan::RendererBackend::getInstanceExtensions();

    // QVulkanInstance already enables VK_KHR_surface and platform-specific
    // extensions internally, but make sure to request them for completeness.
    extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

#ifdef __APPLE__
    extensions.push_back("VK_EXT_metal_surface");
#elif defined(Q_OS_WIN)
    extensions.push_back("VK_KHR_win32_surface");
#elif defined(Q_OS_UNIX)
    extensions.push_back("VK_KHR_xcb_surface"); // best effort
#endif
    return extensions;
}

void* VulkanRendererBackend::currentDrawable() const {
    qDebug() << "VulkanRendererBackend::currentDrawable() called";
    
    // Ensure the backend is initialized before use
    try {
        const_cast<VulkanRendererBackend*>(this)->ensureInitialized();
        
        // Check if initialization was successful
        if (!m_isInitialized) {
            qWarning() << "VulkanRendererBackend not initialized, returning nullptr";
            return nullptr;
        }
        qDebug() << "VulkanRendererBackend is initialized";
    } catch (const std::exception& e) {
        qWarning() << "Failed to initialize VulkanRendererBackend in currentDrawable():" << e.what();
        return nullptr;
    }
    
    // For now, always create offscreen texture (Qt Quick integration needs more work)
    if (!m_offscreenTexture && context) {
        qDebug() << "Creating offscreen texture because m_offscreenTexture is null";
        // Create offscreen texture with the current size
        const_cast<VulkanRendererBackend*>(this)->createOffscreenTexture(size);
    } else if (!m_offscreenTexture) {
        qDebug() << "m_offscreenTexture is null and context is null - cannot create texture";
        return nullptr;
    } else {
        qDebug() << "m_offscreenTexture already exists";
    }
    
    if (m_offscreenTexture) {
        qDebug() << "m_offscreenTexture exists, getting texture";
        // Return the underlying Vulkan texture
        auto texture = m_offscreenTexture->getTexture();
        if (texture) {
            qDebug() << "Got texture from m_offscreenTexture";
            // For Vulkan, we need to return the actual VkImage handle (not VkImageView)
            auto vulkanTexture = static_cast<mbgl::vulkan::Texture2D*>(texture.get());
            // Return the VkImage handle as void* for Qt Quick integration
            const vk::Image& vkImage = vulkanTexture->getVulkanImage();
            qDebug() << "Returning VkImage handle:" << (void*)static_cast<VkImage>(vkImage);
            return reinterpret_cast<void*>(static_cast<VkImage>(vkImage));
        } else {
            qDebug() << "Failed to get texture from m_offscreenTexture";
        }
    } else {
        qDebug() << "m_offscreenTexture is null after attempted creation";
    }
    
    qDebug() << "Returning nullptr from currentDrawable()";
    return nullptr;
}

void VulkanRendererBackend::createOffscreenTexture(const mbgl::Size& newSize) {
    if (context && !newSize.isEmpty()) {
        qDebug() << "Creating offscreen texture for VulkanRendererBackend with size:" << newSize.width << "x" << newSize.height;
        
        // Create an offscreen texture that we can render to
        // Use RGBA format for proper color rendering
        m_offscreenTexture = context->createOffscreenTexture(newSize, mbgl::gfx::TextureChannelDataType::UnsignedByte);
        
        if (m_offscreenTexture) {
            qDebug() << "Successfully created offscreen texture";
            
            // Force texture creation immediately
            auto texture = m_offscreenTexture->getTexture();
            if (texture) {
                texture->create();
                qDebug() << "Offscreen texture created and initialized";
            }
        } else {
            qDebug() << "Failed to create offscreen texture";
        }
    } else {
        if (!context) {
            qDebug() << "Cannot create offscreen texture: context is null";
        }
        if (newSize.isEmpty()) {
            qDebug() << "Cannot create offscreen texture: size is empty";
        }
    }
}

mbgl::vulkan::Texture2D* VulkanRendererBackend::getOffscreenTexture() const {
    if (m_offscreenTexture) {
        auto texture = m_offscreenTexture->getTexture();
        if (texture) {
            return static_cast<mbgl::vulkan::Texture2D*>(texture.get());
        }
    }
    return nullptr;
}

} // namespace QMapLibre

#endif // MLN_RENDER_BACKEND_VULKAN
