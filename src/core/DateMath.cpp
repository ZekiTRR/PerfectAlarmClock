#include "DateMath.h"

#include "Catalog.h"

#include <QObject>

#include <algorithm>
#include <cstdlib>

namespace pac::datemath {

DateSpan dateDifference(const QDate& from, const QDate& to)
{
    if (!from.isValid() || !to.isValid()) {
        return {};
    }
    DateSpan span;
    QDate start = from;
    QDate end = to;
    if (start > end) {
        std::swap(start, end);
        span.negative = true;
    }
    int years = end.year() - start.year();
    QDate cursor = start.addYears(years);
    if (cursor > end) {
        --years;
        cursor = start.addYears(years);
    }
    int months = (end.year() - cursor.year()) * 12 + end.month() - cursor.month();
    QDate candidate = cursor.addMonths(months);
    if (candidate > end) {
        --months;
        candidate = cursor.addMonths(months);
    }
    span.years = years;
    span.months = std::max(0, months);
    span.days = static_cast<int>(candidate.daysTo(end));
    return span;
}

DateTimeSpan dateTimeDifference(const QDateTime& from, const QDateTime& to)
{
    if (!from.isValid() || !to.isValid()) {
        return {};
    }
    DateTimeSpan span;
    QDateTime start = from;
    QDateTime end = to;
    if (start > end) {
        std::swap(start, end);
        span.negative = true;
    }
    int years = end.date().year() - start.date().year();
    QDateTime cursor = start.addYears(years);
    if (cursor > end) {
        --years;
        cursor = start.addYears(years);
    }
    int months = (end.date().year() - cursor.date().year()) * 12
                 + end.date().month() - cursor.date().month();
    QDateTime candidate = cursor.addMonths(months);
    if (candidate > end) {
        --months;
        candidate = cursor.addMonths(months);
    }
    cursor = candidate;
    span.years = std::max(0, years);
    span.months = std::max(0, months);
    qint64 remainingMs = cursor.msecsTo(end);
    if (remainingMs < 0) {
        remainingMs = 0;
    }
    span.days = static_cast<int>(remainingMs / 86'400'000);
    remainingMs %= 86'400'000;
    span.hours = static_cast<int>(remainingMs / 3'600'000);
    remainingMs %= 3'600'000;
    span.minutes = static_cast<int>(remainingMs / 60'000);
    span.seconds = static_cast<int>((remainingMs % 60'000) / 1'000);
    return span;
}

QDate addBusinessDays(QDate date, int businessDays)
{
    if (!date.isValid()) {
        return {};
    }
    const int direction = businessDays >= 0 ? 1 : -1;
    int remaining = std::abs(businessDays);
    while (remaining > 0) {
        date = date.addDays(direction);
        if (date.dayOfWeek() >= Qt::Monday && date.dayOfWeek() <= Qt::Friday) {
            --remaining;
        }
    }
    return date;
}

int countWeekdayInclusive(const QDate& from, const QDate& to, Qt::DayOfWeek weekday)
{
    if (!from.isValid() || !to.isValid()) {
        return 0;
    }
    QDate start = from;
    QDate end = to;
    if (start > end) {
        std::swap(start, end);
    }
    const qint64 total = start.daysTo(end) + 1;
    const int firstWeekday = start.dayOfWeek();
    int index = (static_cast<int>(weekday) - firstWeekday + 7) % 7;
    if (index >= total) {
        return 0;
    }
    return static_cast<int>((total - index + 6) / 7);
}

int countBusinessDaysInclusive(const QDate& from, const QDate& to)
{
    int result = 0;
    for (int day = Qt::Monday; day <= Qt::Friday; ++day) {
        result += countWeekdayInclusive(from, to, static_cast<Qt::DayOfWeek>(day));
    }
    return result;
}

int countWeekendDaysInclusive(const QDate& from, const QDate& to)
{
    return countWeekdayInclusive(from, to, Qt::Saturday)
           + countWeekdayInclusive(from, to, Qt::Sunday);
}

QDate firstDayOfMonth(const QDate& date)
{
    return date.isValid() ? QDate(date.year(), date.month(), 1) : QDate();
}

QDate lastDayOfMonth(const QDate& date)
{
    return date.isValid() ? QDate(date.year(), date.month(), date.daysInMonth()) : QDate();
}

QDate firstDayOfQuarter(const QDate& date)
{
    if (!date.isValid()) {
        return {};
    }
    const int month = ((date.month() - 1) / 3) * 3 + 1;
    return QDate(date.year(), month, 1);
}

QDate lastDayOfQuarter(const QDate& date)
{
    if (!date.isValid()) {
        return {};
    }
    const int month = firstDayOfQuarter(date).month() + 2;
    return QDate(date.year(), month, QDate(date.year(), month, 1).daysInMonth());
}

int dayOfYear(const QDate& date)
{
    if (!date.isValid()) {
        return 0;
    }
    return static_cast<int>(QDate(date.year(), 1, 1).daysTo(date)) + 1;
}

int daysToEndOfYear(const QDate& date)
{
    return date.isValid() ? static_cast<int>(date.daysTo(QDate(date.year(), 12, 31))) : 0;
}

int isoWeekNumber(const QDate& date, int* isoYear)
{
    if (!date.isValid()) {
        if (isoYear != nullptr) {
            *isoYear = 0;
        }
        return 0;
    }
    int year = date.year();
    const int week = date.weekNumber(&year);
    if (isoYear != nullptr) {
        *isoYear = year;
    }
    return week;
}

QString dateSpanText(const DateSpan& span)
{
    QStringList parts;
    if (span.years > 0) {
        parts.append(QObject::tr("%1 г.").arg(span.years));
    }
    if (span.months > 0) {
        parts.append(QObject::tr("%1 мес.").arg(span.months));
    }
    if (span.days > 0 || parts.isEmpty()) {
        parts.append(QObject::tr("%1 дн.").arg(span.days));
    }
    return (span.negative ? QStringLiteral("−") : QString()) + parts.join(QStringLiteral(" "));
}

QString dateTimeSpanText(const DateTimeSpan& span)
{
    QStringList parts;
    if (span.years > 0) {
        parts.append(QObject::tr("%1 г.").arg(span.years));
    }
    if (span.months > 0) {
        parts.append(QObject::tr("%1 мес.").arg(span.months));
    }
    if (span.days > 0 || parts.isEmpty()) {
        parts.append(QObject::tr("%1 дн.").arg(span.days));
    }
    parts.append(QObject::tr("%1 ч.").arg(span.hours));
    parts.append(QObject::tr("%1 мин").arg(span.minutes));
    parts.append(QObject::tr("%1 с").arg(span.seconds));
    return (span.negative ? QStringLiteral("−") : QString()) + parts.join(QStringLiteral(" "));
}

QString isoWeekText(const QDate& date)
{
    int year = 0;
    const int week = isoWeekNumber(date, &year);
    return QStringLiteral("%1-W%2").arg(year).arg(week, 2, 10, QLatin1Char('0'));
}

} // namespace pac::datemath
