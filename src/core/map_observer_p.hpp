// Copyright (C) 2023 MapLibre contributors
// Copyright (C) 2019 Mapbox, Inc.

// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include "map.hpp"

#include <mln/map/map_observer.hpp>
#include <mln/style/style.hpp>

#include <QtCore/QObject>

#include <exception>
#include <memory>

namespace QMapLibre {

class MapPrivate;

class MapObserver : public QObject, public mln::MapObserver {
    Q_OBJECT

public:
    explicit MapObserver(MapPrivate *ptr);
    ~MapObserver() override;

    // mln::MapObserver implementation.
    void onCameraWillChange(mln::MapObserver::CameraChangeMode mode) final;
    void onCameraIsChanging() final;
    void onCameraDidChange(mln::MapObserver::CameraChangeMode mode) final;
    void onWillStartLoadingMap() final;
    void onDidFinishLoadingMap() final;
    void onDidFailLoadingMap(mln::MapLoadError error, const std::string &what) final;
    void onWillStartRenderingFrame() final;
    void onDidFinishRenderingFrame(const mln::MapObserver::RenderFrameStatus &status) final;
    void onWillStartRenderingMap() final;
    void onDidFinishRenderingMap(mln::MapObserver::RenderMode mode) final;
    void onDidFinishLoadingStyle() final;
    void onSourceChanged(mln::style::Source &source) final;

signals:
    void mapChanged(Map::MapChange);
    void mapLoadingFailed(Map::MapLoadingFailure, const QString &reason);
    void copyrightsChanged(const QString &copyrightsHtml);

private:
    Q_DISABLE_COPY(MapObserver)

    MapPrivate *d_ptrRef;
};

} // namespace QMapLibre
