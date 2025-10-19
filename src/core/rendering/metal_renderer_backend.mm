// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_OSX || TARGET_OS_IPHONE

#include "metal_renderer_backend_p.hpp"

#include <mbgl/gfx/backend_scope.hpp>
#include <mbgl/gfx/context.hpp>
#include <mbgl/mtl/renderable_resource.hpp>
#include <mbgl/mtl/texture2d.hpp>

#import <Metal/Metal.hpp>
#import <QuartzCore/CAMetalLayer.hpp>

#include <QDebug>

#include <cassert>

namespace {

constexpr uint32_t defaultOffscreenWidth = 800;
constexpr uint32_t defaultOffscreenHeight = 600;

class QtMetalRenderableResource final : public mbgl::mtl::RenderableResource {
public:
    using LayerPtr = CA::MetalLayer *;
    QtMetalRenderableResource(QMapLibre::MetalRendererBackend &backend_, LayerPtr layer_)
        : backend(backend_),
          layer(layer_),
          commandQueue(NS::TransferPtr(backend.getDevice()->newCommandQueue())) {
        if (layer != nullptr) {
            layer->setDevice(backend.getDevice().get());
#ifdef MLN_RENDERER_DEBUGGING
            qDebug() << "QtMetalRenderableResource() - Created renderable resource with size"
                     << layer->drawableSize().width << "x" << layer->drawableSize().height;
#endif
        } else {
#ifdef MLN_RENDERER_DEBUGGING
            qDebug() << "QtMetalRenderableResource() - Will use external drawable";
#endif
        }
    }

    void setBackendSize(mbgl::Size size_) {
        if (size_.width == size.width && size_.height == size.height) {
            return; // No change in size, nothing to do.
        }

#ifdef MLN_RENDERER_DEBUGGING
        qDebug() << "QtMetalRenderableResource::setBackendSize() -" << size_.width << "x" << size_.height;
#endif
        size = size_;
        if (layer != nullptr) {
            layer->setDrawableSize({static_cast<CGFloat>(size.width), static_cast<CGFloat>(size.height)});
        }
        buffersInvalid = true;
    }

    [[nodiscard]] mbgl::Size getSize() const { return size; }

    // — mbgl::mtl::RenderableResource -----------------------------------
    void bind() override {
#ifdef MLN_RENDERER_DEBUGGING
        qDebug() << "QtMetalRenderableResource::bind() - Binding renderable resource with size" << size.width << "x"
                 << size.height;
#endif

        // Qt Quick may supply us with the current swap-chain texture via
        // MetalRendererBackend::setCurrentDrawable().  Use that texture if
        // present to avoid contending for a second drawable from the same
        // CAMetalLayer.
        // Binding renderable resource

        // First check if we have an external drawable from QRhiWidget
        auto *externalTex = static_cast<MTL::Texture *>(backend.externalDrawable());
        if (externalTex == nullptr) {
            // Fall back to checking for Quick drawable
            externalTex = static_cast<MTL::Texture *>(backend.currentDrawable());
        }

        // If we have a layer and no external texture, try to get a drawable from it
        if (externalTex == nullptr && layer != nullptr) {
            auto *tmpDrawable = layer->nextDrawable();
            if (tmpDrawable == nullptr) {
                qWarning() << "QtMetalRenderableResource::bind() - nextDrawable() returned nil."
                           << "drawableSize=" << layer->drawableSize().width << "x" << layer->drawableSize().height
                           << ", device=" << (void *)layer->device()
                           << ", pixelFormat=" << static_cast<int>(layer->pixelFormat());

                // Still allocate a command buffer so subsequent frames continue
                commandBuffer = NS::RetainPtr(commandQueue->commandBuffer());
                // Leave renderPassDescriptor null; caller will treat this as
                // an empty frame.
                return;
            }

            surface = NS::RetainPtr(tmpDrawable);
            externalTex = surface->texture();
            backend.setCurrentDrawable(externalTex);
        }

        // If still no texture and no layer, we can't render
        if (externalTex == nullptr) {
            qWarning() << "QtMetalRenderableResource::bind() - No drawable texture available for rendering";
            // Create a command buffer anyway to avoid crashes
            commandBuffer = NS::RetainPtr(commandQueue->commandBuffer());
            return;
        }

        // Get texture size from the actual texture
        auto texSize = mbgl::Size{size.width, size.height};
        if (externalTex != nullptr) {
            // If texture is available, use its size
            texSize = mbgl::Size{static_cast<uint32_t>(externalTex->width()),
                                 static_cast<uint32_t>(externalTex->height())};
        } else if (layer != nullptr) {
            // Only use layer size when the texture is not available
            texSize = mbgl::Size{static_cast<uint32_t>(layer->drawableSize().width),
                                 static_cast<uint32_t>(layer->drawableSize().height)};
        }

        commandBuffer = NS::RetainPtr(commandQueue->commandBuffer());
        renderPassDescriptor = NS::RetainPtr(MTL::RenderPassDescriptor::alloc()->init());
        renderPassDescriptor->colorAttachments()->object(0)->setTexture(externalTex);

        if (buffersInvalid || !depthTexture || !stencilTexture) {
#ifdef MLN_RENDERER_DEBUGGING
            qDebug() << "QtMetalRenderableResource::bind() - Allocating depth/stencil textures with size"
                     << texSize.width << "x" << texSize.height;
#endif
            buffersInvalid = false;
            depthTexture = backend.getContext().createTexture2D();
            depthTexture->setSize(texSize);
            depthTexture->setFormat(mbgl::gfx::TexturePixelType::Depth, mbgl::gfx::TextureChannelDataType::Float);
            depthTexture->setSamplerConfiguration({.filter = mbgl::gfx::TextureFilterType::Linear,
                                                   .wrapU = mbgl::gfx::TextureWrapType::Clamp,
                                                   .wrapV = mbgl::gfx::TextureWrapType::Clamp});
            static_cast<mbgl::mtl::Texture2D *>(depthTexture.get())
                ->setUsage(MTL::TextureUsageShaderRead | MTL::TextureUsageShaderWrite | MTL::TextureUsageRenderTarget);

            stencilTexture = backend.getContext().createTexture2D();
            stencilTexture->setSize(texSize);
            stencilTexture->setFormat(mbgl::gfx::TexturePixelType::Stencil,
                                      mbgl::gfx::TextureChannelDataType::UnsignedByte);
            stencilTexture->setSamplerConfiguration({.filter = mbgl::gfx::TextureFilterType::Linear,
                                                     .wrapU = mbgl::gfx::TextureWrapType::Clamp,
                                                     .wrapV = mbgl::gfx::TextureWrapType::Clamp});
            static_cast<mbgl::mtl::Texture2D *>(stencilTexture.get())
                ->setUsage(MTL::TextureUsageShaderRead | MTL::TextureUsageShaderWrite | MTL::TextureUsageRenderTarget);
        }

        if (depthTexture) {
            depthTexture->create();
            // Using depth texture
            if (auto *depthTarget = renderPassDescriptor->depthAttachment()) {
                depthTarget->setTexture(static_cast<mbgl::mtl::Texture2D *>(depthTexture.get())->getMetalTexture());
            }
        }
        if (stencilTexture) {
            stencilTexture->create();
            // Using stencil texture
            if (auto *stencilTarget = renderPassDescriptor->stencilAttachment()) {
                stencilTarget->setTexture(static_cast<mbgl::mtl::Texture2D *>(stencilTexture.get())->getMetalTexture());
            }
        }
    }

