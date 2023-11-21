// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#include "gl_tester.hpp"

#include <QtCore/QDebug>

namespace {
constexpr int kAnimationDuration = 5000;
} // anonymous namespace

namespace QMapLibre {

GLTester::GLTester(const QMapLibre::Settings &settings)
    : GLWidget(settings) {}

void GLTester::initializeAnimation() {
    m_bearingAnimation = std::make_unique<QPropertyAnimation>(map(), "bearing");
    m_zoomAnimation = std::make_unique<QPropertyAnimation>(map(), "zoom");

    connect(m_zoomAnimation.get(), &QPropertyAnimation::finished, this, &GLTester::animationFinished);
    connect(m_zoomAnimation.get(), &QPropertyAnimation::valueChanged, this, &GLTester::animationValueChanged);
}

int GLTester::selfTest() {
    if (m_bearingAnimation) {
        m_bearingAnimation->setDuration(kAnimationDuration);
        m_bearingAnimation->setEndValue(map()->bearing() + 360 * 4);
        m_bearingAnimation->start();
    }

    if (m_zoomAnimation) {
        m_zoomAnimation->setDuration(kAnimationDuration);
        m_zoomAnimation->setEndValue(map()->zoom() + 3);
        m_zoomAnimation->start();
    }

    return kAnimationDuration;
}

void GLTester::animationValueChanged() {
    m_animationTicks++;
}

void GLTester::animationFinished() const {
    qDebug() << "Animation ticks/s: "
             << static_cast<float>(m_animationTicks) / static_cast<float>(kAnimationDuration) * 1000.f;
    qDebug() << "Frame draws/s: " << static_cast<float>(m_frameDraws) / static_cast<float>(kAnimationDuration) * 1000.f;
}

void GLTester::paintGL() {
    m_frameDraws++;
    GLWidget::paintGL();
}

} // namespace QMapLibre
