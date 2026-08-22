// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#include "map_quick_item.hpp"
#include "map_quick_item_p.hpp"

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

#include "style_change_p.hpp"

#include <QMapLibre/Map>

#include <QtCore/QTimer>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGRectangleNode>
#ifdef MLN_RENDER_BACKEND_OPENGL
#include <QtGui/QOpenGLContext>
#endif

#include <memory>

namespace {
constexpr int minSize{64};
constexpr int intervalTime{250};

constexpr double minZoomLevel{0.0};
constexpr double maxZoomLevel{20.0};
} // namespace

namespace QMapLibre {

MapQuickItem::MapQuickItem(QQuickItem *parent)
    : QQuickItem(parent),
      d_ptr(std::make_unique<MapQuickItemPrivate>(this)) {
    Q_D(MapQuickItem);

    setFlag(ItemHasContents, true);
    // TODO: make configurable
    d->m_settings.setCacheDatabasePath(QStringLiteral(":memory:"));
}

MapQuickItem::~MapQuickItem() = default;

QString MapQuickItem::style() const {
    Q_D(const MapQuickItem);

    return d->m_style;
}

void MapQuickItem::setStyle(const QString &style) {
    Q_D(MapQuickItem);

    if (d->m_style == style) {
        return;
    }
    d->m_style = style;
}

double MapQuickItem::zoomLevel() const {
    Q_D(const MapQuickItem);

    return d->m_zoomLevel;
}

bool MapQuickItem::styleLoaded() const {
    Q_D(const MapQuickItem);

    return d->m_styleLoaded;
}

void MapQuickItem::setZoomLevel(double zoomLevel) {
    Q_D(MapQuickItem);

    if (zoomLevel < minZoomLevel) {
        zoomLevel = minZoomLevel;
    } else if (zoomLevel > maxZoomLevel) {
        zoomLevel = maxZoomLevel;
    }

    if (d->m_zoomLevel == zoomLevel) {
        return;
    }

    d->m_zoomLevel = zoomLevel;

    if (d->m_map != nullptr) {
        d->m_syncState |= CameraOptionsSync;
        update();
    }

    emit zoomLevelChanged();
}

QVariantList MapQuickItem::coordinate() const {
    Q_D(const MapQuickItem);

    return d->m_coordinate;
}

void MapQuickItem::setCoordinate(const QVariantList &coordinate) {
    Q_D(MapQuickItem);

    if (d->m_coordinate == coordinate || coordinate.size() != 2) {
        return;
    }

    d->m_coordinate = coordinate;

    if (d->m_map != nullptr) {
        d->m_syncState |= CameraOptionsSync;
        update();
    }

    emit coordinateChanged();
}

void MapQuickItem::setCoordinateFromPixel(const QPointF &pixel) {
    Q_D(MapQuickItem);

    if (d->m_map == nullptr) {
        return;
    }

    const Coordinate coordinate = d->m_map->coordinateForPixel(pixel);
    setCoordinate({coordinate.first, coordinate.second});
}

void MapQuickItem::pan(const QPointF &offset) {
    Q_D(MapQuickItem);

    if (d->m_map == nullptr) {
        return;
    }

    d->m_map->moveBy(offset);
    const Coordinate coordinate = d->m_map->coordinate();
    d->m_coordinate = {coordinate.first, coordinate.second};
    update();
    emit coordinateChanged();
}

void MapQuickItem::scale(double scale, const QPointF &center) {
    Q_D(MapQuickItem);

    if (d->m_map == nullptr) {
        return;
    }

    d->m_map->scaleBy(scale, center);
    const Coordinate coordinate = d->m_map->coordinate();
    d->m_coordinate = {coordinate.first, coordinate.second};
    d->m_zoomLevel = d->m_map->zoom();
    update();
    emit coordinateChanged();
    emit zoomLevelChanged();
}

void MapQuickItem::easeTo(const QVariantMap &camera, const QVariantMap &animation) {
    Q_D(MapQuickItem);

    if (d->m_map == nullptr) {
        return;
    }

    CameraOptions cameraOptions;
    if (camera.contains("center")) {
        const auto center = camera["center"].toList();
        if (center.size() == 2) {
            cameraOptions.center = QVariant::fromValue(Coordinate{center[0].toDouble(), center[1].toDouble()});
        }
    }
    if (camera.contains("zoom")) {
        cameraOptions.zoom = camera["zoom"].toDouble();
    }
    if (camera.contains("bearing")) {
        cameraOptions.bearing = camera["bearing"].toDouble();
    }
    if (camera.contains("pitch")) {
        cameraOptions.pitch = camera["pitch"].toDouble();
    }

    AnimationOptions animationOptions;
    if (animation.contains("duration")) {
        animationOptions.duration = animation["duration"].toLongLong();
    }
    if (animation.contains("velocity")) {
        animationOptions.velocity = animation["velocity"].toDouble();
    }
    if (animation.contains("minZoom")) {
        animationOptions.minZoom = animation["minZoom"].toDouble();
    }

    d->m_map->easeTo(cameraOptions, animationOptions);

    const Coordinate coordinate = d->m_map->coordinate();
    d->m_coordinate = {coordinate.first, coordinate.second};
    d->m_zoomLevel = d->m_map->zoom();
    update();
    emit coordinateChanged();
    emit zoomLevelChanged();
}

void MapQuickItem::flyTo(const QVariantMap &camera, const QVariantMap &animation) {
    Q_D(MapQuickItem);

    if (d->m_map == nullptr) {
        return;
    }

    CameraOptions cameraOptions;
    if (camera.contains("center")) {
        const auto center = camera["center"].toList();
        if (center.size() == 2) {
            cameraOptions.center = QVariant::fromValue(Coordinate{center[0].toDouble(), center[1].toDouble()});
        }
    }
    if (camera.contains("zoom")) {
        cameraOptions.zoom = camera["zoom"].toDouble();
    }
    if (camera.contains("bearing")) {
        cameraOptions.bearing = camera["bearing"].toDouble();
    }
    if (camera.contains("pitch")) {
        cameraOptions.pitch = camera["pitch"].toDouble();
    }

    AnimationOptions animationOptions;
    if (animation.contains("duration")) {
        animationOptions.duration = animation["duration"].toLongLong();
    }
    if (animation.contains("velocity")) {
        animationOptions.velocity = animation["velocity"].toDouble();
    }
    if (animation.contains("minZoom")) {
        animationOptions.minZoom = animation["minZoom"].toDouble();
    }

    d->m_map->flyTo(cameraOptions, animationOptions);

    const Coordinate coordinate = d->m_map->coordinate();
    d->m_coordinate = {coordinate.first, coordinate.second};
    d->m_zoomLevel = d->m_map->zoom();
    update();
    emit coordinateChanged();
    emit zoomLevelChanged();
}

void MapQuickItem::addStyleParameter(StyleParameter *parameter) {
    Q_D(MapQuickItem);
    d->addStyleParameter(parameter);
}

void MapQuickItem::removeStyleParameter(StyleParameter *parameter) {
    Q_D(MapQuickItem);
    d->removeStyleParameter(parameter);
}

void MapQuickItem::clearStyleParameters() {
    Q_D(MapQuickItem);
    d->clearStyleParameters();
}

void MapQuickItem::componentComplete() {
    QQuickItem::componentComplete();

    QTimer::singleShot(intervalTime, this, &MapQuickItem::initialize);
}

void MapQuickItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) {
    Q_D(MapQuickItem);

    QQuickItem::geometryChange(newGeometry, oldGeometry);

    if (d->m_map == nullptr) {
        return;
    }

    if (newGeometry.size() != oldGeometry.size()) {
        const QSize viewportSize{static_cast<int>(newGeometry.width()), static_cast<int>(newGeometry.height())};
        const qreal pixelRatio = window() != nullptr ? window()->devicePixelRatio() : 1.0;
        d->m_map->resize(viewportSize.expandedTo({minSize, minSize}), pixelRatio);
        d->m_syncState |= ViewportSync;
        update();
    }
}

