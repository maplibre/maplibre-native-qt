#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickView>
#include <QQmlContext>
#include <QtPositioning/QGeoCoordinate>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    
    // Set organization and application name
    app.setOrganizationName("MapLibre");
    app.setApplicationName("MapLibre Quick iOS");
    
    QQmlApplicationEngine engine;
    
    // Add import path for MapLibre Quick plugin
    engine.addImportPath("/Users/admin/repos/maplibre-native-qt/qmaplibre-build-macos");
    engine.addImportPath("qrc:/");
    
    // Load the QML file
    const QUrl url(QStringLiteral("qrc:/simple.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    
    engine.load(url);
    
    return app.exec();
}