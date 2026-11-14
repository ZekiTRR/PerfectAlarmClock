#pragma once

#include "Types.h"

#include <QDateTime>
#include <QString>

namespace pac::schedule {

void setNonWorkingDays(const std::vector<NonWorkingDay>& days);
[[nodiscard]] QDateTime nextOccurrence(const Alarm& alarm, const QDateTime& after);
[[nodiscard]] QDateTime nextRegularOccurrence(const Alarm& alarm, const QDateTime& after);
[[nodiscard]] bool occursOnDate(const Alarm& alarm, const QDate& date);
[[nodiscard]] QString repeatSummary(const Alarm& alarm);
[[nodiscard]] QString countdownText(qint64 seconds);
[[nodiscard]] QString timeText(const QTime& time, bool use24h);
[[nodiscard]] QString dateText(const QDate& date, bool withYear = true);
[[nodiscard]] QString dateTimeText(const QDateTime& dateTime, const QDateTime& now, bool use24h);
[[nodiscard]] QString dayListText(const QList<Qt::DayOfWeek>& days);
[[nodiscard]] QString phaseName(PomodoroPhase phase);
[[nodiscard]] QString formatStopwatch(qint64 milliseconds);
[[nodiscard]] QString formatTimer(qint64 milliseconds);

} // namespace pac::schedule
