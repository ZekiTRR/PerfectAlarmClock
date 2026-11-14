#include "CalendarData.h"

#include "core/Catalog.h"
#include "core/Schedule.h"
#include "core/Theme.h"

#include <QObject>

#include <algorithm>

namespace pac::ui {

QVector<CalendarEvent> collectCalendarEvents(const Store& store, const QDate& date,
                                             bool includeAlarms, bool includeTasks)
{
    QVector<CalendarEvent> events;
    if (includeAlarms) {
        for (const auto& alarm : store.data().alarms) {
            if (!alarm.enabled || !schedule::occursOnDate(alarm, date)) {
                continue;
            }
            CalendarEvent event;
            event.id = alarm.id;
            event.time = alarm.time;
            event.title = alarm.label.isEmpty() ? QObject::tr("Будильник") : alarm.label;
            event.subtitle = schedule::repeatSummary(alarm);
            event.color = QColor(alarm.color);
            event.emoji = alarm.icon;
            events.append(event);
        }
    }
    if (includeTasks) {
        for (const auto& todo : store.data().todos) {
            if (!todo.hasDue || todo.dueDate != date) {
                continue;
            }
            CalendarEvent event;
            event.id = todo.id;
            event.task = true;
            event.done = todo.done;
            event.time = todo.dueTime.isValid() ? todo.dueTime : QTime(9, 0);
            event.title = todo.title;
            event.subtitle = catalog::priorityName(todo.priority);
            if (todo.done) {
                event.subtitle += QStringLiteral(" · ") + QObject::tr("выполнено");
            }
            event.color = ThemeManager::instance().current().success;
            event.emoji = QStringLiteral("✓");
            events.append(event);
        }
    }
    std::sort(events.begin(), events.end(), [](const CalendarEvent& left, const CalendarEvent& right) {
        if (left.time == right.time) {
            if (left.task != right.task) {
                return !left.task;
            }
            return left.title.localeAwareCompare(right.title) < 0;
        }
        return left.time < right.time;
    });
    return events;
}

} // namespace pac::ui
