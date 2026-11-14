#pragma once

#include <QDate>
#include <QDateTime>
#include <QList>
#include <QString>
#include <QTime>
#include <QUuid>

#include <vector>

namespace pac {

enum class RepeatKind {
    Once,
    Daily,
    Weekdays,
    Weekly,
    Monthly,
    Yearly,
    Interval,
    Weekends,
    EveryNDays,
    EveryNWeeks,
    MonthlyNthWeekday,
    LastWeekdayOfMonth
};

enum class HolidayPolicy {
    Ignore,
    Skip,
    ShiftToNextWorkday
};

enum class SnoozeEndAction {
    Stop,
    DisableAlarm
};

struct AlarmSchedule {
    QTime time{7, 0};
    RepeatKind repeat{RepeatKind::Daily};
    QList<Qt::DayOfWeek> days;
    int monthDay{1};
    int monthWeek{1};
    int intervalDays{2};
    int intervalWeeks{2};
    int intervalMinutes{30};
    QTime activeFrom{0, 0};
    QTime activeTo{23, 59};
    QDate anchorDate;
    bool enabled{true};
};

struct NonWorkingDay {
    QDate date;
    QString name;
};

enum class TimerState {
    Stopped,
    Running,
    Paused,
    Finished
};

enum class TimerMode {
    Countdown,
    CountUp,
    Repeating,
    Interval,
    MultiStage,
    Hiit,
    Tabata,
    Kitchen,
    Workout,
    Presentation,
    Meeting,
    Study,
    Sleep,
    Deadline,
    RandomInterval,
    Target
};

enum class TimerStageKind {
    Prepare,
    Work,
    Rest,
    Recovery,
    Custom
};

struct TimerStage {
    QUuid id{QUuid::createUuid()};
    QString name;
    int durationSec{300};
    TimerStageKind kind{TimerStageKind::Work};
    QString color{QStringLiteral("#3f7fd6")};
    QString soundId;
    QString message;
};

enum class PomodoroPhase {
    Work,
    ShortBreak,
    LongBreak
};

struct AlarmGroup {
    QString name;
    bool collapsed{false};
};

struct Alarm {
    QUuid id{QUuid::createUuid()};
    QString label;
    QString group{QStringLiteral("Общие")};
    QString note;
    bool enabled{false};

    QTime time{7, 0};
    QDate onceDate;
    RepeatKind repeat{RepeatKind::Daily};
    QList<Qt::DayOfWeek> days;
    int monthDay{1};
    int monthWeek{1};
    int intervalMinutes{30};
    int intervalDays{2};
    int intervalWeeks{2};
    QDate seriesStart;
    QString timeZoneId;
    HolidayPolicy holidayPolicy{HolidayPolicy::Ignore};
    QList<QDate> exceptions;
    std::vector<AlarmSchedule> extraSchedules;
    bool randomWindow{false};
    int randomBeforeMinutes{15};
    int randomAfterMinutes{15};
    QTime activeFrom{0, 0};
    QTime activeTo{23, 59};
    QDate activeFromDate;
    QDate activeToDate;

    QDateTime snoozeUntil;
    int snoozeCount{0};
    QDateTime lastTriggered;
    QDateTime intervalAnchor;

    QString icon{QStringLiteral("⏰")};
    QString color{QStringLiteral("#e2842b")};

    bool soundEnabled{true};
    QString soundId{QStringLiteral("builtin:chime")};
    int volume{80};
    bool loopSound{true};
    bool fadeIn{false};
    int fadeInSeconds{6};
    int fadeOutSeconds{2};
    bool randomSound{false};
    QString repeatSoundId;
    int maxSoundMinutes{5};

    bool snoozeEnabled{true};
    int snoozeMinutes{5};
    int snoozeMinutesAlt{10};
    bool snoozeDecrease{false};
    int snoozeDecreaseStep{1};
    int snoozeVolumeIncrease{5};
    SnoozeEndAction afterLastSnooze{SnoozeEndAction::Stop};
    int maxSnoozes{3};

    bool popupEnabled{true};
    int preAlertMinutes{0};

    bool postSummary{false};
    bool postTasks{false};
    QString postMessage;
    QString postOpen;
    bool disableAfterDismiss{false};
    bool backupEnabled{false};
    int backupAfterMinutes{5};
    bool persistent{false};
};

struct CountdownTimer {
    QUuid id{QUuid::createUuid()};
    QString label;
    int durationSec{600};
    qint64 remainingMs{600000};
    QDateTime endAt;
    TimerState state{TimerState::Stopped};
    TimerMode mode{TimerMode::Countdown};
    std::vector<TimerStage> stages;
    int repeatCycles{1};
    int currentStage{0};
    int completedCycles{0};
    QDateTime targetAt;
    int randomMinSec{60};
    int randomMaxSec{300};

    bool soundEnabled{true};
    QString soundId{QStringLiteral("builtin:digital")};
    int volume{80};
    bool loopSound{false};
    bool popupEnabled{true};
    bool autoRepeat{false};
    QString color{QStringLiteral("#3f7fd6")};
};

struct Todo {
    QUuid id{QUuid::createUuid()};
    QString title;
    QString notes;
    bool hasDue{false};
    QDate dueDate;
    QTime dueTime{9, 0};
    int priority{1};
    bool done{false};
    QDateTime completedAt;
};

struct WorldClock {
    QUuid id{QUuid::createUuid()};
    QString city;
    QString country;
    QString timeZoneId;
    bool showDate{true};
    bool showDifference{true};
};

struct PomodoroSettings {
    int workMinutes{25};
    int shortBreakMinutes{5};
    int longBreakMinutes{15};
    int longBreakEvery{4};
    bool autoStartBreaks{true};
    bool autoStartWork{false};
    bool soundEnabled{true};
    QString soundId{QStringLiteral("builtin:gentle")};
    int volume{70};
    bool notifications{true};
    int dailyGoal{8};
};

struct PomodoroStats {
    QDate day;
    int completedToday{0};
    int focusMinutesToday{0};
    int currentStreak{0};
};

struct AppData {
    std::vector<Alarm> alarms;
    std::vector<AlarmGroup> groups;
    std::vector<CountdownTimer> timers;
    std::vector<Todo> todos;
    std::vector<WorldClock> clocks;
    std::vector<NonWorkingDay> nonWorkingDays;
    bool nonWorkingSeeded{false};
    PomodoroSettings pomodoro;
    PomodoroStats pomodoroStats;
};

struct PomodoroSession {
    PomodoroPhase phase{PomodoroPhase::Work};
    bool running{false};
    qint64 remainingMs{25LL * 60 * 1000};
    int completedInSet{0};
    int completedToday{0};
    int focusMinutesToday{0};
};

} // namespace pac
