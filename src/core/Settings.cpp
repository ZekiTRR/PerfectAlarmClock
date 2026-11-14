#include "Settings.h"

namespace pac {
namespace {

[[nodiscard]] QVariant read(const QSettings& settings, const QString& key, const QVariant& fallback)
{
    return settings.value(key, fallback);
}

} // namespace

Settings& Settings::instance()
{
    static Settings instanceObject;
    return instanceObject;
}

Settings::Settings()
    : m_settings(QSettings::IniFormat, QSettings::UserScope,
                 QStringLiteral("PerfectAlarmClock"), QStringLiteral("PerfectAlarmClock"))
{
}

QSettings& Settings::raw()
{
    return m_settings;
}

QString Settings::filePath() const
{
    return m_settings.fileName();
}

QString Settings::themeId() const
{
    return read(m_settings, QStringLiteral("appearance/theme"), QStringLiteral("office2007-blue")).toString();
}

bool Settings::use24h() const
{
    return read(m_settings, QStringLiteral("general/use24h"), true).toBool();
}

bool Settings::weekStartsMonday() const
{
    return read(m_settings, QStringLiteral("general/weekStartsMonday"), true).toBool();
}

bool Settings::showSecondsInTitle() const
{
    return read(m_settings, QStringLiteral("general/showSecondsInTitle"), true).toBool();
}

bool Settings::closeToTray() const
{
    return read(m_settings, QStringLiteral("general/closeToTray"), true).toBool();
}

bool Settings::minimizeToTray() const
{
    return read(m_settings, QStringLiteral("general/minimizeToTray"), false).toBool();
}

bool Settings::startMinimized() const
{
    return read(m_settings, QStringLiteral("general/startMinimized"), false).toBool();
}

bool Settings::showNavPane() const
{
    return read(m_settings, QStringLiteral("appearance/showNavPane"), true).toBool();
}

bool Settings::showStatusBar() const
{
    return read(m_settings, QStringLiteral("appearance/showStatusBar"), true).toBool();
}

int Settings::rowDensity() const
{
    return read(m_settings, QStringLiteral("appearance/rowDensity"), 1).toInt();
}

bool Settings::showGrid() const
{
    return read(m_settings, QStringLiteral("appearance/showGrid"), true).toBool();
}

bool Settings::alternateRows() const
{
    return read(m_settings, QStringLiteral("appearance/alternateRows"), true).toBool();
}

bool Settings::showSecondsInClocks() const
{
    return read(m_settings, QStringLiteral("appearance/showSecondsInClocks"), true).toBool();
}

bool Settings::calendarShowTasks() const
{
    return read(m_settings, QStringLiteral("calendar/showTasks"), true).toBool();
}

bool Settings::calendarShowAlarms() const
{
    return read(m_settings, QStringLiteral("calendar/showAlarms"), true).toBool();
}

bool Settings::showWeekNumbers() const
{
    return read(m_settings, QStringLiteral("calendar/weekNumbers"), false).toBool();
}

int Settings::notificationSeconds() const
{
    return read(m_settings, QStringLiteral("notifications/seconds"), 20).toInt();
}

bool Settings::preAlertsEnabled() const
{
    return read(m_settings, QStringLiteral("notifications/preAlerts"), true).toBool();
}

int Settings::catchUpSeconds() const
{
    return read(m_settings, QStringLiteral("alarms/catchUpSeconds"), 90).toInt();
}

QDateTime Settings::alarmsSuspendedUntil() const
{
    return QDateTime::fromString(
        read(m_settings, QStringLiteral("alarms/suspendedUntil"), QString()).toString(),
        Qt::ISODate);
}

void Settings::setAlarmsSuspendedUntil(const QDateTime& until)
{
    if (until.isValid() && until > QDateTime::currentDateTime()) {
        m_settings.setValue(QStringLiteral("alarms/suspendedUntil"),
                            until.toString(Qt::ISODate));
    } else {
        m_settings.remove(QStringLiteral("alarms/suspendedUntil"));
    }
    m_settings.sync();
    emit changed();
}

QString Settings::defaultAlarmSound() const
{
    return read(m_settings, QStringLiteral("alarms/sound"), QStringLiteral("builtin:chime")).toString();
}

int Settings::defaultAlarmVolume() const
{
    return read(m_settings, QStringLiteral("alarms/volume"), 80).toInt();
}

int Settings::defaultSnoozeMinutes() const
{
    return read(m_settings, QStringLiteral("alarms/snoozeMinutes"), 5).toInt();
}

int Settings::defaultMaxSnoozes() const
{
    return read(m_settings, QStringLiteral("alarms/maxSnoozes"), 3).toInt();
}

bool Settings::defaultLoopSound() const
{
    return read(m_settings, QStringLiteral("alarms/loopSound"), true).toBool();
}

int Settings::defaultMaxSoundMinutes() const
{
    return read(m_settings, QStringLiteral("alarms/maxSoundMinutes"), 5).toInt();
}

bool Settings::defaultAlarmPopup() const
{
    return read(m_settings, QStringLiteral("alarms/popup"), true).toBool();
}

int Settings::defaultPreAlertMinutes() const
{
    return read(m_settings, QStringLiteral("alarms/preAlertMinutes"), 0).toInt();
}

int Settings::defaultTimerMinutes() const
{
    return read(m_settings, QStringLiteral("timers/minutes"), 10).toInt();
}

QString Settings::defaultTimerSound() const
{
    return read(m_settings, QStringLiteral("timers/sound"), QStringLiteral("builtin:digital")).toString();
}

int Settings::defaultTimerVolume() const
{
    return read(m_settings, QStringLiteral("timers/volume"), 80).toInt();
}

bool Settings::defaultTimerLoop() const
{
    return read(m_settings, QStringLiteral("timers/loopSound"), false).toBool();
}

bool Settings::defaultTimerPopup() const
{
    return read(m_settings, QStringLiteral("timers/popup"), true).toBool();
}

void Settings::notifyChanged()
{
    m_settings.sync();
    emit changed();
}

} // namespace pac