QSGNode *MapQuickItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data) {
    Q_D(MapQuickItem);
    Q_UNUSED(data);

    if (!d->m_map) {
        delete oldNode; // NOLINT(cppcoreguidelines-owning-memory)
        return nullptr;
    }

    auto *root = static_cast<QSGRectangleNode *>(oldNode);
    if (root == nullptr) {
        root = window()->createRectangleNode();
    }

    root->setRect(boundingRect());
    // TODO: background color
    // root->setColor(m_color);

    QSGNode *content = root->childCount() > 0 ? root->firstChild() : nullptr;
    content = updateMapNode(content);
    if (content != nullptr && root->childCount() == 0) {
        root->appendChildNode(content);
    }

    return root;
}

QSGNode *MapQuickItem::updateMapNode(QSGNode *node) {
    Q_D(MapQuickItem);

    const QSize viewportSize{static_cast<int>(width()), static_cast<int>(height())};

    if (node == nullptr) {
#ifdef MLN_RENDERER_DEBUGGING
        qDebug() << "MapQuickItem::updatePaintNode() - Creating new node for size" << viewportSize;
#endif

#ifdef MLN_RENDER_BACKEND_OPENGL
        // OpenGL context check
        const QOpenGLContext *glContext = QOpenGLContext::currentContext();
        if (glContext == nullptr) {
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
            d->m_map, viewportSize, window()->devicePixelRatio());
#elif defined(MLN_RENDER_BACKEND_METAL)
        std::unique_ptr<TextureNodeBase> mbglNode = std::make_unique<TextureNodeMetal>(
            d->m_map, viewportSize, window()->devicePixelRatio());
#elif defined(MLN_RENDER_BACKEND_VULKAN)
        std::unique_ptr<TextureNodeBase> mbglNode = std::make_unique<TextureNodeVulkan>(
            d->m_map, viewportSize, window()->devicePixelRatio());
#endif
        QObject::connect(d->m_map.get(), &Map::needsRendering, this, &QQuickItem::update);
        QObject::connect(d->m_map.get(), &Map::mapChanged, this, &MapQuickItem::onMapChanged);

        d->m_syncState = ViewportSync | CameraOptionsSync;

        node = mbglNode.release();

#ifdef MLN_RENDERER_DEBUGGING
        qDebug() << "MapQuickItem::updatePaintNode() - Created new node" << node;
#endif
    }

    if ((d->m_syncState & CameraOptionsSync) != 0) {
        d->m_map->setCoordinateZoom({d->m_coordinate.size() == 2 ? d->m_coordinate[0].toDouble() : 0.0,
                                     d->m_coordinate.size() == 2 ? d->m_coordinate[1].toDouble() : 0.0},
                                    d->m_zoomLevel);
    }

    if ((d->m_syncState & ViewportSync) != 0) {
        static_cast<TextureNodeBase *>(node)->resize(viewportSize, window()->devicePixelRatio(), window());
    }

    if (d->m_styleLoaded) {
        d->syncStyleChanges();
    }

    static_cast<TextureNodeBase *>(node)->render(window());

    d->m_syncState = NoSync;

    return node;
}

