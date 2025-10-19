// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#include "map_widget_tester.hpp"

#include <QMapLibre/Map>

#include <QtCore/QDebug>

namespace {
constexpr int MinimumWidth = 640;
constexpr int MinimumHeight = 480;
constexpr int AnimationDuration = 5000;
} // namespace

namespace QMapLibre::Test {

MapWidgetTester::MapWidgetTester(const QMapLibre::Settings &settings)
    : MapWidget(settings) {
    setMinimumSize(MinimumWidth, MinimumHeight);
}

void MapWidgetTester::initializeAnimation() {
    m_bearingAnimation = std::make_unique<QPropertyAnimation>(map(), "bearing");
    m_zoomAnimation = std::make_unique<QPropertyAnimation>(map(), "zoom");

    connect(m_zoomAnimation.get(), &QPropertyAnimation::finished, this, &MapWidgetTester::animationFinished);
    connect(m_zoomAnimation.get(), &QPropertyAnimation::valueChanged, this, &MapWidgetTester::animationValueChanged);

    connect(this, &MapWidgetTester::onMousePressEvent, [](Coordinate coordinate) {
        qDebug() << "onMousePressEvent" << coordinate;
    });
    connect(this, &MapWidgetTester::onMouseReleaseEvent, [](Coordinate coordinate) {
        qDebug() << "onMouseReleaseEvent" << coordinate;
    });
    connect(this, &MapWidgetTester::onMouseDoubleClickEvent, [](Coordinate coordinate) {
        qDebug() << "onMouseDoubleClickEvent" << coordinate;
    });
    connect(this, &MapWidgetTester::onMouseMoveEvent, [](Coordinate coordinate) {
        qDebug() << "onMouseMoveEvent" << coordinate;
    });
}

int MapWidgetTester::selfTest() {
    if (m_bearingAnimation) {
        m_bearingAnimation->setDuration(AnimationDuration);
        m_bearingAnimation->setEndValue(map()->bearing() + (360 * 4));
        m_bearingAnimation->start();
    }

    if (m_zoomAnimation) {
        m_zoomAnimation->setDuration(AnimationDuration);
        m_zoomAnimation->setEndValue(map()->zoom() + 3);
        m_zoomAnimation->start();
    }

    return AnimationDuration;
}

void MapWidgetTester::animationValueChanged() {
    m_animationTicks++;
}

void MapWidgetTester::animationFinished() const {
    qDebug() << "Animation ticks/s: "
             << static_cast<float>(m_animationTicks) / static_cast<float>(AnimationDuration) * 1000.f;
    qDebug() << "Frame draws/s: " << static_cast<float>(m_frameDraws) / static_cast<float>(AnimationDuration) * 1000.f;
}

// void MapWidgetTester::paintGL() {
//     m_frameDraws++;
//     GLWidget::paintGL();
// }

} // namespace QMapLibre::Test
