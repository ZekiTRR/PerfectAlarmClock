#include "Scheduler.h"

#include "Schedule.h"
#include "Settings.h"
#include "TimerPlan.h"
#include "Store.h"

#include <algorithm>
#include <limits>
#include <vector>

namespace pac {
namespace {

constexpr qint64 kMaxSleepMs = 60LL * 60 * 1000;

[[nodiscard]] qint64 millisUntil(const QDateTime& target, const QDateTime& now)
{
    if (!target.isValid()) {
        return -1;
    }
    return std::max<qint64>(0, now.msecsTo(target));
}

} // namespace

Scheduler::Scheduler(Store& store, QObject* parent)
    : QObject(parent)
    , m_store(store)
{
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, &Scheduler::checkDue);
    connect(&m_store, &Store::alarmsChanged, this, &Scheduler::refresh);
    connect(&Settings::instance(), &Settings::changed, this, &Scheduler::refresh);
    refresh();
}

void Scheduler::refresh()
{
    const QDateTime now = QDateTime::currentDateTime();
    const QDateTime suspended = Settings::instance().alarmsSuspendedUntil();
    if (suspended > now) {
        const qint64 delay = std::min(now.msecsTo(suspended) + 200, kMaxSleepMs);
        m_timer.setTimerType(Qt::PreciseTimer);
        m_timer.start(static_cast<int>(std::max<qint64>(1, delay)));
        return;
    }
    qint64 earliest = std::numeric_limits<qint64>::max();

    for (const auto& alarm : m_store.data().alarms) {
        if (!alarm.enabled) {
            continue;
        }

        const QDateTime next = schedule::nextOccurrence(alarm, now);
        if (next.isValid()) {
            earliest = std::min(earliest, millisUntil(next, now));
        }

        if (alarm.preAlertMinutes > 0 && !alarm.snoozeUntil.isValid()) {
            const QDateTime occurrence = schedule::nextRegularOccurrence(alarm, now);
            if (occurrence.isValid() && m_preAlerted.value(alarm.id) != occurrence) {
                const QDateTime pre = occurrence.addSecs(-static_cast<qint64>(alarm.preAlertMinutes) * 60);
                if (pre > now) {
                    earliest = std::min(earliest, millisUntil(pre, now));
                } else if (now.secsTo(occurrence) <= alarm.preAlertMinutes * 60
                           + Settings::instance().catchUpSeconds()) {
                    earliest = 0;
                }
            }
        }

        const QDateTime catchUpStart = now.addSecs(-Settings::instance().catchUpSeconds());
        const QDateTime candidate = schedule::nextOccurrence(alarm, catchUpStart.addMSecs(-1));
        if (candidate.isValid() && candidate <= now.addMSecs(250)
            && candidate > alarm.lastTriggered) {
            earliest = 0;
        }
    }

    if (earliest == std::numeric_limits<qint64>::max()) {
        m_timer.stop();
        return;
    }

    const qint64 delay = std::min(earliest, kMaxSleepMs);
    m_timer.setTimerType(Qt::PreciseTimer);
    m_timer.start(static_cast<int>(delay));
}