    void swap() override {
        // For external render targets (QRhiWidget), we don't need to present
        // The texture is already being used by QRhiWidget directly
        if (surface) {
            commandBuffer->presentDrawable(surface.get());
        }

        // Always commit the command buffer whether we have a surface or not
        // This ensures the rendering commands are executed for external textures
        if (commandBuffer) {
            commandBuffer->commit();
            // Wait for completion when using external texture to ensure
            // QRhiWidget can display the result
            if (!surface && backend.externalDrawable() != nullptr) {
                commandBuffer->waitUntilCompleted();
            }
            commandBuffer.reset();
        }
        renderPassDescriptor.reset();
    }

    [[nodiscard]] const mbgl::mtl::RendererBackend &getBackend() const override { return backend; }

    [[nodiscard]] const mbgl::mtl::MTLCommandBufferPtr &getCommandBuffer() const override { return commandBuffer; }

    [[nodiscard]] mbgl::mtl::MTLBlitPassDescriptorPtr getUploadPassDescriptor() const override {
        return NS::RetainPtr(MTL::BlitPassDescriptor::alloc()->init());
    }

    [[nodiscard]] const mbgl::mtl::MTLRenderPassDescriptorPtr &getRenderPassDescriptor() const override {
        return renderPassDescriptor;
    }

private:
    QMapLibre::MetalRendererBackend &backend;
    LayerPtr layer;
    mbgl::mtl::MTLCommandQueuePtr commandQueue;
    mbgl::mtl::MTLCommandBufferPtr commandBuffer;
    mbgl::mtl::MTLRenderPassDescriptorPtr renderPassDescriptor;
    mbgl::mtl::CAMetalDrawablePtr surface;
    mbgl::gfx::Texture2DPtr depthTexture;
    mbgl::gfx::Texture2DPtr stencilTexture;
    mbgl::Size size{0, 0};
    bool buffersInvalid{true};
};

} // namespace

namespace QMapLibre {

MetalRendererBackend::MetalRendererBackend(CA::MetalLayer *layer)
    : mbgl::mtl::RendererBackend(mbgl::gfx::ContextMode::Unique),
      mbgl::gfx::Renderable(layer != nullptr ? mbgl::Size{static_cast<uint32_t>(layer->drawableSize().width),
                                                          static_cast<uint32_t>(layer->drawableSize().height)}
                                             : mbgl::Size{defaultOffscreenWidth, defaultOffscreenHeight},
                            std::make_unique<QtMetalRenderableResource>(*this, layer)) {}

MetalRendererBackend::~MetalRendererBackend() = default;

void MetalRendererBackend::setSize(mbgl::Size size_) {
    getResource<QtMetalRenderableResource>().setBackendSize(size_);
}

mbgl::Size MetalRendererBackend::getSize() const {
    return getResource<QtMetalRenderableResource>().getSize();
}

} // namespace QMapLibre

#endif // TARGET_OS_OSX || TARGET_OS_IPHONE
#endif // __APPLE__