void MapQuickItem::initialize() {
    Q_D(MapQuickItem);

    d->initialize();
}

void MapQuickItem::onMapChanged(Map::MapChange change) {
    Q_D(MapQuickItem);

    if (change == Map::MapChangeDidFinishLoadingStyle || change == Map::MapChangeDidFailLoadingMap) {
        if (!d->m_styleLoaded) {
            d->m_styleLoaded = true;
            emit styleLoadedChanged();
        }
    } else if (change == Map::MapChangeWillStartLoadingMap) {
        if (d->m_styleLoaded) {
            d->m_styleLoaded = false;
            emit styleLoadedChanged();
        }
        d->m_styleChanges.clear();

        for (const StyleParameter *parameter : d->m_mapParameters) {
            std::vector<std::unique_ptr<StyleChange>> changes = StyleChange::addParameter(parameter,
                                                                                          d->m_mapItemsBefore);
            std::ranges::move(changes, std::back_inserter(d->m_styleChanges));
        }
    } else if (change == Map::MapChangeDidFinishLoadingMap) {
        // TODO: make it more elegant
        QTimer::singleShot(intervalTime, this, &QQuickItem::update);
    }
}

void MapQuickItem::onStyleParameterUpdated(StyleParameter *parameter) {
    Q_D(MapQuickItem);

    std::vector<std::unique_ptr<StyleChange>> changes = StyleChange::addParameter(parameter, d->m_mapItemsBefore);
    std::ranges::move(changes, std::back_inserter(d->m_styleChanges));

    update();
}

