#include "Schedule.h"

#include "Catalog.h"

#include <QObject>
#include <QHash>
#include <QTimeZone>

#include <algorithm>
#include <vector>

namespace pac::schedule {
namespace {

std::vector<NonWorkingDay> g_nonWorkingDays;

[[nodiscard]] QTimeZone zoneFor(const Alarm& alarm)
{
    if (alarm.timeZoneId.isEmpty()) {
        return QTimeZone::systemTimeZone();
    }
    const QTimeZone zone(alarm.timeZoneId.toUtf8());
    return zone.isValid() ? zone : QTimeZone::systemTimeZone();
}

[[nodiscard]] bool dateAllowed(const Alarm& alarm, const QDate& date)
{
    if (alarm.activeFromDate.isValid() && date < alarm.activeFromDate) {
        return false;
    }
    if (alarm.activeToDate.isValid() && date > alarm.activeToDate) {
        return false;
    }
    return true;
}

[[nodiscard]] QDate firstAllowedDate(const Alarm& alarm, QDate date)
{
    if (alarm.activeFromDate.isValid() && date < alarm.activeFromDate) {
        return alarm.activeFromDate;
    }
    return date;
}

[[nodiscard]] bool isHoliday(const QDate& date)
{
    return std::any_of(g_nonWorkingDays.cbegin(), g_nonWorkingDays.cend(),
                       [&date](const NonWorkingDay& day) { return day.date == date; });
}

[[nodiscard]] QDate nextWorkday(QDate date)
{
    for (int guard = 0; guard < 90; ++guard) {
        if (date.dayOfWeek() >= Qt::Monday && date.dayOfWeek() <= Qt::Friday
            && !isHoliday(date)) {
            return date;
        }
        date = date.addDays(1);
    }
    return {};
}

[[nodiscard]] QDate nthWeekdayOfMonth(int year, int month, Qt::DayOfWeek weekday, int week)
{
    const QDate first(year, month, 1);
    const int offset = (static_cast<int>(weekday) - first.dayOfWeek() + 7) % 7;
    if (week < 0) {
        const QDate last(year, month, first.daysInMonth());
        const int back = (last.dayOfWeek() - static_cast<int>(weekday) + 7) % 7;
        return last.addDays(-back);
    }
    const QDate result = first.addDays(offset + (week - 1) * 7);
    return result.month() == month ? result : QDate();
}

[[nodiscard]] QDate lastWorkdayOfMonth(int year, int month)
{
    QDate date(year, month, QDate(year, month, 1).daysInMonth());
    for (int guard = 0; guard < 40; ++guard) {
        if (date.dayOfWeek() >= Qt::Monday && date.dayOfWeek() <= Qt::Friday
            && !isHoliday(date)) {
            return date;
        }
        date = date.addDays(-1);
    }
    return {};
}

[[nodiscard]] bool insideWindow(const QTime& from, const QTime& to, const QTime& time)
{
    if (from == to) {
        return true;
    }
    if (from < to) {
        return time >= from && time <= to;
    }
    return time >= from || time <= to;
}

struct Rule {
    QTime time;
    RepeatKind repeat{RepeatKind::Daily};
    QList<Qt::DayOfWeek> days;
    int monthDay{1};
    int monthWeek{1};
    int intervalDays{2};
    int intervalWeeks{2};
    int intervalMinutes{30};
    QTime activeFrom{0, 0};
    QTime activeTo{23, 59};
    QDate anchor;
};

[[nodiscard]] Rule primaryRule(const Alarm& alarm)
{
    Rule rule;
    rule.time = alarm.time;
    rule.repeat = alarm.repeat;
    rule.days = alarm.days;
    rule.monthDay = alarm.monthDay;
    rule.monthWeek = alarm.monthWeek;
    rule.intervalDays = alarm.intervalDays;
    rule.intervalWeeks = alarm.intervalWeeks;
    rule.intervalMinutes = alarm.intervalMinutes;
    rule.activeFrom = alarm.activeFrom;
    rule.activeTo = alarm.activeTo;
    rule.anchor = alarm.onceDate;
    if (alarm.repeat == RepeatKind::EveryNDays || alarm.repeat == RepeatKind::EveryNWeeks) {
        if (alarm.seriesStart.isValid()) {
            rule.anchor = alarm.seriesStart;
        } else if (!alarm.onceDate.isValid() && alarm.activeFromDate.isValid()) {
            rule.anchor = alarm.activeFromDate;
        }
    }
    return rule;
}

[[nodiscard]] Rule extraRule(const AlarmSchedule& schedule)
{
    Rule rule;
    rule.time = schedule.time;
    rule.repeat = schedule.repeat;
    rule.days = schedule.days;
    rule.monthDay = schedule.monthDay;
    rule.monthWeek = schedule.monthWeek;
    rule.intervalDays = schedule.intervalDays;
    rule.intervalWeeks = schedule.intervalWeeks;
    rule.intervalMinutes = schedule.intervalMinutes;
    rule.activeFrom = schedule.activeFrom;
    rule.activeTo = schedule.activeTo;
    rule.anchor = schedule.anchorDate;
    return rule;
}

[[nodiscard]] QList<Rule> rulesFor(const Alarm& alarm)
{
    QList<Rule> rules;
    rules.append(primaryRule(alarm));
    for (const auto& schedule : alarm.extraSchedules) {
        if (schedule.enabled) {
            rules.append(extraRule(schedule));
        }
    }
    return rules;
}

[[nodiscard]] bool matchesDate(const Rule& rule, const QDate& date)
{
    const int dow = date.dayOfWeek();
    switch (rule.repeat) {
    case RepeatKind::Once:
        return rule.anchor.isValid() && date == rule.anchor;
    case RepeatKind::Daily:
        return true;
    case RepeatKind::Weekdays:
        return dow >= Qt::Monday && dow <= Qt::Friday;
    case RepeatKind::Weekends:
        return dow == Qt::Saturday || dow == Qt::Sunday;
    case RepeatKind::Weekly:
        return rule.days.contains(static_cast<Qt::DayOfWeek>(dow));
    case RepeatKind::Monthly: {
        const int wanted = rule.monthDay <= 0 || rule.monthDay >= 31
                               ? date.daysInMonth()
                               : std::clamp(rule.monthDay, 1, date.daysInMonth());
        return date.day() == wanted;
    }
    case RepeatKind::Yearly: {
        if (!rule.anchor.isValid()) {
            return false;
        }
        const QDate first(date.year(), rule.anchor.month(), 1);
        const int day = std::min(rule.anchor.day(), first.daysInMonth());
        return date.month() == rule.anchor.month() && date.day() == day;
    }
    case RepeatKind::Interval:
        return false;
    case RepeatKind::EveryNDays: {
        if (!rule.anchor.isValid() || date < rule.anchor) {
            return false;
        }
        return rule.anchor.daysTo(date) % std::max(1, rule.intervalDays) == 0;
    }
    case RepeatKind::EveryNWeeks: {
        if (!rule.anchor.isValid() || date < rule.anchor) {
            return false;
        }
        const QDate anchorWeek = rule.anchor.addDays(1 - rule.anchor.dayOfWeek());
        const QDate dateWeek = date.addDays(1 - date.dayOfWeek());
        const int weeks = static_cast<int>(anchorWeek.daysTo(dateWeek) / 7);
        if (weeks < 0 || weeks % std::max(1, rule.intervalWeeks) != 0) {
            return false;
        }
        const Qt::DayOfWeek wanted = rule.days.isEmpty()
                                         ? static_cast<Qt::DayOfWeek>(rule.anchor.dayOfWeek())
                                         : rule.days.first();
        return rule.days.isEmpty() ? date.dayOfWeek() == static_cast<int>(wanted)
                                   : rule.days.contains(static_cast<Qt::DayOfWeek>(dow));
    }
    case RepeatKind::MonthlyNthWeekday: {
        const Qt::DayOfWeek weekday = rule.days.isEmpty()
                                          ? static_cast<Qt::DayOfWeek>(rule.anchor.isValid()
                                                                          ? rule.anchor.dayOfWeek()
                                                                          : Qt::Monday)
                                          : rule.days.first();
        const int week = rule.monthWeek == 0 ? 1 : rule.monthWeek;
        const QDate wanted = nthWeekdayOfMonth(date.year(), date.month(), weekday, week);
        return wanted.isValid() && wanted == date;
    }
    case RepeatKind::LastWeekdayOfMonth:
        return lastWorkdayOfMonth(date.year(), date.month()) == date;
    }
    return false;
}

[[nodiscard]] QDateTime randomizedCandidate(const Alarm& alarm, const Rule& rule,
                                             const QDate& date, const QTimeZone& zone)
{
    const QDateTime base(date, rule.time, zone);
    if (!alarm.randomWindow) {
        return base;
    }
    const int before = std::max(0, alarm.randomBeforeMinutes) * 60;
    const int after = std::max(0, alarm.randomAfterMinutes) * 60;
    const int span = before + after;
    if (span <= 0) {
        return base;
    }
    uint seed = qHash(alarm.id.toString(), 0x9e3779b9u);
    seed ^= qHash(date.toJulianDay(), 0x85ebca6bu);
    seed ^= qHash(rule.time.msecsSinceStartOfDay(), 0xc2b2ae35u);
    seed ^= qHash(static_cast<int>(rule.repeat), 0x27d4eb2fu);
    seed = seed * 1664525u + 1013904223u;
    const int offset = static_cast<int>(seed % static_cast<uint>(span + 1)) - before;
    return base.addSecs(offset);
}

[[nodiscard]] QDateTime nextStandard(const Alarm& alarm, const Rule& rule, const QDateTime& after)
{
    if (rule.repeat == RepeatKind::Interval) {
        return {};
    }
    const QTimeZone zone = zoneFor(alarm);
    QDate date = firstAllowedDate(alarm, after.date());
    for (int guard = 0; guard < 366 * 30; ++guard) {
        if (alarm.activeToDate.isValid() && date > alarm.activeToDate) {
            return {};
        }
        if (!matchesDate(rule, date)) {
            date = date.addDays(1);
            continue;
        }
        if (alarm.exceptions.contains(date)) {
            date = date.addDays(1);
            continue;
        }

        QDate effective = date;
        if (isHoliday(date)) {
            if (alarm.holidayPolicy == HolidayPolicy::Skip) {
                date = date.addDays(1);
                continue;
            }
            if (alarm.holidayPolicy == HolidayPolicy::ShiftToNextWorkday) {
                effective = nextWorkday(date);
                if (!effective.isValid() || alarm.exceptions.contains(effective)) {
                    date = date.addDays(1);
                    continue;
                }
            }
        }
        if (!dateAllowed(alarm, effective)) {
            date = date.addDays(1);
            continue;
        }
        const QDateTime candidate = randomizedCandidate(alarm, rule, effective, zone);
        if (!candidate.isValid() || !dateAllowed(alarm, candidate.date())) {
            date = date.addDays(1);
            continue;
        }
        if (candidate > after) {
            return candidate;
        }
        date = date.addDays(1);
    }
    return {};
}

[[nodiscard]] QDateTime intervalAnchor(const Alarm& alarm, const Rule& rule, const QDateTime& after)
{
    const QTimeZone zone = zoneFor(alarm);
    if (rule.repeat == RepeatKind::Interval && rule.anchor.isValid()) {
        return QDateTime(rule.anchor, rule.activeFrom, zone);
    }
    if (!alarm.intervalAnchor.isValid()) {
        return after;
    }
    return alarm.intervalAnchor;
}

[[nodiscard]] QDateTime nextInterval(const Alarm& alarm, const Rule& rule,
                                     const QDateTime& after)
{
    const QTimeZone zone = zoneFor(alarm);
    const qint64 stepMs = static_cast<qint64>(std::max(1, rule.intervalMinutes)) * 60'000;
    const QDateTime anchor = intervalAnchor(alarm, rule, after);
    qint64 delta = anchor.msecsTo(after);
    if (delta < 0) {
        delta = 0;
    }
    qint64 slot = (delta + stepMs - 1) / stepMs;
    QDateTime candidate = anchor.addMSecs(slot * stepMs);
    if (candidate <= after) {
        candidate = candidate.addMSecs(stepMs);
    }

    for (int guard = 0; guard < 2000; ++guard) {
        const QDate date = candidate.date();
        if (!dateAllowed(alarm, date)) {
            if (alarm.activeToDate.isValid() && date > alarm.activeToDate) {
                return {};
            }
            const QDate start = firstAllowedDate(alarm, date);
            candidate = QDateTime(start, rule.activeFrom, zone);
            if (alarm.exceptions.contains(start)
                || (isHoliday(start) && alarm.holidayPolicy == HolidayPolicy::Skip)) {
                candidate = candidate.addDays(1);
            }
            continue;
        }
        if (!insideWindow(rule.activeFrom, rule.activeTo, candidate.time())) {
            QDate targetDate = date;
            if (rule.activeFrom < rule.activeTo) {
                if (candidate.time() > rule.activeTo) {
                    targetDate = targetDate.addDays(1);
                }
            }
            const QDateTime windowStart(targetDate, rule.activeFrom, zone);
            qint64 fromAnchor = anchor.msecsTo(windowStart);
            qint64 targetSlot = fromAnchor <= 0 ? 0 : (fromAnchor + stepMs - 1) / stepMs;
            candidate = anchor.addMSecs(targetSlot * stepMs);
            if (candidate <= after) {
                candidate = candidate.addMSecs(stepMs);
            }
            continue;
        }
        if (alarm.exceptions.contains(date)
            || (isHoliday(date) && alarm.holidayPolicy == HolidayPolicy::Skip)) {
            candidate = candidate.addMSecs(stepMs);
            continue;
        }
        if (candidate > after) {
            return candidate;
        }
        candidate = candidate.addMSecs(stepMs);
    }
    return {};
}

[[nodiscard]] QString nthText(int week)
{
    switch (week) {
    case 1:
        return QObject::tr("первый");
    case 2:
        return QObject::tr("второй");
    case 3:
        return QObject::tr("третий");
    case 4:
        return QObject::tr("четвёртый");
    default:
        return QObject::tr("последний");
    }
}

} // namespace

void setNonWorkingDays(const std::vector<NonWorkingDay>& days)
{
    g_nonWorkingDays = days;
}

QDateTime nextOccurrence(const Alarm& alarm, const QDateTime& after)
{
    if (alarm.snoozeUntil.isValid() && alarm.snoozeUntil > after) {
        return alarm.snoozeUntil;
    }
    return nextRegularOccurrence(alarm, after);
}

QDateTime nextRegularOccurrence(const Alarm& alarm, const QDateTime& after)
{
    QDateTime best;
    for (const Rule& rule : rulesFor(alarm)) {
        QDateTime candidate;
        if (rule.repeat == RepeatKind::Interval) {
            candidate = nextInterval(alarm, rule, after);
        } else {
            candidate = nextStandard(alarm, rule, after);
        }
        if (candidate.isValid() && (!best.isValid() || candidate < best)) {
            best = candidate;
        }
    }
    return best;
}

bool occursOnDate(const Alarm& alarm, const QDate& date)
{
    if (!date.isValid() || !dateAllowed(alarm, date)) {
        return false;
    }
    for (const Rule& rule : rulesFor(alarm)) {
        if (rule.repeat == RepeatKind::Interval) {
            return true;
        }
        if (!matchesDate(rule, date)) {
            continue;
        }
        if (alarm.exceptions.contains(date)) {
            continue;
        }
        QDate effective = date;
        if (isHoliday(date)) {
            if (alarm.holidayPolicy == HolidayPolicy::Skip) {
                continue;
            }
            if (alarm.holidayPolicy == HolidayPolicy::ShiftToNextWorkday) {
                effective = nextWorkday(date);
            }
        }
        if (effective == date && dateAllowed(alarm, effective)) {
            return true;
        }
    }
    return false;
}

QString repeatSummary(const Alarm& alarm)
{
    QString summary;
    switch (alarm.repeat) {
    case RepeatKind::Once:
        summary = QObject::tr("Один раз");
        if (alarm.onceDate.isValid()) {
            summary += QStringLiteral(" · ") + dateText(alarm.onceDate);
        }
        break;
    case RepeatKind::Daily:
        summary = QObject::tr("Каждый день");
        break;
    case RepeatKind::Weekdays:
        summary = QObject::tr("По будням · Пн–Пт");
        break;
    case RepeatKind::Weekends:
        summary = QObject::tr("По выходным · Сб–Вс");
        break;
    case RepeatKind::Weekly: {
        const QString days = dayListText(alarm.days);
        summary = days.isEmpty() ? QObject::tr("Еженедельно") : QObject::tr("По %1").arg(days);
        break;
    }
    case RepeatKind::Monthly:
        summary = alarm.monthDay <= 0 || alarm.monthDay >= 31
                      ? QObject::tr("Последний день месяца")
                      : QObject::tr("Ежемесячно · %1 числа").arg(alarm.monthDay);
        break;
    case RepeatKind::Yearly: {
        const QDate seed = alarm.onceDate.isValid() ? alarm.onceDate : QDate::currentDate();
        summary = QObject::tr("Ежегодно · %1 %2").arg(seed.day()).arg(
            catalog::monthNameGenitive(seed.month()));
        break;
    }
    case RepeatKind::Interval:
        if (alarm.intervalMinutes >= 60 && alarm.intervalMinutes % 60 == 0) {
            summary = QObject::tr("Каждые %1 ч").arg(alarm.intervalMinutes / 60);
        } else {
            summary = QObject::tr("Каждые %1 мин").arg(alarm.intervalMinutes);
        }
        if (alarm.activeFrom != QTime(0, 0) || alarm.activeTo != QTime(23, 59)) {
            summary += QObject::tr(" · %1–%2").arg(timeText(alarm.activeFrom, true),
                                                  timeText(alarm.activeTo, true));
        }
        break;
    case RepeatKind::EveryNDays:
        summary = QObject::tr("Каждые %1 дн.").arg(std::max(1, alarm.intervalDays));
        if (alarm.seriesStart.isValid()) {
            summary += QObject::tr(" · с %1").arg(dateText(alarm.seriesStart, false));
        }
        break;
    case RepeatKind::EveryNWeeks: {
        const QString days = dayListText(alarm.days);
        summary = QObject::tr("Каждые %1 нед.").arg(std::max(1, alarm.intervalWeeks));
        if (!days.isEmpty()) {
            summary += QStringLiteral(" · ") + days;
        }
        if (alarm.seriesStart.isValid()) {
            summary += QObject::tr(" · с %1").arg(dateText(alarm.seriesStart, false));
        }
        break;
    }
    case RepeatKind::MonthlyNthWeekday: {
        const Qt::DayOfWeek weekday = alarm.days.isEmpty()
                                          ? static_cast<Qt::DayOfWeek>(Qt::Tuesday)
                                          : alarm.days.first();
        summary = QObject::tr("%1 %2 месяца")
                      .arg(nthText(alarm.monthWeek == 0 ? 1 : alarm.monthWeek),
                           catalog::dayLong(weekday).toLower());
        break;
    }
    case RepeatKind::LastWeekdayOfMonth:
        summary = QObject::tr("Последний рабочий день месяца");
        break;
    }

    if (alarm.extraSchedules.size() > 0) {
        summary += QObject::tr(" · доп. расписаний: %1").arg(alarm.extraSchedules.size());
    }
    if (!alarm.timeZoneId.isEmpty()) {
        summary += QStringLiteral(" · ") + alarm.timeZoneId;
    }
    if (alarm.randomWindow) {
        summary += QObject::tr(" · случайное время −%1…+%2 мин")
                       .arg(alarm.randomBeforeMinutes)
                       .arg(alarm.randomAfterMinutes);
    }
    if (alarm.holidayPolicy == HolidayPolicy::Skip) {
        summary += QObject::tr(" · пропуск праздников");
    } else if (alarm.holidayPolicy == HolidayPolicy::ShiftToNextWorkday) {
        summary += QObject::tr(" · перенос с праздников");
    }
    if (!alarm.exceptions.isEmpty()) {
        summary += QObject::tr(" · исключений: %1").arg(alarm.exceptions.size());
    }
    if (alarm.activeFromDate.isValid() && alarm.activeToDate.isValid()) {
        summary += QObject::tr(" · с %1 по %2")
                       .arg(dateText(alarm.activeFromDate, false),
                            dateText(alarm.activeToDate, false));
    } else if (alarm.activeToDate.isValid()) {
        summary += QObject::tr(" · до %1").arg(dateText(alarm.activeToDate, false));
    } else if (alarm.activeFromDate.isValid()) {
        summary += QObject::tr(" · с %1").arg(dateText(alarm.activeFromDate, false));
    }
    return summary;
}

QString countdownText(qint64 seconds)
{
    if (seconds <= 0) {
        return QObject::tr("сейчас");
    }
    const qint64 days = seconds / 86'400;
    const qint64 rest = seconds % 86'400;
    const qint64 hours = rest / 3'600;
    const qint64 minutes = (rest % 3'600) / 60;
    const qint64 secs = rest % 60;
    const QString clock = QStringLiteral("%1:%2:%3")
                              .arg(hours, 2, 10, QLatin1Char('0'))
                              .arg(minutes, 2, 10, QLatin1Char('0'))
                              .arg(secs, 2, 10, QLatin1Char('0'));
    if (days > 0) {
        return QStringLiteral("%1 дн. %2").arg(days).arg(clock);
    }
    return clock;
}

QString timeText(const QTime& time, bool use24h)
{
    if (!time.isValid()) {
        return {};
    }
    return use24h ? time.toString(QStringLiteral("HH:mm"))
                  : time.toString(QStringLiteral("h:mm AP"));
}

QString dateText(const QDate& date, bool withYear)
{
    if (!date.isValid()) {
        return {};
    }
    const QString base = QStringLiteral("%1 %2").arg(date.day()).arg(
        catalog::monthNameGenitive(date.month()));
    return withYear ? QStringLiteral("%1 %2").arg(base, QString::number(date.year())) : base;
}

QString dateTimeText(const QDateTime& dateTime, const QDateTime& now, bool use24h)
{
    if (!dateTime.isValid()) {
        return {};
    }
    const QDate today = now.date();
    const QDate date = dateTime.date();
    QString day;
    if (date == today) {
        day = QObject::tr("Сегодня");
    } else if (date == today.addDays(1)) {
        day = QObject::tr("Завтра");
    } else if (date == today.addDays(-1)) {
        day = QObject::tr("Вчера");
    } else if (date.year() == today.year()) {
        day = QStringLiteral("%1 %2").arg(date.day()).arg(catalog::monthNameGenitive(date.month()));
    } else {
        day = dateText(date);
    }
    return QStringLiteral("%1, %2").arg(day, timeText(dateTime.time(), use24h));
}

QString dayListText(const QList<Qt::DayOfWeek>& days)
{
    if (days.size() == 7) {
        return QObject::tr("каждый день");
    }
    QStringList parts;
    for (int d = Qt::Monday; d <= Qt::Sunday; ++d) {
        if (days.contains(static_cast<Qt::DayOfWeek>(d))) {
            parts.append(catalog::dayShort(static_cast<Qt::DayOfWeek>(d)));
        }
    }
    return parts.join(QStringLiteral(", "));
}

QString phaseName(PomodoroPhase phase)
{
    switch (phase) {
    case PomodoroPhase::Work:
        return QObject::tr("Фокус");
    case PomodoroPhase::ShortBreak:
        return QObject::tr("Короткий перерыв");
    case PomodoroPhase::LongBreak:
        return QObject::tr("Длинный перерыв");
    }
    return {};
}

QString formatStopwatch(qint64 milliseconds)
{
    const qint64 cs = (milliseconds / 10) % 100;
    const qint64 totalSeconds = milliseconds / 1'000;
    const qint64 secs = totalSeconds % 60;
    const qint64 minutes = (totalSeconds / 60) % 60;
    const qint64 hours = totalSeconds / 3'600;
    return QStringLiteral("%1:%2:%3.%4")
        .arg(hours, 2, 10, QLatin1Char('0'))
        .arg(minutes, 2, 10, QLatin1Char('0'))
        .arg(secs, 2, 10, QLatin1Char('0'))
        .arg(cs, 2, 10, QLatin1Char('0'));
}

QString formatTimer(qint64 milliseconds)
{
    const qint64 totalSeconds = std::max<qint64>(0, milliseconds) / 1'000;
    const qint64 secs = totalSeconds % 60;
    const qint64 minutes = (totalSeconds / 60) % 60;
    const qint64 hours = totalSeconds / 3'600;
    return QStringLiteral("%1:%2:%3")
        .arg(hours, 2, 10, QLatin1Char('0'))
        .arg(minutes, 2, 10, QLatin1Char('0'))
        .arg(secs, 2, 10, QLatin1Char('0'));
}

} // namespace pac::schedule
