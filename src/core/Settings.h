#pragma once

#include <QDateTime>
#include <QObject>
#include <QSettings>
#include <QString>

namespace pac {

class Settings final : public QObject {
    Q_OBJECT

public:
    static Settings& instance();

    Settings(const Settings&) = delete;
    Settings& operator=(const Settings&) = delete;
    Settings(Settings&&) = delete;
    Settings& operator=(Settings&&) = delete;

    [[nodiscard]] QSettings& raw();
    [[nodiscard]] QString filePath() const;

    [[nodiscard]] QString themeId() const;
    [[nodiscard]] bool use24h() const;
    [[nodiscard]] bool weekStartsMonday() const;
    [[nodiscard]] bool showSecondsInTitle() const;
    [[nodiscard]] bool closeToTray() const;
    [[nodiscard]] bool minimizeToTray() const;
    [[nodiscard]] bool startMinimized() const;
    [[nodiscard]] bool showNavPane() const;
    [[nodiscard]] bool showStatusBar() const;
    [[nodiscard]] int rowDensity() const;
    [[nodiscard]] bool showGrid() const;
    [[nodiscard]] bool alternateRows() const;
    [[nodiscard]] bool showSecondsInClocks() const;
    [[nodiscard]] bool calendarShowTasks() const;
    [[nodiscard]] bool calendarShowAlarms() const;
    [[nodiscard]] bool showWeekNumbers() const;
    [[nodiscard]] int notificationSeconds() const;
    [[nodiscard]] bool preAlertsEnabled() const;
    [[nodiscard]] int catchUpSeconds() const;
    [[nodiscard]] QDateTime alarmsSuspendedUntil() const;
    void setAlarmsSuspendedUntil(const QDateTime& until);

    [[nodiscard]] QString defaultAlarmSound() const;
    [[nodiscard]] int defaultAlarmVolume() const;
    [[nodiscard]] int defaultSnoozeMinutes() const;
    [[nodiscard]] int defaultMaxSnoozes() const;
    [[nodiscard]] bool defaultLoopSound() const;
    [[nodiscard]] int defaultMaxSoundMinutes() const;
    [[nodiscard]] bool defaultAlarmPopup() const;
    [[nodiscard]] int defaultPreAlertMinutes() const;

    [[nodiscard]] int defaultTimerMinutes() const;
    [[nodiscard]] QString defaultTimerSound() const;
    [[nodiscard]] int defaultTimerVolume() const;
    [[nodiscard]] bool defaultTimerLoop() const;
    [[nodiscard]] bool defaultTimerPopup() const;

    void notifyChanged();

signals:
    void changed();

private:
    Settings();

    QSettings m_settings;
};

} // namespace pac
