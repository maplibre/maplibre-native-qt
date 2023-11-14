// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#include "map_tester.hpp"
#include "test_rendering.hpp"

#include <QOpenGLContext>
#include <QTest>

class TestCore : public QObject {
    Q_OBJECT

private slots:
    void testStyleJSON();
    void testStyleURL();
};

void TestCore::testStyleJSON() {
    const std::unique_ptr<QSurface> surface = QMapLibre::createTestSurface(QSurface::Window);
    QVERIFY(surface != nullptr);

    QOpenGLContext ctx;
    QVERIFY(ctx.create());
    QVERIFY(ctx.makeCurrent(surface.get()));

    auto tester = std::make_unique<QMapLibre::MapTester>();

    const QString fixturesPath = qgetenv("MLN_FIXTURES_PATH");
    QVERIFY(!fixturesPath.isEmpty());

    QFile f(fixturesPath + "/resources/style_vector.json");
    QVERIFY(f.open(QFile::ReadOnly | QFile::Text));

    QTextStream in(&f);
    const QString json = in.readAll();

    tester->map.setStyleJson(json);
    QCOMPARE(tester->map.styleJson(), json);

    QSKIP("Skip until we can load the style");

    tester->runUntil(QMapLibre::Map::MapChangeDidFinishLoadingMap);

    tester->map.setStyleJson("invalid json");
    tester->runUntil(QMapLibre::Map::MapChangeDidFailLoadingMap);

    tester->map.setStyleJson("\"\"");
    tester->runUntil(QMapLibre::Map::MapChangeDidFailLoadingMap);

    tester->map.setStyleJson(QString());
    tester->runUntil(QMapLibre::Map::MapChangeDidFailLoadingMap);
}

void TestCore::testStyleURL() {
    const std::unique_ptr<QSurface> surface = QMapLibre::createTestSurface(QSurface::Window);
    QVERIFY(surface != nullptr);

    QOpenGLContext ctx;
    QVERIFY(ctx.create());
    QVERIFY(ctx.makeCurrent(surface.get()));

    auto tester = std::make_unique<QMapLibre::MapTester>();

    const QString url(tester->settings.providerStyles().front().url);

    tester->map.setStyleUrl(url);
    QCOMPARE(tester->map.styleUrl(), url);
    tester->runUntil(QMapLibre::Map::MapChangeDidFinishLoadingMap);

    tester->map.setStyleUrl("invalid://url");
    tester->runUntil(QMapLibre::Map::MapChangeDidFailLoadingMap);

    tester->map.setStyleUrl(QString());
    tester->runUntil(QMapLibre::Map::MapChangeDidFailLoadingMap);
}

// NOLINTNEXTLINE(misc-const-correctness)
QTEST_MAIN(TestCore)
#include "test_core.moc"
