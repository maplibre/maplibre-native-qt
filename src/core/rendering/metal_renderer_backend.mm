// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_OSX || TARGET_OS_IPHONE

#include "metal_renderer_backend_p.hpp"

#include <mln/gfx/backend_scope.hpp>
#include <mln/gfx/context.hpp>
#include <mln/mtl/renderable_resource.hpp>
#include <mln/mtl/texture2d.hpp>

#import <Metal/Metal.hpp>
#import <QuartzCore/CAMetalLayer.hpp>

#include <QDebug>

#include <cassert>

namespace {

constexpr uint32_t defaultOffscreenWidth = 800;
constexpr uint32_t defaultOffscreenHeight = 600;

class QtMetalRenderableResource final : public mln::mtl::RenderableResource {
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

    void setBackendSize(mln::Size size_) {
        if (size_.width == size.width && size_.height == size.height) {
            return; // No change in size, nothing to do.
        }

#ifdef MLN_RENDERER_DEBUGGING
        qDebug() << "QtMetalRenderableResource::setBackendSize() -" << size_.width << "x" << size_.height;
#endif
        size = size_;
        if (layer != nullptr) {
            layer->setDrawableSize(
                {.width = static_cast<CGFloat>(size.width), .height = static_cast<CGFloat>(size.height)});
        }
        buffersInvalid = true;
    }

    [[nodiscard]] mln::Size getSize() const { return size; }

    // — mln::mtl::RenderableResource -----------------------------------
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
        auto texSize = mln::Size{size.width, size.height};
        if (externalTex != nullptr) {
            // If texture is available, use its size
            texSize = mln::Size{static_cast<uint32_t>(externalTex->width()),
                                 static_cast<uint32_t>(externalTex->height())};
        } else if (layer != nullptr) {
            // Only use layer size when the texture is not available
            texSize = mln::Size{static_cast<uint32_t>(layer->drawableSize().width),
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
            depthTexture->setFormat(mln::gfx::TexturePixelType::Depth, mln::gfx::TextureChannelDataType::Float);
            depthTexture->setSamplerConfiguration({.filter = mln::gfx::TextureFilterType::Linear,
                                                   .wrapU = mln::gfx::TextureWrapType::Clamp,
                                                   .wrapV = mln::gfx::TextureWrapType::Clamp});
            static_cast<mln::mtl::Texture2D *>(depthTexture.get())
                ->setUsage(MTL::TextureUsageShaderRead | MTL::TextureUsageShaderWrite | MTL::TextureUsageRenderTarget);

            stencilTexture = backend.getContext().createTexture2D();
            stencilTexture->setSize(texSize);
            stencilTexture->setFormat(mln::gfx::TexturePixelType::Stencil,
                                      mln::gfx::TextureChannelDataType::UnsignedByte);
            stencilTexture->setSamplerConfiguration({.filter = mln::gfx::TextureFilterType::Linear,
                                                     .wrapU = mln::gfx::TextureWrapType::Clamp,
                                                     .wrapV = mln::gfx::TextureWrapType::Clamp});
            static_cast<mln::mtl::Texture2D *>(stencilTexture.get())
                ->setUsage(MTL::TextureUsageShaderRead | MTL::TextureUsageShaderWrite | MTL::TextureUsageRenderTarget);
        }

        if (depthTexture) {
            depthTexture->create();
            // Using depth texture
            if (auto *depthTarget = renderPassDescriptor->depthAttachment()) {
                depthTarget->setTexture(static_cast<mln::mtl::Texture2D *>(depthTexture.get())->getMetalTexture());
            }
        }
        if (stencilTexture) {
            stencilTexture->create();
            // Using stencil texture
            if (auto *stencilTarget = renderPassDescriptor->stencilAttachment()) {
                stencilTarget->setTexture(static_cast<mln::mtl::Texture2D *>(stencilTexture.get())->getMetalTexture());
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

    [[nodiscard]] const mln::mtl::RendererBackend &getBackend() const override { return backend; }

    [[nodiscard]] const mln::mtl::MTLCommandBufferPtr &getCommandBuffer() const override { return commandBuffer; }

    [[nodiscard]] mln::mtl::MTLBlitPassDescriptorPtr getUploadPassDescriptor() const override {
        return NS::TransferPtr(MTL::BlitPassDescriptor::alloc()->init());
    }

    [[nodiscard]] const mln::mtl::MTLRenderPassDescriptorPtr &getRenderPassDescriptor() const override {
        return renderPassDescriptor;
    }

private:
    QMapLibre::MetalRendererBackend &backend;
    LayerPtr layer;
    mln::mtl::MTLCommandQueuePtr commandQueue;
    mln::mtl::MTLCommandBufferPtr commandBuffer;
    mln::mtl::MTLRenderPassDescriptorPtr renderPassDescriptor;
    mln::mtl::CAMetalDrawablePtr surface;
    mln::gfx::Texture2DPtr depthTexture;
    mln::gfx::Texture2DPtr stencilTexture;
    mln::Size size{0, 0};
    bool buffersInvalid{true};
};

} // namespace

namespace QMapLibre {

MetalRendererBackend::MetalRendererBackend(CA::MetalLayer *layer)
    : mln::mtl::RendererBackend(mln::gfx::ContextMode::Unique),
      mln::gfx::Renderable(layer != nullptr ? mln::Size{static_cast<uint32_t>(layer->drawableSize().width),
                                                          static_cast<uint32_t>(layer->drawableSize().height)}
                                             : mln::Size{defaultOffscreenWidth, defaultOffscreenHeight},
                            std::make_unique<QtMetalRenderableResource>(*this, layer)) {}

MetalRendererBackend::~MetalRendererBackend() = default;

void MetalRendererBackend::setSize(mln::Size size_) {
    // Propagate size to the Renderable base class
    mln::gfx::Renderable::size = size_;

    getResource<QtMetalRenderableResource>().setBackendSize(size_);
}

mln::Size MetalRendererBackend::getSize() const {
    return getResource<QtMetalRenderableResource>().getSize();
}

} // namespace QMapLibre

#endif // TARGET_OS_OSX || TARGET_OS_IPHONE
#endif // __APPLE__
