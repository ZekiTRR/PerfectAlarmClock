#pragma once

#include <QDate>
#include <QDateTime>
#include <QString>

namespace pac::datemath {

struct DateSpan {
    bool negative{false};
    int years{0};
    int months{0};
    int days{0};
};

struct DateTimeSpan {
    bool negative{false};
    int years{0};
    int months{0};
    int days{0};
    int hours{0};
    int minutes{0};
    int seconds{0};
};

[[nodiscard]] DateSpan dateDifference(const QDate& from, const QDate& to);
[[nodiscard]] DateTimeSpan dateTimeDifference(const QDateTime& from, const QDateTime& to);
[[nodiscard]] QDate addBusinessDays(QDate date, int businessDays);
[[nodiscard]] int countWeekdayInclusive(const QDate& from, const QDate& to,
                                        Qt::DayOfWeek weekday);
[[nodiscard]] int countBusinessDaysInclusive(const QDate& from, const QDate& to);
[[nodiscard]] int countWeekendDaysInclusive(const QDate& from, const QDate& to);
[[nodiscard]] QDate firstDayOfMonth(const QDate& date);
[[nodiscard]] QDate lastDayOfMonth(const QDate& date);
[[nodiscard]] QDate firstDayOfQuarter(const QDate& date);
[[nodiscard]] QDate lastDayOfQuarter(const QDate& date);
[[nodiscard]] int dayOfYear(const QDate& date);
[[nodiscard]] int daysToEndOfYear(const QDate& date);
[[nodiscard]] int isoWeekNumber(const QDate& date, int* isoYear = nullptr);
[[nodiscard]] QString dateSpanText(const DateSpan& span);
[[nodiscard]] QString dateTimeSpanText(const DateTimeSpan& span);
[[nodiscard]] QString isoWeekText(const QDate& date);

} // namespace pac::datemath
