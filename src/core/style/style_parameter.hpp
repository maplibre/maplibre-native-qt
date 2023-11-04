// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#ifndef QMAPLIBRE_STYLE_PARAMETER_H
#define QMAPLIBRE_STYLE_PARAMETER_H

#include <QMapLibre/Export>

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QVariantMap>

namespace QMapLibre {

class Q_MAPLIBRE_CORE_EXPORT StyleParameter : public QObject {
    Q_OBJECT
public:
    explicit StyleParameter(QObject *parent = nullptr);
    ~StyleParameter() override;

    bool operator==(const StyleParameter &other) const;

    [[nodiscard]] inline bool isReady() const { return m_ready; };

    bool hasProperty(const char *propertyName) const;
    void updateProperty(const char *propertyName, const QVariant &value);

    [[nodiscard]] QVariantMap toVariantMap() const;

    [[nodiscard]] QString styleId() const;
    void setStyleId(const QString &id);

public Q_SLOTS:
    void updateNotify();

Q_SIGNALS:
    void ready(StyleParameter *parameter);
    void updated(StyleParameter *parameter);

protected:
    const int m_initialPropertyCount = staticMetaObject.propertyCount();
    bool m_ready{};

    QString m_styleId;

    Q_DISABLE_COPY(StyleParameter)
};

} // namespace QMapLibre

#endif // QMAPLIBRE_STYLE_PARAMETER_H
