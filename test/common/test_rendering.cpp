// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#include "test_rendering.hpp"

#include <QtGui/QOffscreenSurface>
#include <QtGui/QWindow>

#include <memory>

namespace QMapLibre {

std::unique_ptr<QSurface> createTestSurface(QSurface::SurfaceClass surfaceClass) {
    if (surfaceClass == QSurface::Window) {
        auto window = std::make_unique<QWindow>();
        window->setSurfaceType(QWindow::OpenGLSurface);
        window->setGeometry(0, 0, 1024, 1024);
        window->create();
        return window;
    }

    if (surfaceClass == QSurface::Offscreen) {
        // Create a window and get the format from that.  For example, if an EGL
        // implementation provides 565 and 888 configs for PBUFFER_BIT but only
        // 888 for WINDOW_BIT, we may end up with a pbuffer surface that is
        // incompatible with the context since it could choose the 565 while the
        // window and the context uses a config with 888.
        static QSurfaceFormat format;
        if (format.redBufferSize() == -1) {
            auto window = std::make_unique<QWindow>();
            window->setSurfaceType(QWindow::OpenGLSurface);
            window->setGeometry(0, 0, 10, 10);
            window->create();
            format = window->format();
        }
        auto offscreenSurface = std::make_unique<QOffscreenSurface>();
        offscreenSurface->setFormat(format);
        offscreenSurface->create();
        return offscreenSurface;
    }

    return nullptr;
}

} // namespace QMapLibre
