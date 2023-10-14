// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#ifndef QMAPLIBRE_EXPORT_H
#define QMAPLIBRE_EXPORT_H

// This header follows the Qt coding style: https://wiki.qt.io/Qt_Coding_Style

#if !defined(QT_MAPLIBRE_STATIC)
#if defined(QT_BUILD_MAPLIBRE_LIB)
#define Q_MAPLIBRE_EXPORT Q_DECL_EXPORT
#else
#define Q_MAPLIBRE_EXPORT Q_DECL_IMPORT
#endif
#else
#define Q_MAPLIBRE_EXPORT
#endif

#endif // QMAPLIBRE_EXPORT_H