void Scheduler::checkDue()
{
    if (m_checking) {
        return;
    }
    m_checking = true;

    const QDateTime now = QDateTime::currentDateTime();
    if (Settings::instance().alarmsSuspendedUntil() > now) {
        m_checking = false;
        refresh();
        return;
    }
    const QDateTime catchUpStart = now.addSecs(-Settings::instance().catchUpSeconds());
    std::vector<QUuid> fired;
    std::vector<std::pair<QUuid, int>> alerts;

    for (auto& alarm : m_store.data().alarms) {
        if (!alarm.enabled) {
            continue;
        }

        QDateTime candidate = schedule::nextOccurrence(alarm, catchUpStart.addMSecs(-1));
        QDateTime latestDue;
        while (candidate.isValid() && candidate <= now.addMSecs(250)
               && candidate > alarm.lastTriggered) {
            latestDue = candidate;
            const QDateTime next = schedule::nextOccurrence(alarm, candidate);
            if (!next.isValid() || next <= candidate) {
                break;
            }
            candidate = next;
        }

        if (latestDue.isValid()) {
            const bool wasSnooze = alarm.snoozeUntil.isValid() && alarm.snoozeUntil == latestDue;
            alarm.lastTriggered = latestDue;
            alarm.snoozeUntil = QDateTime();
            if (!wasSnooze) {
                alarm.snoozeCount = 0;
            }
            fired.push_back(alarm.id);
            continue;
        }

        if (alarm.preAlertMinutes > 0 && !alarm.snoozeUntil.isValid()) {
            const QDateTime occurrence = schedule::nextRegularOccurrence(alarm, now);
            if (occurrence.isValid() && m_preAlerted.value(alarm.id) != occurrence) {
                const QDateTime pre = occurrence.addSecs(-static_cast<qint64>(alarm.preAlertMinutes) * 60);
                if (pre <= now && now.secsTo(occurrence) <= alarm.preAlertMinutes * 60
                    + Settings::instance().catchUpSeconds()) {
                    m_preAlerted.insert(alarm.id, occurrence);
                    alerts.emplace_back(alarm.id, alarm.preAlertMinutes);
                }
            }
        }
    }

    if (!fired.empty()) {
        m_store.commit(Domain::Alarms);
    }

    m_checking = false;

    for (const auto& id : fired) {
        emit alarmFired(id);
    }
    for (const auto& [id, minutes] : alerts) {
        emit preAlert(id, minutes);
    }

    refresh();
}

TimerService::TimerService(Store& store, QObject* parent)
    : QObject(parent)
    , m_store(store)
{
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, &TimerService::checkDue);
    connect(&m_store, &Store::timersChanged, this, &TimerService::refresh);
    refresh();
}

void TimerService::refresh()
{
    const QDateTime now = QDateTime::currentDateTime();
    qint64 earliest = std::numeric_limits<qint64>::max();

    for (const auto& timer : m_store.data().timers) {
        if (timer.state != TimerState::Running) {
            continue;
        }
        if (!timer.endAt.isValid()) {
            earliest = 0;
            continue;
        }
        const qint64 delay = now.msecsTo(timer.endAt);
        if (delay <= 0) {
            earliest = 0;
        } else {
            earliest = std::min(earliest, delay);
        }
    }

    if (earliest == std::numeric_limits<qint64>::max()) {
        m_timer.stop();
        return;
    }

    const qint64 delay = std::min(earliest, kMaxSleepMs);
    m_timer.setTimerType(Qt::PreciseTimer);
    m_timer.start(static_cast<int>(delay));
}

void TimerService::checkDue()
{
    if (m_checking) {
        return;
    }
    m_checking = true;

    const QDateTime now = QDateTime::currentDateTime();
    std::vector<QUuid> finished;
    std::vector<QUuid> stageChanged;

    for (auto& timer : m_store.data().timers) {
        if (timer.state != TimerState::Running) {
            continue;
        }
        if (timer.endAt.isValid() && timer.endAt > now) {
            continue;
        }

        if (!timer.stages.empty()) {
            if (timerplan::advanceStage(timer, now)) {
                finished.push_back(timer.id);
            } else {
                stageChanged.push_back(timer.id);
            }
            continue;
        }

        finished.push_back(timer.id);
        timer.remainingMs = 0;
        if (timer.autoRepeat) {
            timer.state = TimerState::Running;
            QDateTime nextEnd = timer.endAt.addSecs(timer.durationSec);
            if (nextEnd <= now) {
                nextEnd = now.addSecs(timer.durationSec);
            }
            timer.endAt = nextEnd;
            timer.remainingMs = now.msecsTo(nextEnd);
        } else {
            timer.state = TimerState::Finished;
            timer.endAt = QDateTime();
        }
    }

    if (!finished.empty() || !stageChanged.empty()) {
        m_store.commit(Domain::Timers);
    }

    m_checking = false;
    for (const auto& id : stageChanged) {
        emit timerStageChanged(id);
    }
    for (const auto& id : finished) {
        emit timerFinished(id);
    }
    refresh();
}

} // namespace pac
