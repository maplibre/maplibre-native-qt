// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#include <QtGui/QSurface>

#include <memory>

namespace QMapLibre {

std::unique_ptr<QSurface> createTestSurface(QSurface::SurfaceClass surfaceClass);

} // namespace QMapLibre
