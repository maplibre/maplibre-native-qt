// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#include "source_parameter.hpp"

namespace QMapLibre {

SourceParameter::SourceParameter(QObject *parent)
    : StyleParameter(parent) {}

SourceParameter::~SourceParameter() = default;

QString SourceParameter::type() const {
    return m_type;
}

void SourceParameter::setType(const QString &type) {
    if (m_type.isEmpty()) {
        m_type = type;
    }
}

} // namespace QMapLibre
