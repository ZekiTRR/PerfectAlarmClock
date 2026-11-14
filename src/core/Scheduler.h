#pragma once

#include "Types.h"

#include <QDateTime>
#include <QHash>
#include <QObject>
#include <QTimer>

namespace pac {

class Store;

class Scheduler final : public QObject {
    Q_OBJECT

public:
    explicit Scheduler(Store& store, QObject* parent = nullptr);

    void refresh();

signals:
    void alarmFired(const QUuid& alarmId);
    void preAlert(const QUuid& alarmId, int minutes);

private:
    void checkDue();

    Store& m_store;
    QTimer m_timer;
    QHash<QUuid, QDateTime> m_preAlerted;
    bool m_checking{false};
};

class TimerService final : public QObject {
    Q_OBJECT

public:
    explicit TimerService(Store& store, QObject* parent = nullptr);

    void refresh();

signals:
    void timerFinished(const QUuid& timerId);
    void timerStageChanged(const QUuid& timerId);

private:
    void checkDue();

    Store& m_store;
    QTimer m_timer;
    bool m_checking{false};
};

} // namespace pac
