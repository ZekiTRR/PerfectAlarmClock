#include "TimerPlan.h"

#include <QRandomGenerator>

#include <algorithm>
#include <limits>

namespace pac::timerplan {
namespace {

[[nodiscard]] int cycleSeconds(const CountdownTimer& timer)
{
    int sum = 0;
    for (const auto& stage : timer.stages) {
        sum += std::max(1, stage.durationSec);
    }
    return sum;
}

void setCurrentStage(CountdownTimer& timer, const QDateTime& now, bool keepRunning)
{
    if (timer.stages.empty()) {
        return;
    }
    const TimerStage& stage = timer.stages.at(static_cast<std::size_t>(
        std::clamp(timer.currentStage, 0, static_cast<int>(timer.stages.size()) - 1)));
    timer.remainingMs = static_cast<qint64>(std::max(1, stage.durationSec)) * 1000;
    if (keepRunning) {
        timer.endAt = now.addMSecs(timer.remainingMs);
        timer.state = TimerState::Running;
    }
}

} // namespace

QString modeName(TimerMode mode)
{
    switch (mode) {
    case TimerMode::Countdown:
        return QObject::tr("Обратный отсчёт");
    case TimerMode::CountUp:
        return QObject::tr("Прямой отсчёт");
    case TimerMode::Repeating:
        return QObject::tr("Повторяющийся");
    case TimerMode::Interval:
        return QObject::tr("Интервальный");
    case TimerMode::MultiStage:
        return QObject::tr("Многоэтапный");
    case TimerMode::Hiit:
        return QStringLiteral("HIIT");
    case TimerMode::Tabata:
        return QStringLiteral("Tabata");
    case TimerMode::Kitchen:
        return QObject::tr("Кухонный");
    case TimerMode::Workout:
        return QObject::tr("Тренировка");
    case TimerMode::Presentation:
        return QObject::tr("Выступление");
    case TimerMode::Meeting:
        return QObject::tr("Встреча");
    case TimerMode::Study:
        return QObject::tr("Учёба");
    case TimerMode::Sleep:
        return QObject::tr("Сон");
    case TimerMode::Deadline:
        return QObject::tr("Дедлайн");
    case TimerMode::RandomInterval:
        return QObject::tr("Случайный интервал");
    case TimerMode::Target:
        return QObject::tr("До заданного времени");
    }
    return QObject::tr("Таймер");
}

QString stageKindName(TimerStageKind kind)
{
    switch (kind) {
    case TimerStageKind::Prepare:
        return QObject::tr("Подготовка");
    case TimerStageKind::Work:
        return QObject::tr("Работа");
    case TimerStageKind::Rest:
        return QObject::tr("Отдых");
    case TimerStageKind::Recovery:
        return QObject::tr("Восстановление");
    case TimerStageKind::Custom:
        return QObject::tr("Другое");
    }
    return QObject::tr("Этап");
}

bool hasSequence(const CountdownTimer& timer)
{
    return !timer.stages.empty();
}

int totalSeconds(const CountdownTimer& timer)
{
    if (timer.stages.empty()) {
        return std::max(1, timer.durationSec);
    }
    return std::max(1, cycleSeconds(timer) * std::max(1, timer.repeatCycles));
}

int currentRemainingMs(const CountdownTimer& timer, const QDateTime& now)
{
    if (timer.state == TimerState::Finished) {
        return 0;
    }
    if (timer.state == TimerState::Running && timer.endAt.isValid()) {
        return static_cast<int>(std::max<qint64>(0, now.msecsTo(timer.endAt)));
    }
    return static_cast<int>(std::max<qint64>(0, timer.remainingMs));
}

int remainingTotalMs(const CountdownTimer& timer, const QDateTime& now)
{
    if (timer.state == TimerState::Finished) {
        return 0;
    }
    if (timer.stages.empty()) {
        if (timer.state == TimerState::Stopped) {
            return std::max(1, timer.durationSec) * 1000;
        }
        return currentRemainingMs(timer, now);
    }
    if (timer.state == TimerState::Stopped) {
        return totalSeconds(timer) * 1000;
    }
    const int currentIndex = std::clamp(timer.currentStage, 0,
                                        static_cast<int>(timer.stages.size()) - 1);
    int remaining = currentRemainingMs(timer, now);
    for (int index = currentIndex + 1; index < static_cast<int>(timer.stages.size()); ++index) {
        remaining += std::max(1, timer.stages.at(static_cast<std::size_t>(index)).durationSec) * 1000;
    }
    const int cyclesLeft = std::max(0, timer.repeatCycles - timer.completedCycles - 1);
    remaining += cyclesLeft * cycleSeconds(timer) * 1000;
    return remaining;
}

QDateTime estimatedEnd(const CountdownTimer& timer, const QDateTime& now)
{
    if (timer.state == TimerState::Running && timer.stages.empty() && timer.endAt.isValid()) {
        return timer.endAt;
    }
    if ((timer.mode == TimerMode::Deadline || timer.mode == TimerMode::Target)
        && timer.targetAt.isValid()) {
        return timer.targetAt;
    }
    return now.addMSecs(remainingTotalMs(timer, now));
}

const TimerStage* currentStage(const CountdownTimer& timer)
{
    if (timer.stages.empty()) {
        return nullptr;
    }
    const int index = std::clamp(timer.currentStage, 0,
                                 static_cast<int>(timer.stages.size()) - 1);
    return &timer.stages.at(static_cast<std::size_t>(index));
}

int cycleCount(const CountdownTimer& timer)
{
    return std::max(1, timer.repeatCycles);
}

void prepareStart(CountdownTimer& timer, const QDateTime& now)
{
    timer.currentStage = 0;
    timer.completedCycles = 0;
    if (!timer.stages.empty()) {
        timer.durationSec = totalSeconds(timer);
        setCurrentStage(timer, now, true);
        return;
    }
    if (timer.mode == TimerMode::RandomInterval) {
        const int minimum = std::max(1, std::min(timer.randomMinSec, timer.randomMaxSec));
        const int maximum = std::max(minimum, std::max(timer.randomMinSec, timer.randomMaxSec));
        timer.durationSec = minimum
                            + QRandomGenerator::global()->bounded(maximum - minimum + 1);
    }
    if ((timer.mode == TimerMode::Deadline || timer.mode == TimerMode::Target)
        && timer.targetAt.isValid() && timer.targetAt > now) {
        timer.durationSec = static_cast<int>(std::min<qint64>(
            std::numeric_limits<int>::max(), now.secsTo(timer.targetAt)));
        timer.remainingMs = static_cast<qint64>(timer.durationSec) * 1000;
        timer.endAt = timer.targetAt;
        timer.state = TimerState::Running;
        return;
    }
    timer.remainingMs = static_cast<qint64>(std::max(1, timer.durationSec)) * 1000;
    timer.endAt = now.addMSecs(timer.remainingMs);
    timer.state = TimerState::Running;
}

bool advanceStage(CountdownTimer& timer, const QDateTime& now)
{
    if (timer.stages.empty()) {
        return true;
    }
    const QDateTime oldEnd = timer.endAt;
    ++timer.currentStage;
    if (timer.currentStage >= static_cast<int>(timer.stages.size())) {
        timer.currentStage = 0;
        ++timer.completedCycles;
    }
    if (timer.completedCycles >= timer.repeatCycles) {
        timer.state = TimerState::Finished;
        timer.endAt = QDateTime();
        timer.remainingMs = 0;
        return true;
    }
    const TimerStage& stage = timer.stages.at(static_cast<std::size_t>(timer.currentStage));
    timer.remainingMs = static_cast<qint64>(std::max(1, stage.durationSec)) * 1000;
    QDateTime nextEnd = oldEnd.addSecs(stage.durationSec);
    if (nextEnd <= now) {
        nextEnd = now.addSecs(stage.durationSec);
    }
    timer.endAt = nextEnd;
    timer.state = TimerState::Running;
    return false;
}

void skipStage(CountdownTimer& timer, const QDateTime& now)
{
    if (timer.stages.empty()) {
        return;
    }
    const bool wasPaused = timer.state == TimerState::Paused;
    if (advanceStage(timer, now)) {
        return;
    }
    if (wasPaused) {
        timer.state = TimerState::Paused;
        timer.endAt = QDateTime();
    }
}

void previousStage(CountdownTimer& timer, const QDateTime& now)
{
    if (timer.stages.empty()) {
        return;
    }
    if (timer.currentStage > 0) {
        --timer.currentStage;
    } else if (timer.completedCycles > 0) {
        --timer.completedCycles;
        timer.currentStage = static_cast<int>(timer.stages.size()) - 1;
    } else {
        return;
    }
    setCurrentStage(timer, now, timer.state == TimerState::Running);
    if (timer.state == TimerState::Paused) {
        timer.endAt = QDateTime();
    }
}

void addTime(CountdownTimer& timer, const QDateTime& now, int seconds)
{
    if (seconds <= 0) {
        return;
    }
    if (!timer.stages.empty()) {
        TimerStage& stage = timer.stages.at(static_cast<std::size_t>(
            std::clamp(timer.currentStage, 0, static_cast<int>(timer.stages.size()) - 1)));
        stage.durationSec += seconds;
        timer.remainingMs += static_cast<qint64>(seconds) * 1000;
        if (timer.state == TimerState::Running && timer.endAt.isValid()) {
            timer.endAt = timer.endAt.addSecs(seconds);
        }
        timer.durationSec = totalSeconds(timer);
        return;
    }
    timer.durationSec += seconds;
    timer.remainingMs += static_cast<qint64>(seconds) * 1000;
    if (timer.state == TimerState::Running && timer.endAt.isValid()) {
        timer.endAt = timer.endAt.addSecs(seconds);
    }
    Q_UNUSED(now)
}

void reset(CountdownTimer& timer)
{
    timer.state = TimerState::Stopped;
    timer.endAt = QDateTime();
    timer.currentStage = 0;
    timer.completedCycles = 0;
    timer.durationSec = totalSeconds(timer);
    timer.remainingMs = timer.stages.empty()
                            ? static_cast<qint64>(std::max(1, timer.durationSec)) * 1000
                            : static_cast<qint64>(std::max(1, timer.stages.front().durationSec)) * 1000;
}

} // namespace pac::timerplan
