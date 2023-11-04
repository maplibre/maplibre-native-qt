// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include <QMapLibreWidgets/GLWidget>

#include <QtCore/QPropertyAnimation>

namespace QMapLibre {

class GLTester : public GLWidget {
    Q_OBJECT

public:
    explicit GLTester(const QMapLibre::Settings &);

    void initializeAnimation();
    int selfTest();

private slots:
    void animationValueChanged();
    void animationFinished();

private:
    void paintGL() final;

    QPropertyAnimation *m_bearingAnimation{};
    QPropertyAnimation *m_zoomAnimation{};

    unsigned m_animationTicks{};
    unsigned m_frameDraws{};
};

} // namespace QMapLibre
