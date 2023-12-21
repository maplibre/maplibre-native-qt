// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#ifndef QMAPLIBRE_LAYER_PARAMETER_H
#define QMAPLIBRE_LAYER_PARAMETER_H

#include "style_parameter.hpp"

#include <QMapLibre/Export>

#include <QtCore/QJsonObject>
#include <QtCore/QObject>
#include <QtCore/QString>

namespace QMapLibre {

class Q_MAPLIBRE_CORE_EXPORT LayerParameter : public StyleParameter {
    Q_OBJECT
public:
    explicit LayerParameter(QObject *parent = nullptr);
    ~LayerParameter() override;

    [[nodiscard]] QString type() const;
    void setType(const QString &type);

    [[nodiscard]] QJsonObject layout() const;
    void setLayout(const QJsonObject &layout);
    Q_INVOKABLE void setLayoutProperty(const QString &key, const QVariant &value);

    [[nodiscard]] QJsonObject paint() const;
    void setPaint(const QJsonObject &paint);
    Q_INVOKABLE void setPaintProperty(const QString &key, const QVariant &value);

Q_SIGNALS:
    void layoutUpdated();
    void paintUpdated();

protected:
    QString m_type;
    QJsonObject m_layout;
    QJsonObject m_paint;

    Q_DISABLE_COPY(LayerParameter)
};

} // namespace QMapLibre

#endif // QMAPLIBRE_LAYER_PARAMETER_H
