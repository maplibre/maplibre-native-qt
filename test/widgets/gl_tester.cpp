// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#include "gl_tester.hpp"

#include <QDebug>

namespace {
constexpr int kAnimationDuration = 5000;
}

namespace QMapLibre {

GLTester::GLTester(const QMapLibre::Settings &settings)
    : GLWidget(settings) {}

void GLTester::initializeAnimation() {
    m_bearingAnimation = new QPropertyAnimation(map(), "bearing");
    m_zoomAnimation = new QPropertyAnimation(map(), "zoom");

    connect(m_zoomAnimation, &QPropertyAnimation::finished, this, &GLTester::animationFinished);
    connect(m_zoomAnimation, &QPropertyAnimation::valueChanged, this, &GLTester::animationValueChanged);
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

void GLTester::animationFinished() {
    qDebug() << "Animation ticks/s: " << m_animationTicks / static_cast<float>(kAnimationDuration) * 1000.;
    qDebug() << "Frame draws/s: " << m_frameDraws / static_cast<float>(kAnimationDuration) * 1000.;
}

void GLTester::paintGL() {
    m_frameDraws++;
    GLWidget::paintGL();
}

} // namespace QMapLibre
