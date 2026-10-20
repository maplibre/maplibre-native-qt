// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include <mln/gfx/context.hpp>
#include <mln/gfx/texture2d.hpp>
#include <mln/vulkan/renderable_resource.hpp>
#include <mln/vulkan/renderer_backend.hpp>

#include <QtCore/QtGlobal>

#include <vulkan/vulkan.hpp>

class QVulkanInstance;
class QVulkanWindow;
class QWindow;

namespace mln::vulkan {
class Texture2D;
} // namespace mln::vulkan

namespace QMapLibre {

class VulkanRendererBackend final : public mln::vulkan::RendererBackend, public mln::vulkan::Renderable {
public:
    explicit VulkanRendererBackend(QWindow *window);
    explicit VulkanRendererBackend(QVulkanInstance *instance);
    // Constructor that uses Qt's Vulkan device
    VulkanRendererBackend(QWindow *window,
                          vk::PhysicalDevice qtPhysicalDevice,
                          vk::Device qtDevice,
                          uint32_t qtGraphicsQueueIndex);
    VulkanRendererBackend(const VulkanRendererBackend &) = delete;
    VulkanRendererBackend &operator=(const VulkanRendererBackend &) = delete;
    ~VulkanRendererBackend() override;

    // mln::gfx::RendererBackend ------------------------------------------------
    mln::gfx::Renderable &getDefaultRenderable() override { return static_cast<mln::gfx::Renderable &>(*this); }
    void activate() override {}
    void deactivate() override {}

    // Qt-specific --------------------------------------------------------------
    void setSize(mln::Size size_);
    [[nodiscard]] mln::Size getSize() const;

    // Returns the color texture of the drawable rendered in the last frame.
    [[nodiscard]] void *currentDrawable() const { return m_currentDrawable; }
    void setCurrentDrawable(void *tex) { m_currentDrawable = static_cast<mln::gfx::Texture2D *>(tex); }

    // Set external vk::Image to render to (for zero-copy with QRhiWidget)
    void setExternalDrawable(void *image, const mln::Size &size_);

    // Qt Widgets path still expects this hook even though Vulkan doesn't use an
    // OpenGL FBO. Update the size for Vulkan rendering.
    void updateRenderer(const mln::Size &newSize, uint32_t /* fbo */) { setSize(newSize); }

    // Helper method to get the texture object for pixel data extraction
    [[nodiscard]] mln::vulkan::Texture2D *getOffscreenTexture() const;

protected:
    // Override base class methods to use external Vulkan resources
    void init();
    void initInstance() override;
    void initSurface() override;
    void initDevice() override;
    void initSwapchain() override;
    std::vector<const char *> getDeviceExtensions() override;

private:
    mln::gfx::Texture2D *m_currentDrawable{nullptr};
    QVulkanInstance *m_qtInstance{nullptr};
    std::unique_ptr<QVulkanInstance> m_ownedInstance; // Instance we created and own

    // Qt device info
    vk::PhysicalDevice m_qtPhysicalDevice{nullptr};
    vk::Device m_qtDevice{nullptr};
    uint32_t m_qtGraphicsQueueIndex{0};
    bool m_useQtDevice{false};

    void initializeWithQtInstance(QVulkanInstance *qtInstance);
    static std::unique_ptr<QVulkanInstance> createQVulkanInstance();
};

} // namespace QMapLibre
