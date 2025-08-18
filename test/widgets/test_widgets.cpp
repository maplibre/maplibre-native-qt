// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#include "gl_tester.hpp"
#include "test_rendering.hpp"

#include <QOpenGLContext>
#include <QTest>

#include <memory>

class TestWidgets : public QObject {
    Q_OBJECT

private slots:
    void testGLWidgetNoProvider();
    void testGLWidgetMapLibreProvider();
    void testGLWidgetStyle();
    void testGLWidgetQuery();
};

void TestWidgets::testGLWidgetNoProvider() {
    const std::unique_ptr<QSurface> surface = QMapLibre::createTestSurface(QSurface::Window);
    QVERIFY(surface != nullptr);

    QOpenGLContext ctx;
    QVERIFY(ctx.create());
    QVERIFY(ctx.makeCurrent(surface.get()));

    const QMapLibre::Settings settings;
    auto tester = std::make_unique<QMapLibre::GLTester>(settings);
    tester->show();
    QTest::qWait(1000);
}

void TestWidgets::testGLWidgetMapLibreProvider() {
    const std::unique_ptr<QSurface> surface = QMapLibre::createTestSurface(QSurface::Window);
    QVERIFY(surface != nullptr);

    QOpenGLContext ctx;
    QVERIFY(ctx.create());
    QVERIFY(ctx.makeCurrent(surface.get()));

    const QMapLibre::Settings settings(QMapLibre::Settings::MapLibreProvider);
    auto tester = std::make_unique<QMapLibre::GLTester>(settings);
    tester->show();
    QTest::qWait(100);
    tester->initializeAnimation();
    QTest::qWait(tester->selfTest());
}

void TestWidgets::testGLWidgetStyle() {
    const std::unique_ptr<QSurface> surface = QMapLibre::createTestSurface(QSurface::Window);
    QVERIFY(surface != nullptr);

    QOpenGLContext ctx;
    QVERIFY(ctx.create());
    QVERIFY(ctx.makeCurrent(surface.get()));

    QMapLibre::Styles styles;
    styles.append(QMapLibre::Style("https://demotiles.maplibre.org/style.json", "Demo tiles"));

    QMapLibre::Settings settings;
    settings.setStyles(styles);
    auto tester = std::make_unique<QMapLibre::GLTester>(settings);
    tester->show();
    QTest::qWait(100);
    tester->initializeAnimation();
    QTest::qWait(tester->selfTest());
}

void TestWidgets::testGLWidgetQuery() {
    const std::unique_ptr<QSurface> surface = QMapLibre::createTestSurface(QSurface::Window);
    QVERIFY(surface != nullptr);

    QOpenGLContext ctx;
    QVERIFY(ctx.create());
    QVERIFY(ctx.makeCurrent(surface.get()));

    QMapLibre::Styles styles;
    styles.append(QMapLibre::Style("https://demotiles.maplibre.org/style.json", "Demo tiles"));

    QMapLibre::Settings settings;
    settings.setStyles(styles);
    auto tester = std::make_unique<QMapLibre::GLTester>(settings);
    tester->show();
    QTest::qWait(100);
    tester->initializeQuery();
    QTest::qWait(10000);
}

// NOLINTNEXTLINE(misc-const-correctness)
QTEST_MAIN(TestWidgets)
#include "test_widgets.moc"
