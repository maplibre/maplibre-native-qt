// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include <QMapLibreWidgets/GLWidget>

#include <QtCore/QPropertyAnimation>

#include <memory>

namespace QMapLibre {

class GLTester : public GLWidget {
    Q_OBJECT

public:
    explicit GLTester(const QMapLibre::Settings &);

    void initializeAnimation();
    void initializeQuery();
    int selfTest();

private slots:
    void animationValueChanged();
    void animationFinished() const;

private:
    void paintGL() final;

    std::unique_ptr<QPropertyAnimation> m_bearingAnimation{};
    std::unique_ptr<QPropertyAnimation> m_zoomAnimation{};

    unsigned m_animationTicks{};
    unsigned m_frameDraws{};
};

} // namespace QMapLibre
