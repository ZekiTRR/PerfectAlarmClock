#pragma once

#include "core/Store.h"

#include <QColor>
#include <QDate>
#include <QString>
#include <QTime>
#include <QUuid>
#include <QVector>

namespace pac::ui {

struct CalendarEvent {
    QUuid id;
    bool task{false};
    bool done{false};
    QTime time;
    QString title;
    QString subtitle;
    QColor color;
    QString emoji;
};

[[nodiscard]] QVector<CalendarEvent> collectCalendarEvents(const Store& store, const QDate& date,
                                                           bool includeAlarms, bool includeTasks);

} // namespace pac::ui
