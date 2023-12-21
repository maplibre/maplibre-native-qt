// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QString>

namespace QMapLibre::StyleChangeUtils {

QList<QByteArray> allPropertyNamesList(const QObject *object);
QByteArray formatPropertyName(const QString &name);
QByteArray formatPropertyName(const QByteArray &name);

} // namespace QMapLibre::StyleChangeUtils