// private implementation

MapQuickItemPrivate::MapQuickItemPrivate(MapQuickItem *q)
    : q_ptr(q) {}

void MapQuickItemPrivate::initialize() {
    Q_Q(MapQuickItem);

    if (m_map != nullptr) {
        return;
    }

    const QSize viewportSize{static_cast<int>(q->width()), static_cast<int>(q->height())};
    const qreal pixelRatio = q->window() != nullptr ? q->window()->devicePixelRatio() : 1.0;
    m_map = std::make_unique<Map>(nullptr, m_settings, viewportSize, pixelRatio);
    m_map->setConnectionEstablished();

    // Set default style
    if (!m_style.isEmpty()) {
        m_map->setStyleUrl(m_style);
    } else if (!m_settings.styles().empty()) {
        m_map->setStyleUrl(m_settings.styles().front().url);
    } else if (!m_settings.providerStyles().empty()) {
        m_map->setStyleUrl(m_settings.providerStyles().front().url);
    }

    for (const StyleParameter *parameter : m_mapParameters) {
        std::vector<std::unique_ptr<StyleChange>> changes = StyleChange::addParameter(parameter, m_mapItemsBefore);
        std::ranges::move(changes, std::back_inserter(m_styleChanges));
    }

    q->update();
}

void MapQuickItemPrivate::addStyleParameter(StyleParameter *parameter) {
    Q_Q(MapQuickItem);

    if (m_mapParameters.contains(parameter)) {
        return;
    }

    m_mapParameters << parameter;

    QObject::connect(parameter, &StyleParameter::updated, q, &MapQuickItem::onStyleParameterUpdated);

    if (m_styleLoaded) {
        std::vector<std::unique_ptr<StyleChange>> changes = StyleChange::addParameter(parameter, m_mapItemsBefore);
        std::ranges::move(changes, std::back_inserter(m_styleChanges));
        q->update();
    }
}

void MapQuickItemPrivate::removeStyleParameter(StyleParameter *parameter) {
    Q_Q(MapQuickItem);

    q->disconnect(parameter);

    if (m_styleLoaded) {
        std::vector<std::unique_ptr<StyleChange>> changes = StyleChange::removeParameter(parameter);
        std::ranges::move(changes, std::back_inserter(m_styleChanges));
        q->update();
    }

    m_mapParameters.removeOne(parameter);
}

void MapQuickItemPrivate::clearStyleParameters() {
    for (StyleParameter *parameter : m_mapParameters) {
        removeStyleParameter(parameter);
    }
}

void MapQuickItemPrivate::syncStyleChanges() {
    for (const auto &change : m_styleChanges) {
        if (change->isValid()) {
            change->apply(m_map.get());
        }
    }

    m_styleChanges.clear();
}

} // namespace QMapLibre
