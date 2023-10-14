// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#include <QtQml/qqml.h>
#include <QtQml/qqmlmoduleregistration.h>

#include <qml_types.hpp>

#if !defined(QT_STATIC)
#define Q_QMLTYPE_EXPORT Q_DECL_EXPORT
#else
#define Q_QMLTYPE_EXPORT
#endif
Q_QMLTYPE_EXPORT void qml_register_types_QtLocation_MapLibre() {
    qmlRegisterTypesAndRevisions<MapLibreStyleAttached>("QtLocation.MapLibre", 3);
    qmlRegisterTypesAndRevisions<MapLibreStyleProperties>("QtLocation.MapLibre", 3);
    qmlRegisterModule("QtLocation.MapLibre", 3, 0);
}

static const QQmlModuleRegistration registration("QtLocation.MapLibre", 3, qml_register_types_QtLocation_MapLibre);
