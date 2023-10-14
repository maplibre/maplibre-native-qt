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
    void testGLWidget();
};

void TestWidgets::testGLWidget() {
    std::unique_ptr<QSurface> surface = QMapLibre::createTestSurface(QSurface::Window);
    QVERIFY(surface != nullptr);

    QOpenGLContext ctx;
    QVERIFY(ctx.create());
    QVERIFY(ctx.makeCurrent(surface.get()));

    QMapLibre::Settings settings;
    auto tester = std::make_unique<QMapLibre::GLTester>(settings);
    tester->show();
    QTest::qWait(1000);
    tester->initializeAnimation();
    QTest::qWait(tester->selfTest());
}

QTEST_MAIN(TestWidgets)
#include "test_widgets.moc"
