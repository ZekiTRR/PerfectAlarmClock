#include "Store.h"

#include "Catalog.h"
#include "Schedule.h"
#include "Platform.h"
#include "Settings.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>

#include <algorithm>

namespace pac {
namespace {

constexpr int kFormatVersion = 2;

[[nodiscard]] QJsonArray daysToJson(const QList<Qt::DayOfWeek>& days)
{
    QJsonArray array;
    for (const auto day : days) {
        array.append(static_cast<int>(day));
    }
    return array;
}

[[nodiscard]] QList<Qt::DayOfWeek> daysFromJson(const QJsonArray& array)
{
    QList<Qt::DayOfWeek> days;
    for (const auto& value : array) {
        const int day = value.toInt();
        if (day >= Qt::Monday && day <= Qt::Sunday) {
            const auto weekday = static_cast<Qt::DayOfWeek>(day);
            if (!days.contains(weekday)) {
                days.append(weekday);
            }
        }
    }
    return days;
}

[[nodiscard]] QJsonArray datesToJson(const QList<QDate>& dates)
{
    QJsonArray array;
    for (const QDate& date : dates) {
        if (date.isValid()) {
            array.append(date.toString(Qt::ISODate));
        }
    }
    return array;
}

[[nodiscard]] QList<QDate> datesFromJson(const QJsonArray& array)
{
    QList<QDate> dates;
    for (const auto& value : array) {
        const QDate date = QDate::fromString(value.toString(), Qt::ISODate);
        if (date.isValid() && !dates.contains(date)) {
            dates.append(date);
        }
    }
    return dates;
}

[[nodiscard]] bool knownRepeat(int value)
{
    return value >= static_cast<int>(RepeatKind::Once)
           && value <= static_cast<int>(RepeatKind::LastWeekdayOfMonth);
}

[[nodiscard]] QJsonObject scheduleToJson(const AlarmSchedule& schedule)
{
    QJsonObject object;
    object.insert(QStringLiteral("time"), schedule.time.toString(QStringLiteral("HH:mm:ss")));
    object.insert(QStringLiteral("repeat"), static_cast<int>(schedule.repeat));
    object.insert(QStringLiteral("days"), daysToJson(schedule.days));
    object.insert(QStringLiteral("monthDay"), schedule.monthDay);
    object.insert(QStringLiteral("monthWeek"), schedule.monthWeek);
    object.insert(QStringLiteral("intervalDays"), schedule.intervalDays);
    object.insert(QStringLiteral("intervalWeeks"), schedule.intervalWeeks);
    object.insert(QStringLiteral("intervalMinutes"), schedule.intervalMinutes);
    object.insert(QStringLiteral("activeFrom"), schedule.activeFrom.toString(QStringLiteral("HH:mm:ss")));
    object.insert(QStringLiteral("activeTo"), schedule.activeTo.toString(QStringLiteral("HH:mm:ss")));
    object.insert(QStringLiteral("anchorDate"),
                  schedule.anchorDate.isValid() ? schedule.anchorDate.toString(Qt::ISODate)
                                                : QString());
    object.insert(QStringLiteral("enabled"), schedule.enabled);
    return object;
}

[[nodiscard]] AlarmSchedule scheduleFromJson(const QJsonObject& object)
{
    AlarmSchedule schedule;
    const QTime time = QTime::fromString(
        object.value(QStringLiteral("time")).toString(), QStringLiteral("HH:mm:ss"));
    if (time.isValid()) {
        schedule.time = time;
    }
    const int repeat = object.value(QStringLiteral("repeat")).toInt(static_cast<int>(RepeatKind::Daily));
    schedule.repeat = knownRepeat(repeat) ? static_cast<RepeatKind>(repeat) : RepeatKind::Daily;
    schedule.days = daysFromJson(object.value(QStringLiteral("days")).toArray());
    schedule.monthDay = std::clamp(object.value(QStringLiteral("monthDay")).toInt(1), 0, 31);
    schedule.monthWeek = std::clamp(object.value(QStringLiteral("monthWeek")).toInt(1), -1, 4);
    schedule.intervalDays = std::clamp(object.value(QStringLiteral("intervalDays")).toInt(2), 1, 365);
    schedule.intervalWeeks = std::clamp(object.value(QStringLiteral("intervalWeeks")).toInt(2), 1, 52);
    schedule.intervalMinutes = std::clamp(object.value(QStringLiteral("intervalMinutes")).toInt(30),
                                          1, 10'080);
    schedule.activeFrom = QTime::fromString(
        object.value(QStringLiteral("activeFrom")).toString(), QStringLiteral("HH:mm:ss"));
    schedule.activeTo = QTime::fromString(
        object.value(QStringLiteral("activeTo")).toString(), QStringLiteral("HH:mm:ss"));
    if (!schedule.activeFrom.isValid()) {
        schedule.activeFrom = QTime(0, 0);
    }
    if (!schedule.activeTo.isValid()) {
        schedule.activeTo = QTime(23, 59);
    }
    schedule.anchorDate = QDate::fromString(
        object.value(QStringLiteral("anchorDate")).toString(), Qt::ISODate);
    schedule.enabled = object.value(QStringLiteral("enabled")).toBool(true);
    return schedule;
}

[[nodiscard]] QString dateTimeToString(const QDateTime& value)
{
    return value.isValid() ? value.toString(Qt::ISODateWithMs) : QString();
}

[[nodiscard]] QDateTime dateTimeFromString(const QString& value)
{
    return value.isEmpty() ? QDateTime() : QDateTime::fromString(value, Qt::ISODateWithMs);
}

[[nodiscard]] QJsonObject alarmToJson(const Alarm& alarm)
{
    QJsonObject object;
    object.insert(QStringLiteral("id"), alarm.id.toString(QUuid::WithoutBraces));
    object.insert(QStringLiteral("label"), alarm.label);
    object.insert(QStringLiteral("group"), alarm.group);
    object.insert(QStringLiteral("note"), alarm.note);
    object.insert(QStringLiteral("enabled"), alarm.enabled);
    object.insert(QStringLiteral("time"), alarm.time.toString(QStringLiteral("HH:mm:ss")));
    object.insert(QStringLiteral("onceDate"), alarm.onceDate.isValid() ? alarm.onceDate.toString(Qt::ISODate) : QString());
    object.insert(QStringLiteral("repeat"), static_cast<int>(alarm.repeat));
    object.insert(QStringLiteral("days"), daysToJson(alarm.days));
    object.insert(QStringLiteral("monthDay"), alarm.monthDay);
    object.insert(QStringLiteral("monthWeek"), alarm.monthWeek);
    object.insert(QStringLiteral("intervalMinutes"), alarm.intervalMinutes);
    object.insert(QStringLiteral("intervalDays"), alarm.intervalDays);
    object.insert(QStringLiteral("intervalWeeks"), alarm.intervalWeeks);
    object.insert(QStringLiteral("seriesStart"), alarm.seriesStart.isValid()
                                                     ? alarm.seriesStart.toString(Qt::ISODate)
                                                     : QString());
    object.insert(QStringLiteral("timeZoneId"), alarm.timeZoneId);
    object.insert(QStringLiteral("holidayPolicy"), static_cast<int>(alarm.holidayPolicy));
    object.insert(QStringLiteral("exceptions"), datesToJson(alarm.exceptions));
    object.insert(QStringLiteral("randomWindow"), alarm.randomWindow);
    object.insert(QStringLiteral("randomBeforeMinutes"), alarm.randomBeforeMinutes);
    object.insert(QStringLiteral("randomAfterMinutes"), alarm.randomAfterMinutes);
    QJsonArray schedules;
    for (const auto& schedule : alarm.extraSchedules) {
        schedules.append(scheduleToJson(schedule));
    }
    object.insert(QStringLiteral("extraSchedules"), schedules);
    object.insert(QStringLiteral("activeFrom"), alarm.activeFrom.toString(QStringLiteral("HH:mm:ss")));
    object.insert(QStringLiteral("activeTo"), alarm.activeTo.toString(QStringLiteral("HH:mm:ss")));
    object.insert(QStringLiteral("activeFromDate"), alarm.activeFromDate.isValid() ? alarm.activeFromDate.toString(Qt::ISODate) : QString());
    object.insert(QStringLiteral("activeToDate"), alarm.activeToDate.isValid() ? alarm.activeToDate.toString(Qt::ISODate) : QString());
    object.insert(QStringLiteral("snoozeUntil"), dateTimeToString(alarm.snoozeUntil));
    object.insert(QStringLiteral("snoozeCount"), alarm.snoozeCount);
    object.insert(QStringLiteral("lastTriggered"), dateTimeToString(alarm.lastTriggered));
    object.insert(QStringLiteral("intervalAnchor"), dateTimeToString(alarm.intervalAnchor));
    object.insert(QStringLiteral("icon"), alarm.icon);
    object.insert(QStringLiteral("color"), alarm.color);
    object.insert(QStringLiteral("soundEnabled"), alarm.soundEnabled);
    object.insert(QStringLiteral("soundId"), alarm.soundId);
    object.insert(QStringLiteral("volume"), alarm.volume);
    object.insert(QStringLiteral("loopSound"), alarm.loopSound);
    object.insert(QStringLiteral("fadeIn"), alarm.fadeIn);
    object.insert(QStringLiteral("fadeInSeconds"), alarm.fadeInSeconds);
    object.insert(QStringLiteral("fadeOutSeconds"), alarm.fadeOutSeconds);
    object.insert(QStringLiteral("randomSound"), alarm.randomSound);
    object.insert(QStringLiteral("repeatSoundId"), alarm.repeatSoundId);
    object.insert(QStringLiteral("maxSoundMinutes"), alarm.maxSoundMinutes);
    object.insert(QStringLiteral("snoozeEnabled"), alarm.snoozeEnabled);
    object.insert(QStringLiteral("snoozeMinutes"), alarm.snoozeMinutes);
    object.insert(QStringLiteral("snoozeMinutesAlt"), alarm.snoozeMinutesAlt);
    object.insert(QStringLiteral("snoozeDecrease"), alarm.snoozeDecrease);
    object.insert(QStringLiteral("snoozeDecreaseStep"), alarm.snoozeDecreaseStep);
    object.insert(QStringLiteral("snoozeVolumeIncrease"), alarm.snoozeVolumeIncrease);
    object.insert(QStringLiteral("afterLastSnooze"), static_cast<int>(alarm.afterLastSnooze));
    object.insert(QStringLiteral("maxSnoozes"), alarm.maxSnoozes);
    object.insert(QStringLiteral("popupEnabled"), alarm.popupEnabled);
    object.insert(QStringLiteral("preAlertMinutes"), alarm.preAlertMinutes);
    object.insert(QStringLiteral("postSummary"), alarm.postSummary);
    object.insert(QStringLiteral("postTasks"), alarm.postTasks);
    object.insert(QStringLiteral("postMessage"), alarm.postMessage);
    object.insert(QStringLiteral("postOpen"), alarm.postOpen);
    object.insert(QStringLiteral("disableAfterDismiss"), alarm.disableAfterDismiss);
    object.insert(QStringLiteral("backupEnabled"), alarm.backupEnabled);
    object.insert(QStringLiteral("backupAfterMinutes"), alarm.backupAfterMinutes);
    object.insert(QStringLiteral("persistent"), alarm.persistent);
    return object;
}

[[nodiscard]] Alarm alarmFromJson(const QJsonObject& object)
{
    Alarm alarm;
    const QString id = object.value(QStringLiteral("id")).toString();
    alarm.id = id.isEmpty() ? QUuid::createUuid() : QUuid::fromString(id);
    if (alarm.id.isNull()) {
        alarm.id = QUuid::createUuid();
    }
    alarm.label = object.value(QStringLiteral("label")).toString();
    alarm.group = object.value(QStringLiteral("group")).toString(QStringLiteral("Общие"));
    alarm.note = object.value(QStringLiteral("note")).toString();
    alarm.enabled = object.value(QStringLiteral("enabled")).toBool(false);
    alarm.time = QTime::fromString(object.value(QStringLiteral("time")).toString(), QStringLiteral("HH:mm:ss"));
    if (!alarm.time.isValid()) {
        alarm.time = QTime(7, 0);
    }
    alarm.onceDate = QDate::fromString(object.value(QStringLiteral("onceDate")).toString(), Qt::ISODate);
    const int repeat = object.value(QStringLiteral("repeat")).toInt(static_cast<int>(RepeatKind::Daily));
    alarm.repeat = knownRepeat(repeat) ? static_cast<RepeatKind>(repeat) : RepeatKind::Daily;
    alarm.days = daysFromJson(object.value(QStringLiteral("days")).toArray());
    alarm.monthDay = std::clamp(object.value(QStringLiteral("monthDay")).toInt(1), 0, 31);
    alarm.monthWeek = std::clamp(object.value(QStringLiteral("monthWeek")).toInt(1), -1, 4);
    alarm.intervalMinutes = std::clamp(object.value(QStringLiteral("intervalMinutes")).toInt(30), 1, 10'080);
    alarm.intervalDays = std::clamp(object.value(QStringLiteral("intervalDays")).toInt(2), 1, 365);
    alarm.intervalWeeks = std::clamp(object.value(QStringLiteral("intervalWeeks")).toInt(2), 1, 52);
    alarm.seriesStart = QDate::fromString(
        object.value(QStringLiteral("seriesStart")).toString(), Qt::ISODate);
    alarm.timeZoneId = object.value(QStringLiteral("timeZoneId")).toString();
    const int holidayPolicy = object.value(QStringLiteral("holidayPolicy")).toInt(0);
    alarm.holidayPolicy = holidayPolicy >= static_cast<int>(HolidayPolicy::Ignore)
                                  && holidayPolicy <= static_cast<int>(HolidayPolicy::ShiftToNextWorkday)
                              ? static_cast<HolidayPolicy>(holidayPolicy)
                              : HolidayPolicy::Ignore;
    alarm.exceptions = datesFromJson(object.value(QStringLiteral("exceptions")).toArray());
    alarm.randomWindow = object.value(QStringLiteral("randomWindow")).toBool(false);
    alarm.randomBeforeMinutes = std::clamp(
        object.value(QStringLiteral("randomBeforeMinutes")).toInt(15), 0, 720);
    alarm.randomAfterMinutes = std::clamp(
        object.value(QStringLiteral("randomAfterMinutes")).toInt(15), 0, 720);
    for (const auto& value : object.value(QStringLiteral("extraSchedules")).toArray()) {
        alarm.extraSchedules.push_back(scheduleFromJson(value.toObject()));
    }
    alarm.activeFrom = QTime::fromString(object.value(QStringLiteral("activeFrom")).toString(), QStringLiteral("HH:mm:ss"));
    alarm.activeTo = QTime::fromString(object.value(QStringLiteral("activeTo")).toString(), QStringLiteral("HH:mm:ss"));
    if (!alarm.activeFrom.isValid()) {
        alarm.activeFrom = QTime(0, 0);
    }
    if (!alarm.activeTo.isValid()) {
        alarm.activeTo = QTime(23, 59);
    }
    alarm.activeFromDate = QDate::fromString(object.value(QStringLiteral("activeFromDate")).toString(), Qt::ISODate);
    alarm.activeToDate = QDate::fromString(object.value(QStringLiteral("activeToDate")).toString(), Qt::ISODate);
    alarm.snoozeUntil = dateTimeFromString(object.value(QStringLiteral("snoozeUntil")).toString());
    alarm.snoozeCount = std::max(0, object.value(QStringLiteral("snoozeCount")).toInt(0));
    alarm.lastTriggered = dateTimeFromString(object.value(QStringLiteral("lastTriggered")).toString());
    alarm.intervalAnchor = dateTimeFromString(object.value(QStringLiteral("intervalAnchor")).toString());
    const QString icon = object.value(QStringLiteral("icon")).toString();
    if (!icon.isEmpty()) {
        alarm.icon = icon;
    }
    const QString color = object.value(QStringLiteral("color")).toString();
    if (!color.isEmpty()) {
        alarm.color = color;
    }
    alarm.soundEnabled = object.value(QStringLiteral("soundEnabled")).toBool(true);
    alarm.soundId = object.value(QStringLiteral("soundId")).toString(QStringLiteral("builtin:chime"));
    alarm.volume = std::clamp(object.value(QStringLiteral("volume")).toInt(80), 0, 100);
    alarm.loopSound = object.value(QStringLiteral("loopSound")).toBool(true);
    alarm.fadeIn = object.value(QStringLiteral("fadeIn")).toBool(false);
    alarm.fadeInSeconds = std::clamp(object.value(QStringLiteral("fadeInSeconds")).toInt(6), 1, 120);
    alarm.fadeOutSeconds = std::clamp(object.value(QStringLiteral("fadeOutSeconds")).toInt(2), 0, 60);
    alarm.randomSound = object.value(QStringLiteral("randomSound")).toBool(false);
    alarm.repeatSoundId = object.value(QStringLiteral("repeatSoundId")).toString();
    alarm.maxSoundMinutes = std::clamp(object.value(QStringLiteral("maxSoundMinutes")).toInt(5), 1, 120);
    alarm.snoozeEnabled = object.value(QStringLiteral("snoozeEnabled")).toBool(true);
    alarm.snoozeMinutes = std::clamp(object.value(QStringLiteral("snoozeMinutes")).toInt(5), 1, 180);
    alarm.snoozeMinutesAlt = std::clamp(object.value(QStringLiteral("snoozeMinutesAlt")).toInt(10), 1, 180);
    alarm.snoozeDecrease = object.value(QStringLiteral("snoozeDecrease")).toBool(false);
    alarm.snoozeDecreaseStep = std::clamp(object.value(QStringLiteral("snoozeDecreaseStep")).toInt(1), 1, 60);
    alarm.snoozeVolumeIncrease = std::clamp(object.value(QStringLiteral("snoozeVolumeIncrease")).toInt(5), 0, 100);
    const int snoozeEnd = object.value(QStringLiteral("afterLastSnooze")).toInt(0);
    alarm.afterLastSnooze = snoozeEnd == static_cast<int>(SnoozeEndAction::DisableAlarm)
                                ? SnoozeEndAction::DisableAlarm
                                : SnoozeEndAction::Stop;
    alarm.maxSnoozes = std::clamp(object.value(QStringLiteral("maxSnoozes")).toInt(3), 0, 20);
    alarm.popupEnabled = object.value(QStringLiteral("popupEnabled")).toBool(true);
    alarm.preAlertMinutes = std::clamp(object.value(QStringLiteral("preAlertMinutes")).toInt(0), 0, 1440);
    alarm.postSummary = object.value(QStringLiteral("postSummary")).toBool(false);
    alarm.postTasks = object.value(QStringLiteral("postTasks")).toBool(false);
    alarm.postMessage = object.value(QStringLiteral("postMessage")).toString();
    alarm.postOpen = object.value(QStringLiteral("postOpen")).toString();
    alarm.disableAfterDismiss = object.value(QStringLiteral("disableAfterDismiss")).toBool(false);
    alarm.backupEnabled = object.value(QStringLiteral("backupEnabled")).toBool(false);
    alarm.backupAfterMinutes = std::clamp(
        object.value(QStringLiteral("backupAfterMinutes")).toInt(5), 1, 120);
    alarm.persistent = object.value(QStringLiteral("persistent")).toBool(false);
    return alarm;
}

[[nodiscard]] QJsonObject timerStageToJson(const TimerStage& stage)
{
    QJsonObject object;
    object.insert(QStringLiteral("id"), stage.id.toString(QUuid::WithoutBraces));
    object.insert(QStringLiteral("name"), stage.name);
    object.insert(QStringLiteral("durationSec"), stage.durationSec);
    object.insert(QStringLiteral("kind"), static_cast<int>(stage.kind));
    object.insert(QStringLiteral("color"), stage.color);
    object.insert(QStringLiteral("soundId"), stage.soundId);
    object.insert(QStringLiteral("message"), stage.message);
    return object;
}

[[nodiscard]] TimerStage timerStageFromJson(const QJsonObject& object)
{
    TimerStage stage;
    const QString id = object.value(QStringLiteral("id")).toString();
    if (!id.isEmpty()) {
        const QUuid parsed = QUuid::fromString(id);
        if (!parsed.isNull()) {
            stage.id = parsed;
        }
    }
    stage.name = object.value(QStringLiteral("name")).toString();
    stage.durationSec = std::clamp(object.value(QStringLiteral("durationSec")).toInt(300),
                                   1, 7 * 86'400);
    const int kind = object.value(QStringLiteral("kind")).toInt(static_cast<int>(TimerStageKind::Work));
    stage.kind = kind >= static_cast<int>(TimerStageKind::Prepare)
                         && kind <= static_cast<int>(TimerStageKind::Custom)
                     ? static_cast<TimerStageKind>(kind)
                     : TimerStageKind::Work;
    const QString color = object.value(QStringLiteral("color")).toString();
    if (!color.isEmpty()) {
        stage.color = color;
    }
    stage.soundId = object.value(QStringLiteral("soundId")).toString();
    stage.message = object.value(QStringLiteral("message")).toString();
    return stage;
}

[[nodiscard]] QJsonObject timerToJson(const CountdownTimer& timer)
{
    QJsonObject object;
    object.insert(QStringLiteral("id"), timer.id.toString(QUuid::WithoutBraces));
    object.insert(QStringLiteral("label"), timer.label);
    object.insert(QStringLiteral("durationSec"), timer.durationSec);
    object.insert(QStringLiteral("remainingMs"), static_cast<double>(timer.remainingMs));
    object.insert(QStringLiteral("endAt"), dateTimeToString(timer.endAt));
    object.insert(QStringLiteral("state"), static_cast<int>(timer.state));
    object.insert(QStringLiteral("mode"), static_cast<int>(timer.mode));
    object.insert(QStringLiteral("repeatCycles"), timer.repeatCycles);
    object.insert(QStringLiteral("currentStage"), timer.currentStage);
    object.insert(QStringLiteral("completedCycles"), timer.completedCycles);
    object.insert(QStringLiteral("targetAt"), dateTimeToString(timer.targetAt));
    object.insert(QStringLiteral("randomMinSec"), timer.randomMinSec);
    object.insert(QStringLiteral("randomMaxSec"), timer.randomMaxSec);
    QJsonArray stages;
    for (const auto& stage : timer.stages) {
        stages.append(timerStageToJson(stage));
    }
    object.insert(QStringLiteral("stages"), stages);
    object.insert(QStringLiteral("soundEnabled"), timer.soundEnabled);
    object.insert(QStringLiteral("soundId"), timer.soundId);
    object.insert(QStringLiteral("volume"), timer.volume);
    object.insert(QStringLiteral("loopSound"), timer.loopSound);
    object.insert(QStringLiteral("popupEnabled"), timer.popupEnabled);
    object.insert(QStringLiteral("autoRepeat"), timer.autoRepeat);
    object.insert(QStringLiteral("color"), timer.color);
    return object;
}

[[nodiscard]] CountdownTimer timerFromJson(const QJsonObject& object)
{
    CountdownTimer timer;
    const QString id = object.value(QStringLiteral("id")).toString();
    timer.id = id.isEmpty() ? QUuid::createUuid() : QUuid::fromString(id);
    if (timer.id.isNull()) {
        timer.id = QUuid::createUuid();
    }
    timer.label = object.value(QStringLiteral("label")).toString();
    timer.durationSec = std::clamp(object.value(QStringLiteral("durationSec")).toInt(600), 1, 7 * 86'400);
    timer.remainingMs = std::clamp<qint64>(static_cast<qint64>(object.value(QStringLiteral("remainingMs")).toDouble(timer.durationSec * 1000.0)),
                                          0, static_cast<qint64>(timer.durationSec) * 1000);
    timer.endAt = dateTimeFromString(object.value(QStringLiteral("endAt")).toString());
    const int state = object.value(QStringLiteral("state")).toInt(static_cast<int>(TimerState::Stopped));
    timer.state = (state >= static_cast<int>(TimerState::Stopped)
                   && state <= static_cast<int>(TimerState::Finished))
                      ? static_cast<TimerState>(state)
                      : TimerState::Stopped;
    const int mode = object.value(QStringLiteral("mode")).toInt(static_cast<int>(TimerMode::Countdown));
    timer.mode = mode >= static_cast<int>(TimerMode::Countdown)
                         && mode <= static_cast<int>(TimerMode::Target)
                     ? static_cast<TimerMode>(mode)
                     : TimerMode::Countdown;
    for (const auto& value : object.value(QStringLiteral("stages")).toArray()) {
        timer.stages.push_back(timerStageFromJson(value.toObject()));
    }
    timer.repeatCycles = std::clamp(object.value(QStringLiteral("repeatCycles")).toInt(1), 1, 999);
    timer.currentStage = std::clamp(object.value(QStringLiteral("currentStage")).toInt(0), 0,
                                    std::max(0, static_cast<int>(timer.stages.size()) - 1));
    timer.completedCycles = std::clamp(object.value(QStringLiteral("completedCycles")).toInt(0),
                                       0, timer.repeatCycles);
    timer.targetAt = dateTimeFromString(object.value(QStringLiteral("targetAt")).toString());
    timer.randomMinSec = std::clamp(object.value(QStringLiteral("randomMinSec")).toInt(60),
                                    1, 7 * 86'400);
    timer.randomMaxSec = std::clamp(object.value(QStringLiteral("randomMaxSec")).toInt(300),
                                    1, 7 * 86'400);
    timer.soundEnabled = object.value(QStringLiteral("soundEnabled")).toBool(true);
    timer.soundId = object.value(QStringLiteral("soundId")).toString(QStringLiteral("builtin:digital"));
    timer.volume = std::clamp(object.value(QStringLiteral("volume")).toInt(80), 0, 100);
    timer.loopSound = object.value(QStringLiteral("loopSound")).toBool(false);
    timer.popupEnabled = object.value(QStringLiteral("popupEnabled")).toBool(true);
    timer.autoRepeat = object.value(QStringLiteral("autoRepeat")).toBool(false);
    const QString color = object.value(QStringLiteral("color")).toString();
    if (!color.isEmpty()) {
        timer.color = color;
    }
    return timer;
}

[[nodiscard]] QJsonObject todoToJson(const Todo& todo)
{
    QJsonObject object;
    object.insert(QStringLiteral("id"), todo.id.toString(QUuid::WithoutBraces));
    object.insert(QStringLiteral("title"), todo.title);
    object.insert(QStringLiteral("notes"), todo.notes);
    object.insert(QStringLiteral("hasDue"), todo.hasDue);
    object.insert(QStringLiteral("dueDate"), todo.dueDate.isValid() ? todo.dueDate.toString(Qt::ISODate) : QString());
    object.insert(QStringLiteral("dueTime"), todo.dueTime.toString(QStringLiteral("HH:mm:ss")));
    object.insert(QStringLiteral("priority"), todo.priority);
    object.insert(QStringLiteral("done"), todo.done);
    object.insert(QStringLiteral("completedAt"), dateTimeToString(todo.completedAt));
    return object;
}

[[nodiscard]] Todo todoFromJson(const QJsonObject& object)
{
    Todo todo;
    const QString id = object.value(QStringLiteral("id")).toString();
    todo.id = id.isEmpty() ? QUuid::createUuid() : QUuid::fromString(id);
    if (todo.id.isNull()) {
        todo.id = QUuid::createUuid();
    }
    todo.title = object.value(QStringLiteral("title")).toString();
    todo.notes = object.value(QStringLiteral("notes")).toString();
    todo.hasDue = object.value(QStringLiteral("hasDue")).toBool(false);
    todo.dueDate = QDate::fromString(object.value(QStringLiteral("dueDate")).toString(), Qt::ISODate);
    todo.dueTime = QTime::fromString(object.value(QStringLiteral("dueTime")).toString(), QStringLiteral("HH:mm:ss"));
    if (!todo.dueTime.isValid()) {
        todo.dueTime = QTime(9, 0);
    }
    todo.priority = std::clamp(object.value(QStringLiteral("priority")).toInt(1), 0, 2);
    todo.done = object.value(QStringLiteral("done")).toBool(false);
    todo.completedAt = dateTimeFromString(object.value(QStringLiteral("completedAt")).toString());
    return todo;
}

[[nodiscard]] QJsonObject clockToJson(const WorldClock& clock)
{
    QJsonObject object;
    object.insert(QStringLiteral("id"), clock.id.toString(QUuid::WithoutBraces));
    object.insert(QStringLiteral("city"), clock.city);
    object.insert(QStringLiteral("country"), clock.country);
    object.insert(QStringLiteral("timeZoneId"), clock.timeZoneId);
    object.insert(QStringLiteral("showDate"), clock.showDate);
    object.insert(QStringLiteral("showDifference"), clock.showDifference);
    return object;
}

[[nodiscard]] WorldClock clockFromJson(const QJsonObject& object)
{
    WorldClock clock;
    const QString id = object.value(QStringLiteral("id")).toString();
    clock.id = id.isEmpty() ? QUuid::createUuid() : QUuid::fromString(id);
    if (clock.id.isNull()) {
        clock.id = QUuid::createUuid();
    }
    clock.city = object.value(QStringLiteral("city")).toString();
    clock.country = object.value(QStringLiteral("country")).toString();
    clock.timeZoneId = object.value(QStringLiteral("timeZoneId")).toString();
    clock.showDate = object.value(QStringLiteral("showDate")).toBool(true);
    clock.showDifference = object.value(QStringLiteral("showDifference")).toBool(true);
    return clock;
}

[[nodiscard]] QJsonObject pomodoroToJson(const PomodoroSettings& settings)
{
    QJsonObject object;
    object.insert(QStringLiteral("workMinutes"), settings.workMinutes);
    object.insert(QStringLiteral("shortBreakMinutes"), settings.shortBreakMinutes);
    object.insert(QStringLiteral("longBreakMinutes"), settings.longBreakMinutes);
    object.insert(QStringLiteral("longBreakEvery"), settings.longBreakEvery);
    object.insert(QStringLiteral("autoStartBreaks"), settings.autoStartBreaks);
    object.insert(QStringLiteral("autoStartWork"), settings.autoStartWork);
    object.insert(QStringLiteral("soundEnabled"), settings.soundEnabled);
    object.insert(QStringLiteral("soundId"), settings.soundId);
    object.insert(QStringLiteral("volume"), settings.volume);
    object.insert(QStringLiteral("notifications"), settings.notifications);
    object.insert(QStringLiteral("dailyGoal"), settings.dailyGoal);
    return object;
}

[[nodiscard]] PomodoroSettings pomodoroFromJson(const QJsonObject& object)
{
    PomodoroSettings settings;
    settings.workMinutes = std::clamp(object.value(QStringLiteral("workMinutes")).toInt(25), 1, 180);
    settings.shortBreakMinutes = std::clamp(object.value(QStringLiteral("shortBreakMinutes")).toInt(5), 1, 60);
    settings.longBreakMinutes = std::clamp(object.value(QStringLiteral("longBreakMinutes")).toInt(15), 1, 120);
    settings.longBreakEvery = std::clamp(object.value(QStringLiteral("longBreakEvery")).toInt(4), 2, 12);
    settings.autoStartBreaks = object.value(QStringLiteral("autoStartBreaks")).toBool(true);
    settings.autoStartWork = object.value(QStringLiteral("autoStartWork")).toBool(false);
    settings.soundEnabled = object.value(QStringLiteral("soundEnabled")).toBool(true);
    settings.soundId = object.value(QStringLiteral("soundId")).toString(QStringLiteral("builtin:gentle"));
    settings.volume = std::clamp(object.value(QStringLiteral("volume")).toInt(70), 0, 100);
    settings.notifications = object.value(QStringLiteral("notifications")).toBool(true);
    settings.dailyGoal = std::clamp(object.value(QStringLiteral("dailyGoal")).toInt(8), 1, 30);
    return settings;
}

[[nodiscard]] QJsonObject statsToJson(const PomodoroStats& stats)
{
    QJsonObject object;
    object.insert(QStringLiteral("day"), stats.day.isValid() ? stats.day.toString(Qt::ISODate) : QString());
    object.insert(QStringLiteral("completedToday"), stats.completedToday);
    object.insert(QStringLiteral("focusMinutesToday"), stats.focusMinutesToday);
    object.insert(QStringLiteral("currentStreak"), stats.currentStreak);
    return object;
}

[[nodiscard]] PomodoroStats statsFromJson(const QJsonObject& object)
{
    PomodoroStats stats;
    stats.day = QDate::fromString(object.value(QStringLiteral("day")).toString(), Qt::ISODate);
    stats.completedToday = std::max(0, object.value(QStringLiteral("completedToday")).toInt(0));
    stats.focusMinutesToday = std::max(0, object.value(QStringLiteral("focusMinutesToday")).toInt(0));
    stats.currentStreak = std::max(0, object.value(QStringLiteral("currentStreak")).toInt(0));
    return stats;
}

} // namespace

Store::Store(QObject* parent)
    : QObject(parent)
{
    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(400);
    connect(&m_saveTimer, &QTimer::timeout, this, &Store::saveNow);
}

Store::~Store()
{
    if (m_saveTimer.isActive()) {
        saveNow();
    }
}

AppData& Store::data() noexcept
{
    return m_data;
}

const AppData& Store::data() const noexcept
{
    return m_data;
}

QString Store::dataFilePath() const
{
    return platform::dataDirectory() + QStringLiteral("/data.json");
}

void Store::load()
{
    m_data = AppData();
    QFile file(dataFilePath());
    if (!file.exists()) {
        seedDemoData();
        seedDefaultClocks();
        normalize();
        m_loaded = true;
        saveNow();
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        seedDemoData();
        seedDefaultClocks();
        normalize();
        m_loaded = true;
        return;
    }

    QJsonParseError error{};
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        file.close();
        const QString backup = dataFilePath()
                               + QStringLiteral(".corrupt-")
                               + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss"));
        QFile::copy(dataFilePath(), backup);
        seedDemoData();
        seedDefaultClocks();
        normalize();
        m_loaded = true;
        saveNow();
        return;
    }

    const QJsonObject root = document.object();
    const QJsonArray groups = root.value(QStringLiteral("groups")).toArray();
    for (const auto& value : groups) {
        const QJsonObject groupObject = value.toObject();
        const QString name = groupObject.value(QStringLiteral("name")).toString();
        if (!name.isEmpty()) {
            m_data.groups.push_back({name, groupObject.value(QStringLiteral("collapsed")).toBool(false)});
        }
    }

    const QJsonArray alarms = root.value(QStringLiteral("alarms")).toArray();
    for (const auto& value : alarms) {
        m_data.alarms.push_back(alarmFromJson(value.toObject()));
    }
    for (const auto& value : root.value(QStringLiteral("timers")).toArray()) {
        m_data.timers.push_back(timerFromJson(value.toObject()));
    }
    for (const auto& value : root.value(QStringLiteral("todos")).toArray()) {
        m_data.todos.push_back(todoFromJson(value.toObject()));
    }
    for (const auto& value : root.value(QStringLiteral("clocks")).toArray()) {
        m_data.clocks.push_back(clockFromJson(value.toObject()));
    }
    m_data.nonWorkingSeeded = root.value(QStringLiteral("nonWorkingSeeded")).toBool(false);
    for (const auto& value : root.value(QStringLiteral("nonWorkingDays")).toArray()) {
        const QJsonObject object = value.toObject();
        const QDate date = QDate::fromString(object.value(QStringLiteral("date")).toString(),
                                             Qt::ISODate);
        if (date.isValid()) {
            m_data.nonWorkingDays.push_back(
                {date, object.value(QStringLiteral("name")).toString()});
        }
    }
    m_data.pomodoro = pomodoroFromJson(root.value(QStringLiteral("pomodoro")).toObject());
    m_data.pomodoroStats = statsFromJson(root.value(QStringLiteral("pomodoroStats")).toObject());

    if (m_data.pomodoroStats.day != QDate::currentDate()) {
        m_data.pomodoroStats.day = QDate::currentDate();
        m_data.pomodoroStats.completedToday = 0;
        m_data.pomodoroStats.focusMinutesToday = 0;
    }

    seedDefaultClocks();
    normalize();
    m_loaded = true;
}

void Store::saveNow()
{
    m_saveTimer.stop();
    if (!m_loaded) {
        return;
    }

    QJsonObject root;
    root.insert(QStringLiteral("version"), kFormatVersion);

    QJsonArray groups;
    for (const auto& group : m_data.groups) {
        QJsonObject object;
        object.insert(QStringLiteral("name"), group.name);
        object.insert(QStringLiteral("collapsed"), group.collapsed);
        groups.append(object);
    }
    root.insert(QStringLiteral("groups"), groups);

    QJsonArray alarms;
    for (const auto& alarm : m_data.alarms) {
        alarms.append(alarmToJson(alarm));
    }
    root.insert(QStringLiteral("alarms"), alarms);

    QJsonArray timers;
    for (const auto& timer : m_data.timers) {
        timers.append(timerToJson(timer));
    }
    root.insert(QStringLiteral("timers"), timers);

    QJsonArray todos;
    for (const auto& todo : m_data.todos) {
        todos.append(todoToJson(todo));
    }
    root.insert(QStringLiteral("todos"), todos);

    QJsonArray clocks;
    for (const auto& clock : m_data.clocks) {
        clocks.append(clockToJson(clock));
    }
    root.insert(QStringLiteral("clocks"), clocks);

    QJsonArray nonWorking;
    for (const auto& day : m_data.nonWorkingDays) {
        QJsonObject object;
        object.insert(QStringLiteral("date"), day.date.toString(Qt::ISODate));
        object.insert(QStringLiteral("name"), day.name);
        nonWorking.append(object);
    }
    root.insert(QStringLiteral("nonWorkingDays"), nonWorking);
    root.insert(QStringLiteral("nonWorkingSeeded"), m_data.nonWorkingSeeded);

    root.insert(QStringLiteral("pomodoro"), pomodoroToJson(m_data.pomodoro));
    root.insert(QStringLiteral("pomodoroStats"), statsToJson(m_data.pomodoroStats));

    QSaveFile file(dataFilePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.commit();
}

void Store::commit(Domain domain)
{
    schedule::setNonWorkingDays(m_data.nonWorkingDays);
    switch (domain) {
    case Domain::Alarms:
        emit alarmsChanged();
        break;
    case Domain::Timers:
        emit timersChanged();
        break;
    case Domain::Todos:
        emit todosChanged();
        break;
    case Domain::Clocks:
        emit clocksChanged();
        break;
    case Domain::Pomodoro:
        emit pomodoroChanged();
        break;
    case Domain::All:
        emit alarmsChanged();
        emit timersChanged();
        emit todosChanged();
        emit clocksChanged();
        emit pomodoroChanged();
        break;
    }
    emit changed();
    scheduleSave();
}

Alarm* Store::findAlarm(const QUuid& id)
{
    const auto it = std::find_if(m_data.alarms.begin(), m_data.alarms.end(),
                                 [&id](const Alarm& alarm) { return alarm.id == id; });
    return it == m_data.alarms.end() ? nullptr : &(*it);
}

const Alarm* Store::findAlarm(const QUuid& id) const
{
    const auto it = std::find_if(m_data.alarms.cbegin(), m_data.alarms.cend(),
                                 [&id](const Alarm& alarm) { return alarm.id == id; });
    return it == m_data.alarms.cend() ? nullptr : &(*it);
}

CountdownTimer* Store::findTimer(const QUuid& id)
{
    const auto it = std::find_if(m_data.timers.begin(), m_data.timers.end(),
                                 [&id](const CountdownTimer& timer) { return timer.id == id; });
    return it == m_data.timers.end() ? nullptr : &(*it);
}

const CountdownTimer* Store::findTimer(const QUuid& id) const
{
    const auto it = std::find_if(m_data.timers.cbegin(), m_data.timers.cend(),
                                 [&id](const CountdownTimer& timer) { return timer.id == id; });
    return it == m_data.timers.cend() ? nullptr : &(*it);
}

Todo* Store::findTodo(const QUuid& id)
{
    const auto it = std::find_if(m_data.todos.begin(), m_data.todos.end(),
                                 [&id](const Todo& todo) { return todo.id == id; });
    return it == m_data.todos.end() ? nullptr : &(*it);
}

void Store::ensureGroup(const QString& name)
{
    if (name.isEmpty()) {
        return;
    }
    const auto it = std::find_if(m_data.groups.cbegin(), m_data.groups.cend(),
                                 [&name](const AlarmGroup& group) { return group.name == name; });
    if (it == m_data.groups.cend()) {
        m_data.groups.push_back({name, false});
    }
}

void Store::seedDemoData()
{
    m_data.groups.push_back({QStringLiteral("Утро"), false});
    m_data.groups.push_back({QStringLiteral("Дела"), false});

    Alarm morning;
    morning.label = QStringLiteral("Доброе утро");
    morning.group = QStringLiteral("Утро");
    morning.time = QTime(7, 0);
    morning.repeat = RepeatKind::Daily;
    morning.icon = QStringLiteral("🌅");
    morning.color = QStringLiteral("#e2842b");
    morning.note = QStringLiteral("Пример будильника. Включите его, когда будете готовы.");
    morning.enabled = false;
    m_data.alarms.push_back(morning);

    Alarm flowers;
    flowers.label = QStringLiteral("Полить цветы");
    flowers.group = QStringLiteral("Дела");
    flowers.time = QTime(19, 0);
    flowers.repeat = RepeatKind::Once;
    flowers.onceDate = QDate::currentDate().addDays(1);
    flowers.icon = QStringLiteral("🌱");
    flowers.color = QStringLiteral("#4c9a62");
    flowers.enabled = false;
    m_data.alarms.push_back(flowers);

    Alarm mail;
    mail.label = QStringLiteral("Проверить почту");
    mail.group = QStringLiteral("Дела");
    mail.time = QTime(9, 30);
    mail.repeat = RepeatKind::Weekdays;
    mail.icon = QStringLiteral("☕");
    mail.color = QStringLiteral("#3f7fd6");
    mail.enabled = false;
    m_data.alarms.push_back(mail);
}

void Store::seedDefaultClocks()
{
    if (!m_data.clocks.empty()) {
        return;
    }
    const auto add = [this](const QString& city) {
        for (const auto& entry : catalog::cities()) {
            if (entry.city.compare(city, Qt::CaseInsensitive) == 0) {
                WorldClock clock;
                clock.city = entry.city;
                clock.country = entry.country;
                clock.timeZoneId = entry.timeZoneId;
                m_data.clocks.push_back(clock);
                return;
            }
        }
    };
    add(QStringLiteral("Лондон"));
    add(QStringLiteral("Нью-Йорк"));
    add(QStringLiteral("Токио"));
}

void Store::normalize()
{
    for (const auto& alarm : m_data.alarms) {
        ensureGroup(alarm.group.isEmpty() ? QStringLiteral("Общие") : alarm.group);
    }
    if (m_data.groups.empty()) {
        m_data.groups.push_back({QStringLiteral("Общие"), false});
    }
    for (auto& alarm : m_data.alarms) {
        if (alarm.group.isEmpty()) {
            alarm.group = QStringLiteral("Общие");
        }
        alarm.monthWeek = std::clamp(alarm.monthWeek, -1, 4);
        alarm.intervalDays = std::clamp(alarm.intervalDays, 1, 365);
        alarm.intervalWeeks = std::clamp(alarm.intervalWeeks, 1, 52);
        for (auto& schedule : alarm.extraSchedules) {
            schedule.monthDay = std::clamp(schedule.monthDay, 0, 31);
            schedule.monthWeek = std::clamp(schedule.monthWeek, -1, 4);
            schedule.intervalDays = std::clamp(schedule.intervalDays, 1, 365);
            schedule.intervalWeeks = std::clamp(schedule.intervalWeeks, 1, 52);
        }
    }
    if (!m_data.nonWorkingSeeded) {
        const int year = QDate::currentDate().year();
        const auto addDays = [this](int y, int month, int first, int last, const QString& name) {
            for (int day = first; day <= last; ++day) {
                const QDate date(y, month, day);
                if (date.isValid()) {
                    m_data.nonWorkingDays.push_back({date, name});
                }
            }
        };
        for (int y = year; y <= year + 2; ++y) {
            addDays(y, 1, 1, 8, QStringLiteral("Новогодние каникулы"));
            m_data.nonWorkingDays.push_back({QDate(y, 2, 23), QStringLiteral("День защитника Отечества")});
            m_data.nonWorkingDays.push_back({QDate(y, 3, 8), QStringLiteral("Международный женский день")});
            m_data.nonWorkingDays.push_back({QDate(y, 5, 1), QStringLiteral("Праздник Весны и Труда")});
            m_data.nonWorkingDays.push_back({QDate(y, 5, 9), QStringLiteral("День Победы")});
            m_data.nonWorkingDays.push_back({QDate(y, 6, 12), QStringLiteral("День России")});
            m_data.nonWorkingDays.push_back({QDate(y, 11, 4), QStringLiteral("День народного единства")});
        }
        m_data.nonWorkingSeeded = true;
    }
    std::sort(m_data.nonWorkingDays.begin(), m_data.nonWorkingDays.end(),
              [](const NonWorkingDay& left, const NonWorkingDay& right) {
                  return left.date < right.date;
              });
    schedule::setNonWorkingDays(m_data.nonWorkingDays);
    for (auto& timer : m_data.timers) {
        if (timer.label.isEmpty()) {
            timer.label = QStringLiteral("Таймер");
        }
        timer.repeatCycles = std::clamp(timer.repeatCycles, 1, 999);
        for (auto& stage : timer.stages) {
            stage.durationSec = std::clamp(stage.durationSec, 1, 7 * 86'400);
        }
    }
    if (m_data.clocks.empty()) {
        seedDefaultClocks();
    }
}

void Store::scheduleSave()
{
    if (m_loaded) {
        m_saveTimer.start();
    }
}

} // namespace pac
