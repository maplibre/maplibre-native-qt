// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#include "gl_tester.hpp"

#include <QtCore/QDebug>

namespace {
constexpr int kAnimationDuration = 5000;
} // anonymous namespace

namespace QMapLibre {

struct IdentifierToQStringVisitor {
    QString operator()(const std::string& value) const {
        return QString::fromStdString(value);
    }
    QString operator()(uint64_t value) const {
        return QString::number(value);
    }
    QString operator()(int64_t value) const {
        return QString::number(value);
    }
    QString operator()(double value) const {
        return QString::number(value);
    }
    QString operator()(mapbox::feature::null_value_t) const {
        return QStringLiteral("null");
    }
};

QString identifierToQString(const mapbox::feature::identifier& id) {
    return mapbox::util::apply_visitor(IdentifierToQStringVisitor(), id);
}

GLTester::GLTester(const QMapLibre::Settings &settings)
    : GLWidget(settings) {}

QString valueToQString(const mapbox::feature::value& val) {
    if (val.is<std::string>()) {
        return QString::fromStdString(val.get<std::string>());
    } else if (val.is<bool>()) {
        return val.get<bool>() ? "true" : "false";
    } else if (val.is<double>()) {
        return QString::number(val.get<double>());
    } else if (val.is<uint64_t>()) {
        return QString::number(val.get<uint64_t>());
    } else if (val.is<int64_t>()) {
        return QString::number(val.get<int64_t>());
    } else if (val.is<mapbox::feature::null_value_t>()) {
        return "(null)";
    } else {
        return "(unsupported)";
    }
}

void GLTester::initializeQuery() {
    connect(this, &GLTester::onMousePressEvent, [this](Coordinate coordinate) {
        qDebug() << "onMousePressEvent" << coordinate;

        /* This feels stupid, need to change the interface. */
        QPointF pixel = map()->pixelForCoordinate(coordinate);
        mbgl::ScreenCoordinate screenPoint(pixel.x(), pixel.y());

        mbgl::RenderedQueryOptions options;
        options.layerIDs = {"countries-fill"};

        std::vector<mbgl::Feature> features = map()->queryRenderedFeatures(screenPoint, options);

        for (const auto& feature : features) {
            qDebug() << "Feature ID:" << identifierToQString(feature.id);

            for (const auto& [key, value] : feature.properties) {
                QString keyStr = QString::fromStdString(key);
                 QString valueStr = valueToQString(value);
                qDebug() << keyStr << ":" << valueStr;
            }
        }
    });
}

void GLTester::initializeAnimation() {
    m_bearingAnimation = std::make_unique<QPropertyAnimation>(map(), "bearing");
    m_zoomAnimation = std::make_unique<QPropertyAnimation>(map(), "zoom");

    connect(m_zoomAnimation.get(), &QPropertyAnimation::finished, this, &GLTester::animationFinished);
    connect(m_zoomAnimation.get(), &QPropertyAnimation::valueChanged, this, &GLTester::animationValueChanged);

    connect(this, &GLTester::onMousePressEvent, [](Coordinate coordinate) {
        qDebug() << "onMousePressEvent" << coordinate;
    });
    connect(this, &GLTester::onMouseReleaseEvent, [](Coordinate coordinate) {
        qDebug() << "onMouseReleaseEvent" << coordinate;
    });
    connect(this, &GLTester::onMouseDoubleClickEvent, [](Coordinate coordinate) {
        qDebug() << "onMouseDoubleClickEvent" << coordinate;
    });
    connect(
        this, &GLTester::onMouseMoveEvent, [](Coordinate coordinate) { qDebug() << "onMouseMoveEvent" << coordinate; });
}

int GLTester::selfTest() {
    if (m_bearingAnimation) {
        m_bearingAnimation->setDuration(kAnimationDuration);
        m_bearingAnimation->setEndValue(map()->bearing() + (360 * 4));
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
