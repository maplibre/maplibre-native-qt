// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#ifndef QMAPLIBRE_SOURCE_PARAMETER_H
#define QMAPLIBRE_SOURCE_PARAMETER_H

#include "style_parameter.hpp"

#include <QMapLibre/Export>

#include <QtCore/QObject>
#include <QtCore/QString>

namespace QMapLibre {

class Q_MAPLIBRE_CORE_EXPORT SourceParameter : public StyleParameter {
    Q_OBJECT
public:
    explicit SourceParameter(QObject *parent = nullptr);
    ~SourceParameter() override;

    [[nodiscard]] QString type() const;
    void setType(const QString &type);

protected:
    QString m_type;

    Q_DISABLE_COPY(SourceParameter)
};

} // namespace QMapLibre

#endif // QMAPLIBRE_SOURCE_PARAMETER_H
