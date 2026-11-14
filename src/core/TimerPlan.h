#pragma once

#include "Types.h"

#include <QDateTime>
#include <QString>

namespace pac::timerplan {

[[nodiscard]] QString modeName(TimerMode mode);
[[nodiscard]] QString stageKindName(TimerStageKind kind);
[[nodiscard]] int totalSeconds(const CountdownTimer& timer);
[[nodiscard]] int remainingTotalMs(const CountdownTimer& timer, const QDateTime& now);
[[nodiscard]] int currentRemainingMs(const CountdownTimer& timer, const QDateTime& now);
[[nodiscard]] QDateTime estimatedEnd(const CountdownTimer& timer, const QDateTime& now);
[[nodiscard]] const TimerStage* currentStage(const CountdownTimer& timer);
[[nodiscard]] int cycleCount(const CountdownTimer& timer);
[[nodiscard]] bool hasSequence(const CountdownTimer& timer);

void prepareStart(CountdownTimer& timer, const QDateTime& now);
bool advanceStage(CountdownTimer& timer, const QDateTime& now);
void skipStage(CountdownTimer& timer, const QDateTime& now);
void previousStage(CountdownTimer& timer, const QDateTime& now);
void addTime(CountdownTimer& timer, const QDateTime& now, int seconds);
void reset(CountdownTimer& timer);

} // namespace pac::timerplan
