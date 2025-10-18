// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#include "map_widget_tester.hpp"
#include "test_main_window.hpp"
#include "test_window.hpp"

#include <QDebug>
#include <QTest>

#include <memory>

class TestWidgets : public QObject {
    Q_OBJECT

private slots:
    void testGLWidgetNoProvider();
    void testGLWidgetMapLibreProvider();
    void testGLWidgetDocking();
    void testGLWidgetStyle();
};

void TestWidgets::testGLWidgetNoProvider() {
    const QMapLibre::Settings settings;
    auto widget = std::make_unique<QMapLibre::MapWidget>(settings);
    widget->show();
    QTest::qWait(1000);
}

void TestWidgets::testGLWidgetMapLibreProvider() {
    QMapLibre::Settings settings(QMapLibre::Settings::MapLibreProvider);
    settings.setDefaultCoordinate(QMapLibre::Coordinate(59.91, 10.75));
    settings.setDefaultZoom(4);
    auto tester = std::make_unique<QMapLibre::Test::MapWidgetTester>(settings);
    tester->show();
    QTest::qWait(1000);
    qDebug() << "Resizing to" << QSize(800, 600);
    tester->resize(800, 600);
    QTest::qWait(1000);
    qDebug() << "Resizing to" << QSize(400, 300);
    tester->resize(400, 300);
    QTest::qWait(1000);
    qDebug() << "Hiding";
    tester->hide();
    QTest::qWait(500);
    qDebug() << "Showing";
    tester->show();
    QTest::qWait(1000);
}

void TestWidgets::testGLWidgetDocking() {
    QMapLibre::Settings settings(QMapLibre::Settings::MapLibreProvider);
    settings.setDefaultCoordinate(QMapLibre::Coordinate(59.91, 10.75));
    settings.setDefaultZoom(4);
    auto tester = std::make_unique<QMapLibre::Test::MainWindow>();
    tester->show();
    QTest::qWait(1000);
    qDebug() << "Undocking";
    QMapLibre::Test::Window *window = tester->currentCentralWidget();
    window->dockUndock();
    QTest::qWait(500);
    qDebug() << "Resizing undocked window to" << QSize(800, 600);
    window->resize(800, 600);
    QTest::qWait(500);
    qDebug() << "Docking back";
    window->dockUndock();
    QTest::qWait(1000);
    qDebug() << "Undock again";
    window->dockUndock();
    qDebug() << "Adding new window to main window";
    tester->addNewWindow();
    QTest::qWait(1000);
    window->close();
}

void TestWidgets::testGLWidgetStyle() {
    QMapLibre::Styles styles;
    styles.append(QMapLibre::Style("https://demotiles.maplibre.org/style.json", "Demo tiles"));

    QMapLibre::Settings settings;
    settings.setStyles(styles);
    auto tester = std::make_unique<QMapLibre::Test::MapWidgetTester>(settings);
    tester->show();
    QTest::qWait(100);
    tester->initializeAnimation();
    QTest::qWait(tester->selfTest());
}

// NOLINTNEXTLINE(misc-const-correctness)
QTEST_MAIN(TestWidgets)
#include "test_widgets.moc"
