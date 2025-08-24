#include "map_quick_item.hpp"

#include "texture_node_base_p.hpp"
#ifdef MLN_RENDER_BACKEND_OPENGL
#include "texture_node_opengl_p.hpp"
#endif
#ifdef MLN_RENDER_BACKEND_METAL
#include "texture_node_metal_p.hpp"
#endif
#ifdef MLN_RENDER_BACKEND_VULKAN
#include "texture_node_vulkan_p.hpp"
#endif

#include <QMapLibre/Map>

#include <QtCore/QTimer>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGRectangleNode>
#ifdef MLN_RENDER_BACKEND_OPENGL
#include <QtGui/QOpenGLContext>
#endif

#include <memory>

namespace {
constexpr int intervalTime{250};
} // namespace

namespace QMapLibre {

MapQuickItem::MapQuickItem(QQuickItem *parent)
    : QQuickItem(parent) {
    setFlag(ItemHasContents, true);

    QMapLibre::Styles styles;
    styles.append(QMapLibre::Style("https://demotiles.maplibre.org/style.json", "Demo tiles"));
    m_settings.setStyles(styles);
    m_settings.setDefaultZoom(4);
    m_settings.setDefaultCoordinate(QMapLibre::Coordinate(59.91, 10.75));
    m_settings.setCacheDatabasePath(QStringLiteral(":memory:"));
}

void MapQuickItem::initialize() {
    if (m_map != nullptr) {
        return;
    }

    qDebug() << "MapQuickItem::initialize()";

    const QSize viewportSize{static_cast<int>(width()), static_cast<int>(height())};
    const qreal pixelRatio = window() != nullptr ? window()->devicePixelRatio() : 1.0;
    m_map = std::make_unique<Map>(nullptr, m_settings, viewportSize, pixelRatio);
    m_map->setConnectionEstablished();

    // Set default style
    if (!m_settings.styles().empty()) {
        qDebug() << "Setting style URL:" << m_settings.styles().front().url;
        m_map->setStyleUrl(m_settings.styles().front().url);
    } else if (!m_settings.providerStyles().empty()) {
        qDebug() << "Setting provider style URL:" << m_settings.providerStyles().front().url;
        m_map->setStyleUrl(m_settings.providerStyles().front().url);
    }

    update();
}

void MapQuickItem::componentComplete() {
    QQuickItem::componentComplete();

    qDebug() << "MapQuickItem::componentComplete()";

    QTimer::singleShot(intervalTime, this, &MapQuickItem::initialize);
}

void MapQuickItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) {
    QQuickItem::geometryChange(newGeometry, oldGeometry);

    if (m_map == nullptr) {
        return;
    }

    const qreal pixelRatio = window() != nullptr ? window()->devicePixelRatio() : 1.0;

    qDebug() << "MapQuickItem::geometryChange" << newGeometry.width() << "x" << newGeometry.height()
             << "pixelRatio:" << pixelRatio;

    if (newGeometry.size() != oldGeometry.size()) {
        const QSize viewportSize{static_cast<int>(newGeometry.width()), static_cast<int>(newGeometry.height())};
        m_map->resize(viewportSize.expandedTo({64, 64}), pixelRatio);
        update();
    }
}

QSGNode *MapQuickItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data) {
    Q_UNUSED(data);

    if (!m_map) {
        delete oldNode;
        return nullptr;
    }

    auto *root = static_cast<QSGRectangleNode *>(oldNode);
    if (root == nullptr) {
        root = window()->createRectangleNode();
    }

    root->setRect(boundingRect());
    // root->setColor(m_color);

    QSGNode *content = root->childCount() > 0 ? root->firstChild() : nullptr;
    content = updateMapNode(content);
    if (content != nullptr && root->childCount() == 0) {
        root->appendChildNode(content);
    }

    return root;
}

QSGNode *MapQuickItem::updateMapNode(QSGNode *node) {
    const QSize viewportSize{static_cast<int>(width()), static_cast<int>(height())};

    if (node == nullptr) {
        qDebug() << "MapQuickItem::updatePaintNode() - creating new node for size" << viewportSize;
#if defined(MLN_RENDER_BACKEND_OPENGL)
        // OpenGL context check
        QOpenGLContext *currentCtx = QOpenGLContext::currentContext();
        if (currentCtx == nullptr) {
            qWarning("QOpenGLContext is NULL!");
            // qWarning() << "You are running on QSG backend " << QSGContext::backend();
            qWarning("The MapLibre plugin works with both Desktop and ES 2.0+ OpenGL versions.");
            qWarning("Verify that your Qt is built with OpenGL, and what kind of OpenGL.");
            qWarning(
                "To force using a specific OpenGL version, check QSurfaceFormat::setRenderableType and "
                "QSurfaceFormat::setDefaultFormat");

            return node;
        }

        std::unique_ptr<TextureNodeBase> mbglNode = std::make_unique<TextureNodeOpenGL>(
            m_map, viewportSize, window()->devicePixelRatio());
#elif defined(MLN_RENDER_BACKEND_METAL)
        std::unique_ptr<TextureNodeBase> mbglNode = std::make_unique<TextureNodeMetal>(
            m_map, viewportSize, window()->devicePixelRatio());
#elif defined(MLN_RENDER_BACKEND_VULKAN)
        std::unique_ptr<TextureNodeBase> mbglNode = std::make_unique<TextureNodeVulkan>(
            m_map, viewportSize, window()->devicePixelRatio());
#endif
        QObject::connect(m_map.get(), &Map::needsRendering, this, &QQuickItem::update);
        QObject::connect(m_map.get(), &Map::needsRendering, [] { qDebug() << "Map needs rendering"; });

        QObject::connect(m_map.get(), &Map::mapChanged, this, &MapQuickItem::onMapChanged);

        node = mbglNode.release();

        qDebug() << "MapQuickItem::updatePaintNode() - created new node" << node;

        m_map->setZoom(m_settings.defaultZoom());
        m_map->setCoordinate(m_settings.defaultCoordinate());

        static_cast<TextureNodeBase *>(node)->resize(viewportSize, window()->devicePixelRatio(), window());
    }

    static_cast<TextureNodeBase *>(node)->resize(viewportSize, window()->devicePixelRatio(), window());

    static_cast<TextureNodeBase *>(node)->render(window());

    return node;
}

void MapQuickItem::onMapChanged(Map::MapChange change) {
    if (change == Map::MapChangeDidFinishLoadingStyle || change == Map::MapChangeDidFailLoadingMap) {
        qDebug() << "MapQuickItem::onMapChanged() - style loaded";
    } else if (change == Map::MapChangeWillStartLoadingMap) {
        qDebug() << "MapQuickItem::onMapChanged() - style loading started";
    } else if (change == Map::MapChangeDidFinishLoadingMap) {
        // TODO: make it more elegant
        QTimer::singleShot(intervalTime, this, &QQuickItem::update);
        qDebug() << "MapLibre map loaded";
    }
}

void MapQuickItem::wheelEvent(QWheelEvent *event) {
    if (m_map == nullptr) {
        QQuickItem::wheelEvent(event);
        return;
    }
    qreal angle = event->angleDelta().y();
    if (angle == 0) {
        QQuickItem::wheelEvent(event);
        return;
    }
    double factor = angle > 0 ? 1.05 : 0.95;
    m_map->scaleBy(factor, event->position());
    update();
    event->accept();
}

} // namespace QMapLibre
