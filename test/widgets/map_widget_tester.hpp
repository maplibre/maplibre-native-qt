// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include <QMapLibre/Settings>
#include <QMapLibreWidgets/MapWidget>

#include <QtCore/QPropertyAnimation>

#include <memory>

namespace QMapLibre::Test {

class MapWidgetTester : public MapWidget {
    Q_OBJECT

public:
    explicit MapWidgetTester(const QMapLibre::Settings &);

    void initializeAnimation();
    int selfTest();

private slots:
    void animationValueChanged();
    void animationFinished() const;

private:
    std::unique_ptr<QPropertyAnimation> m_bearingAnimation;
    std::unique_ptr<QPropertyAnimation> m_zoomAnimation;

    unsigned m_animationTicks{};
    unsigned m_frameDraws{};
};

} // namespace QMapLibre::Test
