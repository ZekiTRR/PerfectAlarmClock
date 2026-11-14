#pragma once

#include <Qt>
#include <QString>

#include <vector>

namespace pac::catalog {

struct AlarmIconInfo {
    QString emoji;
    QString name;
};

struct BuiltinSound {
    QString id;
    QString name;
    QString resource;
};

struct CityInfo {
    QString city;
    QString country;
    QString timeZoneId;
};

[[nodiscard]] const std::vector<AlarmIconInfo>& alarmIcons();
[[nodiscard]] const std::vector<QString>& alarmColors();
[[nodiscard]] const std::vector<BuiltinSound>& builtinSounds();
[[nodiscard]] std::vector<BuiltinSound> availableSounds();
[[nodiscard]] const std::vector<CityInfo>& cities();
[[nodiscard]] QString soundName(const QString& soundId);
[[nodiscard]] QString soundResource(const QString& soundId);
[[nodiscard]] QString priorityName(int priority);
[[nodiscard]] QString dayShort(Qt::DayOfWeek day);
[[nodiscard]] QString dayLong(Qt::DayOfWeek day);
[[nodiscard]] QString monthName(int month);
[[nodiscard]] QString monthNameGenitive(int month);

} // namespace pac::catalog
