#include "map_quick_item.hpp"
#include <QtCore/qlogging.h>

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

#include <QQuickWindow>
#include <QSGSimpleTextureNode>

namespace QMapLibre {

MapQuickItem::MapQuickItem(QQuickItem *parent)
    : QQuickItem(parent) {
    setFlag(ItemHasContents, true);

    QMapLibre::Styles styles;
    styles.append(QMapLibre::Style("https://demotiles.maplibre.org/style.json", "Demo tiles"));
    m_settings.setStyles(styles);
    m_settings.setDefaultZoom(5);
    m_settings.setDefaultCoordinate(QMapLibre::Coordinate(43, 21));
    m_settings.setCacheDatabasePath(QStringLiteral(":memory:"));
}

void MapQuickItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) {
    QQuickItem::geometryChange(newGeometry, oldGeometry);

    qDebug() << "MapQuickItem::geometryChange" << newGeometry.width() << "x" << newGeometry.height();

    if (newGeometry.size() != oldGeometry.size()) {
        if (m_map != nullptr) {
            m_map->resize({static_cast<int>(newGeometry.width()), static_cast<int>(newGeometry.height())},
                          window()->devicePixelRatio());
            m_initialized = true;
        }
    }
}

QSGNode *MapQuickItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data) {
    Q_UNUSED(data);

    qDebug() << "MapQuickItem::updatePaintNode()" << width() << "x" << height()
             << "devicePixelRatio:" << window()->devicePixelRatio() << "init:" << m_initialized;

    auto *node = static_cast<TextureNodeBase *>(oldNode);
    if (node == nullptr) {
        const QSize viewportSize{static_cast<int>(width()), static_cast<int>(height())};
        qDebug() << "MapQuickItem::updatePaintNode() - creating new node for size" << viewportSize;
#if defined(MLN_RENDER_BACKEND_OPENGL)
        // OpenGL context check
        QOpenGLContext *currentCtx = QOpenGLContext::currentContext();
        if (currentCtx == nullptr) {
            qWarning("QOpenGLContext is NULL!");
            qWarning() << "You are running on QSG backend " << QSGContext::backend();
            qWarning("The MapLibre plugin works with both Desktop and ES 2.0+ OpenGL versions.");
            qWarning("Verify that your Qt is built with OpenGL, and what kind of OpenGL.");
            qWarning(
                "To force using a specific OpenGL version, check QSurfaceFormat::setRenderableType and "
                "QSurfaceFormat::setDefaultFormat");

            return node;
        }

        std::unique_ptr<TextureNodeBase> mbglNode = std::make_unique<TextureNodeOpenGL>(
            m_settings, viewportSize, window()->devicePixelRatio());
#elif defined(MLN_RENDER_BACKEND_METAL)
        std::unique_ptr<TextureNodeBase> mbglNode = std::make_unique<TextureNodeMetal>(
            m_settings, viewportSize, window()->devicePixelRatio());
#elif defined(MLN_RENDER_BACKEND_VULKAN)
        std::unique_ptr<TextureNodeBase> mbglNode = std::make_unique<TextureNodeVulkan>(
            m_settings, viewportSize, window()->devicePixelRatio());
#endif
        QObject::connect(mbglNode->map(), &Map::needsRendering, this, &QQuickItem::update);
        QObject::connect(mbglNode->map(), &Map::needsRendering, [] { qDebug() << "Map needs rendering"; });
        mbglNode->map()->setConnectionEstablished();

        if (m_map == nullptr) {
            m_map = mbglNode->map();
        }

        node = mbglNode.release();

        qDebug() << "MapQuickItem::updatePaintNode() - created new node" << node;
    }

    // Set default style
    if (!m_settings.styles().empty()) {
        m_map->setStyleUrl(m_settings.styles().front().url);
        qDebug() << "Setting style URL:" << m_settings.styles().front().url;
    } else if (!m_settings.providerStyles().empty()) {
        m_map->setStyleUrl(m_settings.providerStyles().front().url);
        qDebug() << "Setting provider style URL:" << m_settings.providerStyles().front().url;
    }

    assert(node != nullptr);

    node->render(window());

    return node;
}

} // namespace QMapLibre
