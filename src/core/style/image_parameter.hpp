// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#ifndef QMAPLIBRE_IMAGE_PARAMETER_H
#define QMAPLIBRE_IMAGE_PARAMETER_H

#include "style_parameter.hpp"

#include <QMapLibre/Export>

#include <QtCore/QObject>
#include <QtCore/QString>

namespace QMapLibre {

class Q_MAPLIBRE_CORE_EXPORT ImageParameter : public StyleParameter {
    Q_OBJECT
public:
    explicit ImageParameter(QObject* parent = nullptr);
    ~ImageParameter() override;

    [[nodiscard]] QString source() const;
    void setSource(const QString& source);

Q_SIGNALS:
    void sourceUpdated();

protected:
    QString m_source;

    Q_DISABLE_COPY(ImageParameter)
};

} // namespace QMapLibre

#endif // QMAPLIBRE_IMAGE_PARAMETER_H
