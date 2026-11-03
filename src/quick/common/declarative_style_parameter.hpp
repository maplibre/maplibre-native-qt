// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include <QMapLibre/StyleParameter>

#include <QtCore/QMetaMethod>
#include <QtCore/QMetaObject>
#include <QtCore/QMetaProperty>

#define MLN_DECLARATIVE_PARSER(Class)                                                                           \
protected:                                                                                                      \
    void classBegin() override {}                                                                               \
    void componentComplete() override {                                                                         \
        for (int i = m_initialPropertyCount; i < metaObject()->propertyCount(); ++i) {                          \
            const QMetaProperty property = metaObject()->property(i);                                           \
            if (!property.hasNotifySignal()) continue;                                                          \
                                                                                                                \
            const QMetaMethod notify = staticMetaObject.method(staticMetaObject.indexOfSlot("updateNotify()")); \
            QObject::connect(this, property.notifySignal(), this, notify);                                      \
        }                                                                                                       \
        m_ready = true;                                                                                         \
        emit ready(this);                                                                                       \
    }
