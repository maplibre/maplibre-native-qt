// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include <mln/actor/actor.hpp>
#include <mln/actor/actor_ref.hpp>
#include <mln/actor/mailbox.hpp>
#include <mln/renderer/renderer_observer.hpp>
#include <mln/util/run_loop.hpp>

#include <memory>

namespace QMapLibre {

// Forwards RendererObserver signals to the given
// Delegate RendererObserver on the given RunLoop
class RendererObserver final : public mln::RendererObserver {
public:
    RendererObserver(mln::util::RunLoop &mapRunLoop, mln::RendererObserver &delegate_)
        : mailbox(std::make_shared<mln::Mailbox>(mapRunLoop)),
          delegate(delegate_, mailbox) {}

    ~RendererObserver() final { mailbox->close(); }

    void onInvalidate() final { delegate.invoke(&mln::RendererObserver::onInvalidate); }

    void onResourceError(std::exception_ptr err) final {
        delegate.invoke(&mln::RendererObserver::onResourceError, err);
    }

    void onWillStartRenderingMap() final { delegate.invoke(&mln::RendererObserver::onWillStartRenderingMap); }

    void onWillStartRenderingFrame() final { delegate.invoke(&mln::RendererObserver::onWillStartRenderingFrame); }

    void onDidFinishRenderingFrame(RenderMode mode, bool repaint, bool placementChanged) final {
        void (mln::RendererObserver::*f)(RenderMode, bool, bool) = &mln::RendererObserver::onDidFinishRenderingFrame;
        delegate.invoke(f, mode, repaint, placementChanged);
    }

    void onDidFinishRenderingFrame(RenderMode mode,
                                   bool repaint,
                                   bool placementChanged,
                                   double frameEncodingTime,
                                   double frameRenderingTime) final {
        void (mln::RendererObserver::*f)(
            RenderMode, bool, bool, double, double) = &mln::RendererObserver::onDidFinishRenderingFrame;
        delegate.invoke(f, mode, repaint, placementChanged, frameEncodingTime, frameRenderingTime);
    }

    void onDidFinishRenderingFrame(RenderMode mode,
                                   bool repaint,
                                   bool placementChanged,
                                   const mln::gfx::RenderingStats &stats) final {
        void (mln::RendererObserver::*f)(RenderMode, bool, bool, const mln::gfx::RenderingStats &) =
            &mln::RendererObserver::onDidFinishRenderingFrame;
        delegate.invoke(f, mode, repaint, placementChanged, stats);
    }

    void onDidFinishRenderingMap() final { delegate.invoke(&mln::RendererObserver::onDidFinishRenderingMap); }

    void onStyleImageMissing(const std::string &id, const StyleImageMissingCallback &done) override {
        delegate.invoke(&mln::RendererObserver::onStyleImageMissing, id, done);
    }

    void onRemoveUnusedStyleImages(const std::vector<std::string> &ids) override {
        delegate.invoke(&mln::RendererObserver::onRemoveUnusedStyleImages, ids);
    }

    void onPreCompileShader(mln::shaders::BuiltIn id,
                            mln::gfx::Backend::Type type,
                            const std::string &additionalDefines) override {
        delegate.invoke(&mln::RendererObserver::onPreCompileShader, id, type, additionalDefines);
    }

    void onPostCompileShader(mln::shaders::BuiltIn id,
                             mln::gfx::Backend::Type type,
                             const std::string &additionalDefines) override {
        delegate.invoke(&mln::RendererObserver::onPostCompileShader, id, type, additionalDefines);
    }

    void onShaderCompileFailed(mln::shaders::BuiltIn id,
                               mln::gfx::Backend::Type type,
                               const std::string &additionalDefines) override {
        delegate.invoke(&mln::RendererObserver::onShaderCompileFailed, id, type, additionalDefines);
    }

    void onGlyphsLoaded(const mln::FontStack &stack, const mln::GlyphRange &range) override {
        delegate.invoke(&mln::RendererObserver::onGlyphsLoaded, stack, range);
    }

    void onGlyphsError(const mln::FontStack &stack, const mln::GlyphRange &range, std::exception_ptr ex) override {
        delegate.invoke(&mln::RendererObserver::onGlyphsError, stack, range, ex);
    }

    void onGlyphsRequested(const mln::FontStack &stack, const mln::GlyphRange &range) override {
        delegate.invoke(&mln::RendererObserver::onGlyphsRequested, stack, range);
    }

    void onTileAction(mln::TileOperation op, const mln::OverscaledTileID &id, const std::string &sourceID) override {
        delegate.invoke(&mln::RendererObserver::onTileAction, op, id, sourceID);
    }

private:
    std::shared_ptr<mln::Mailbox> mailbox;
    mln::ActorRef<mln::RendererObserver> delegate;
};

} // namespace QMapLibre
