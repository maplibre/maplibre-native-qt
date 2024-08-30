// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#ifndef QMAPLIBRE_FILTER_PARAMETER_H
#define QMAPLIBRE_FILTER_PARAMETER_H

#include "style_parameter.hpp"

#include <QMapLibre/Export>

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QVariantList>

namespace QMapLibre {

class Q_MAPLIBRE_CORE_EXPORT FilterParameter : public StyleParameter {
    Q_OBJECT
public:
    explicit FilterParameter(QObject *parent = nullptr);
    ~FilterParameter() override;

    [[nodiscard]] QVariantList expression() const;
    void setExpression(const QVariantList &expression);

Q_SIGNALS:
    void expressionUpdated();

protected:
    QVariantList m_expression;

    Q_DISABLE_COPY(FilterParameter)
};

} // namespace QMapLibre

#endif // QMAPLIBRE_FILTER_PARAMETER_H
