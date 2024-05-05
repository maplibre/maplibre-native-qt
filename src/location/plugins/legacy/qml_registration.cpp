// Copyright (C) 2024 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#include <QtQml/qqml.h>
#include <QtQml/qqmlmoduleregistration.h>

#include <declarative_layer_parameter.hpp>
#include <declarative_source_parameter.hpp>
#include <declarative_style.hpp>
#include <qml_types.hpp>

#if !defined(QT_STATIC)
#define Q_QMLTYPE_EXPORT Q_DECL_EXPORT
#else
#define Q_QMLTYPE_EXPORT
#endif
Q_QMLTYPE_EXPORT void qml_register_types_MapLibre() {
    qmlRegisterTypesAndRevisions<MapLibreStyleAttached>("MapLibre", 3);
    qmlRegisterTypesAndRevisions<MapLibreStyleProperties>("MapLibre", 3);
    qmlRegisterTypesAndRevisions<QMapLibre::DeclarativeLayerParameter>("MapLibre", 3);
    qmlRegisterTypesAndRevisions<QMapLibre::DeclarativeSourceParameter>("MapLibre", 3);
    qmlRegisterTypesAndRevisions<QMapLibre::DeclarativeStyle>("MapLibre", 3);
    qmlRegisterAnonymousType<QQuickItem>("MapLibre", 3);
    qmlRegisterModule("MapLibre", 3, 0);
}

static const QQmlModuleRegistration registration("MapLibre", 3, qml_register_types_MapLibre);
