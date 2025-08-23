// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include <mbgl/actor/actor.hpp>
#include <mbgl/actor/actor_ref.hpp>
#include <mbgl/actor/mailbox.hpp>
#include <mbgl/renderer/renderer_observer.hpp>
#include <mbgl/util/run_loop.hpp>

#include <memory>

namespace QMapLibre {

// Forwards RendererObserver signals to the given
// Delegate RendererObserver on the given RunLoop
class RendererObserver final : public mbgl::RendererObserver {
public:
    RendererObserver(mbgl::util::RunLoop& mapRunLoop, mbgl::RendererObserver& delegate_)
        : mailbox(std::make_shared<mbgl::Mailbox>(mapRunLoop)),
          delegate(delegate_, mailbox) {}

    ~RendererObserver() final { mailbox->close(); }

    void onInvalidate() final { delegate.invoke(&mbgl::RendererObserver::onInvalidate); }

    void onResourceError(std::exception_ptr err) final {
        delegate.invoke(&mbgl::RendererObserver::onResourceError, err);
    }

    void onWillStartRenderingMap() final { delegate.invoke(&mbgl::RendererObserver::onWillStartRenderingMap); }

    void onWillStartRenderingFrame() final { delegate.invoke(&mbgl::RendererObserver::onWillStartRenderingFrame); }

    void onDidFinishRenderingFrame(RenderMode mode, bool repaint, bool placementChanged) final {
        void (mbgl::RendererObserver::*f)(RenderMode, bool, bool) = &mbgl::RendererObserver::onDidFinishRenderingFrame;
        delegate.invoke(f, mode, repaint, placementChanged);
    }

    void onDidFinishRenderingFrame(RenderMode mode,
                                   bool repaint,
                                   bool placementChanged,
                                   double frameEncodingTime,
                                   double frameRenderingTime) final {
        void (mbgl::RendererObserver::*f)(
            RenderMode, bool, bool, double, double) = &mbgl::RendererObserver::onDidFinishRenderingFrame;
        delegate.invoke(f, mode, repaint, placementChanged, frameEncodingTime, frameRenderingTime);
    }

    void onDidFinishRenderingFrame(RenderMode mode,
                                   bool repaint,
                                   bool placementChanged,
                                   const mbgl::gfx::RenderingStats& stats) final {
        void (mbgl::RendererObserver::*f)(RenderMode, bool, bool, const mbgl::gfx::RenderingStats&) =
            &mbgl::RendererObserver::onDidFinishRenderingFrame;
        delegate.invoke(f, mode, repaint, placementChanged, stats);
    }

    void onDidFinishRenderingMap() final { delegate.invoke(&mbgl::RendererObserver::onDidFinishRenderingMap); }

    void onStyleImageMissing(const std::string& id, const StyleImageMissingCallback& done) override {
        delegate.invoke(&mbgl::RendererObserver::onStyleImageMissing, id, done);
    }

    void onRemoveUnusedStyleImages(const std::vector<std::string>& ids) override {
        delegate.invoke(&mbgl::RendererObserver::onRemoveUnusedStyleImages, ids);
    }

    void onPreCompileShader(mbgl::shaders::BuiltIn id,
                            mbgl::gfx::Backend::Type type,
                            const std::string& additionalDefines) override {
        delegate.invoke(&mbgl::RendererObserver::onPreCompileShader, id, type, additionalDefines);
    }

    void onPostCompileShader(mbgl::shaders::BuiltIn id,
                             mbgl::gfx::Backend::Type type,
                             const std::string& additionalDefines) override {
        delegate.invoke(&mbgl::RendererObserver::onPostCompileShader, id, type, additionalDefines);
    }

    void onShaderCompileFailed(mbgl::shaders::BuiltIn id,
                               mbgl::gfx::Backend::Type type,
                               const std::string& additionalDefines) override {
        delegate.invoke(&mbgl::RendererObserver::onShaderCompileFailed, id, type, additionalDefines);
    }

    void onGlyphsLoaded(const mbgl::FontStack& stack, const mbgl::GlyphRange& range) override {
        delegate.invoke(&mbgl::RendererObserver::onGlyphsLoaded, stack, range);
    }

    void onGlyphsError(const mbgl::FontStack& stack, const mbgl::GlyphRange& range, std::exception_ptr ex) override {
        delegate.invoke(&mbgl::RendererObserver::onGlyphsError, stack, range, ex);
    }

    void onGlyphsRequested(const mbgl::FontStack& stack, const mbgl::GlyphRange& range) override {
        delegate.invoke(&mbgl::RendererObserver::onGlyphsRequested, stack, range);
    }

    void onTileAction(mbgl::TileOperation op, const mbgl::OverscaledTileID& id, const std::string& sourceID) override {
        delegate.invoke(&mbgl::RendererObserver::onTileAction, op, id, sourceID);
    }

private:
    std::shared_ptr<mbgl::Mailbox> mailbox;
    mbgl::ActorRef<mbgl::RendererObserver> delegate;
};

} // namespace QMapLibre
