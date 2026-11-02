// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include <mln/actor/scheduler.hpp>
#include <mln/util/util.hpp>

#include <QObject>

#include <condition_variable>
#include <mutex>
#include <queue>

namespace QMapLibre {

class Scheduler : public QObject, public mln::Scheduler {
    Q_OBJECT

public:
    Scheduler();
    ~Scheduler() override;

    // mln::Scheduler implementation.
    void schedule(const mln::util::SimpleIdentity identity, std::function<void()> &&function) final;
    void schedule(std::function<void()> &&function) final;

    void waitForEmpty(const mln::util::SimpleIdentity tag = mln::util::SimpleIdentity::Empty) override;

    mapbox::base::WeakPtr<mln::Scheduler> makeWeakPtr() override { return weakFactory.makeWeakPtr(); }

    void processEvents();

signals:
    void needsProcessing();

private:
    MBGL_STORE_THREAD(tid);

    std::mutex m_taskQueueMutex;
    std::condition_variable cvEmpty;
    std::atomic<std::size_t> pendingItems;
    std::queue<std::function<void()>> m_taskQueue;
    mapbox::base::WeakPtrFactory<Scheduler> weakFactory{this};
    // Do not add members here, see `WeakPtrFactory`
};

} // namespace QMapLibre
