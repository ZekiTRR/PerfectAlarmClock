#include "CalendarBoards.h"

#include "CalendarData.h"
#include "UiKit.h"
#include "core/Catalog.h"
#include "core/Schedule.h"
#include "core/Settings.h"
#include "core/Store.h"
#include "core/Theme.h"

#include <QEnterEvent>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QObject>
#include <QPainter>
#include <QScrollBar>
#include <QTimeZone>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>
#include <utility>

namespace pac::ui {
namespace {

[[nodiscard]] QFont boardFont(const QFont& base, qreal size, QFont::Weight weight = QFont::Normal)
{
    QFont font = base;
    font.setPointSizeF(size);
    font.setWeight(weight);
    return font;
}

[[nodiscard]] QString shortTime(const QTime& time)
{
    return schedule::timeText(time, Settings::instance().use24h());
}

[[nodiscard]] QString eventCountText(int count)
{
    const int mod10 = count % 10;
    const int mod100 = count % 100;
    QString word = QObject::tr("событий");
    if (mod10 == 1 && mod100 != 11) {
        word = QObject::tr("событие");
    } else if (mod10 >= 2 && mod10 <= 4 && (mod100 < 12 || mod100 > 14)) {
        word = QObject::tr("события");
    }
    return QStringLiteral("%1 %2").arg(count).arg(word);
}

constexpr qreal kDayHourHeight = 52.0;

} // namespace

WeekBoard::WeekBoard(Store& store, QWidget* parent)
    : QWidget(parent)
    , m_store(store)
{
    setMouseTracking(true);
    setMinimumSize(300, 220);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_start = QDate::currentDate();
    m_selected = QDate::currentDate();
}

void WeekBoard::setWeeks(int weeks)
{
    m_weeks = std::clamp(weeks, 1, 8);
    update();
}

void WeekBoard::setStartDate(const QDate& date)
{
    if (date.isValid() && m_start != date) {
        m_start = date;
        update();
    }
}

void WeekBoard::setSelectedDate(const QDate& date)
{
    if (date.isValid() && m_selected != date) {
        m_selected = date;
        update();
    }
}

void WeekBoard::setCallbacks(std::function<void(const QDate&)> daySelected,
                             std::function<void(const QDate&)> dayActivated)
{
    m_onDaySelected = std::move(daySelected);
    m_onDayActivated = std::move(dayActivated);
}

int WeekBoard::headerHeight() const
{
    return m_weeks == 1 ? 56 : 28;
}

QRectF WeekBoard::cellRect(int index) const
{
    const qreal columnWidth = width() / 7.0;
    if (m_weeks == 1) {
        return QRectF(index * columnWidth, 0, columnWidth, height());
    }
    const int row = index / 7;
    const int column = index % 7;
    const qreal rowHeight = (height() - headerHeight()) / m_weeks;
    return QRectF(column * columnWidth, headerHeight() + row * rowHeight, columnWidth, rowHeight);
}

QDate WeekBoard::dateAt(const QPoint& point) const
{
    if (m_weeks == 1) {
        if (point.x() < 0 || point.x() >= width()) {
            return {};
        }
        const int column = static_cast<int>(point.x() / (width() / 7.0));
        return m_start.addDays(column);
    }
    if (point.y() < headerHeight()) {
        return {};
    }
    const qreal rowHeight = (height() - headerHeight()) / m_weeks;
    if (rowHeight <= 0.0) {
        return {};
    }
    const int column = static_cast<int>(point.x() / (width() / 7.0));
    const int row = static_cast<int>((point.y() - headerHeight()) / rowHeight);
    if (column < 0 || column > 6 || row < 0 || row >= m_weeks) {
        return {};
    }
    return m_start.addDays(row * 7 + column);
}

void WeekBoard::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    const ThemeColors& colors = ThemeManager::instance().current();
    const bool showAlarms = Settings::instance().calendarShowAlarms();
    const bool showTasks = Settings::instance().calendarShowTasks();
    const QDate today = QDate::currentDate();
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), colors.card);

    const qreal columnWidth = width() / 7.0;
    QFont headerFont = boardFont(font(), 7.8, QFont::DemiBold);
    QFont dateFont = boardFont(font(), 10.0, QFont::DemiBold);
    QFont eventFont = boardFont(font(), 7.8);
    QFont timeFont = boardFont(font(), 7.0, QFont::Medium);

    if (m_weeks == 1) {
        for (int column = 0; column < 7; ++column) {
            const QDate date = m_start.addDays(column);
            const QRectF header(column * columnWidth, 0.0, columnWidth, headerHeight());
            const bool weekend = date.dayOfWeek() >= 6;
            QColor background = tinted(colors.headerTop, 0.9);
            if (date == m_selected) {
                background = tinted(colors.accent, 0.10);
            } else if (date == m_hover) {
                background = tinted(colors.hoverRow, 0.9);
            }
            painter.fillRect(header, background);
            painter.setFont(headerFont);
            painter.setPen(weekend ? colors.danger : colors.headerText);
            painter.drawText(QRectF(header.left(), 6.0, header.width(), 15.0), Qt::AlignCenter,
                             catalog::dayShort(static_cast<Qt::DayOfWeek>(date.dayOfWeek())).toUpper());
            painter.setFont(dateFont);
            painter.setPen(date == today ? QColor(Qt::white) : colors.text);
            const QRectF dateCircle(header.center().x() - 24.0, 22.0, 48.0, 24.0);
            if (date == today) {
                painter.setPen(Qt::NoPen);
                painter.setBrush(colors.accent);
                painter.drawRoundedRect(dateCircle, 12.0, 12.0);
                painter.setPen(QColor(Qt::white));
            }
            painter.drawText(QRectF(header.left(), 22.0, header.width(), 24.0), Qt::AlignCenter,
                             date.toString(QStringLiteral("dd.MM")));
            painter.setPen(QPen(colors.grid, 1.0));
            if (column > 0) {
                painter.drawLine(header.topLeft(), header.bottomLeft());
            }
            painter.drawLine(header.bottomLeft(), header.bottomRight());
            painter.setPen(QPen(tinted(colors.grid, 0.7), 1.0));
            painter.drawLine(header.left(), height() - 1, header.right(), height() - 1);

            const QVector<CalendarEvent> events =
                collectCalendarEvents(m_store, date, showAlarms, showTasks);
            if (date == m_selected) {
                painter.fillRect(QRectF(header.left(), headerHeight(), header.width(),
                                        height() - headerHeight()),
                                 tinted(colors.accent, 0.035));
            }
            qreal y = headerHeight() + 9.0;
            int shown = 0;
            const bool narrow = columnWidth < 132.0;
            const qreal itemHeight = narrow ? 34.0 : 23.0;
            for (const auto& item : events) {
                if (y + itemHeight - 2.0 > height() - 7.0) {
                    break;
                }
                painter.setPen(Qt::NoPen);
                painter.setBrush(item.done ? tinted(item.color, 0.45) : item.color);
                painter.drawRoundedRect(QRectF(header.left() + 7.0, y + 4.0, 3.0,
                                               narrow ? 25.0 : 11.0),
                                        1.5, 1.5);
                if (narrow) {
                    painter.setFont(boardFont(font(), 6.8, QFont::Medium));
                    painter.setPen(colors.dim);
                    painter.drawText(QRectF(header.left() + 14.0, y, header.width() - 20.0, 13.0),
                                     Qt::AlignLeft | Qt::AlignVCenter, shortTime(item.time));
                    painter.setFont(eventFont);
                    painter.setPen(item.done ? colors.faint : colors.text);
                    const int available = std::max(10, static_cast<int>(header.width()) - 20);
                    painter.drawText(QRectF(header.left() + 14.0, y + 13.0, available, 17.0),
                                     Qt::AlignLeft | Qt::AlignVCenter,
                                     painter.fontMetrics().elidedText(item.title, Qt::ElideRight,
                                                                      available));
                } else {
                    painter.setFont(timeFont);
                    painter.setPen(colors.dim);
                    painter.drawText(QRectF(header.left() + 14.0, y, 31.0, 21.0),
                                     Qt::AlignLeft | Qt::AlignVCenter, shortTime(item.time));
                    painter.setFont(eventFont);
                    painter.setPen(item.done ? colors.faint : colors.text);
                    const int available = static_cast<int>(header.width() - 51.0);
                    painter.drawText(QRectF(header.left() + 48.0, y, available, 21.0),
                                     Qt::AlignLeft | Qt::AlignVCenter,
                                     painter.fontMetrics().elidedText(item.title, Qt::ElideRight,
                                                                      std::max(10, available)));
                }
                y += itemHeight;
                ++shown;
            }
            if (shown == 0) {
                painter.setFont(boardFont(font(), 7.4));
                painter.setPen(colors.faint);
                painter.drawText(QRectF(header.left(), headerHeight(), header.width(), 34.0),
                                 Qt::AlignCenter, tr("Нет событий"));
            } else if (shown < events.size()) {
                painter.setFont(boardFont(font(), 7.0));
                painter.setPen(colors.faint);
                painter.drawText(QRectF(header.left() + 14.0, y, header.width() - 20.0, 16.0),
                                 Qt::AlignLeft | Qt::AlignVCenter,
                                 tr("+%1 ещё").arg(events.size() - shown));
            }
            if (date == m_selected) {
                painter.setPen(QPen(colors.accent, 2.0));
                painter.drawLine(QPointF(header.left() + 3.0, height() - 2.0),
                                 QPointF(header.right() - 3.0, height() - 2.0));
            }
        }
        painter.setPen(QPen(colors.grid, 1.0));
        for (int column = 1; column < 7; ++column) {
            painter.drawLine(QPointF(column * columnWidth, headerHeight()),
                             QPointF(column * columnWidth, height()));
        }
        return;
    }

    // Multi-week overview.
    for (int column = 0; column < 7; ++column) {
        const int day = ((Settings::instance().weekStartsMonday() ? Qt::Monday : Qt::Sunday)
                         + column - 1) % 7 + 1;
        const QRectF header(column * columnWidth, 0.0, columnWidth, headerHeight());
        painter.fillRect(header, tinted(colors.headerTop, 0.9));
        painter.setFont(headerFont);
        painter.setPen(day >= 6 ? colors.danger : colors.headerText);
        painter.drawText(header, Qt::AlignCenter,
                         catalog::dayShort(static_cast<Qt::DayOfWeek>(day)).toUpper());
        painter.setPen(QPen(colors.grid, 1.0));
        if (column > 0) {
            painter.drawLine(header.topLeft(), header.bottomLeft());
        }
        painter.drawLine(header.bottomLeft(), header.bottomRight());
    }

    for (int row = 0; row < m_weeks; ++row) {
        for (int column = 0; column < 7; ++column) {
            const int index = row * 7 + column;
            const QDate date = m_start.addDays(index);
            const QRectF cell = cellRect(index);
            const bool weekend = date.dayOfWeek() >= 6;
            QColor background = weekend ? tinted(colors.danger, 0.035) : colors.card;
            if (date == m_selected) {
                background = tinted(colors.accent, 0.11);
            } else if (date == m_hover) {
                background = tinted(colors.hoverRow, 0.8);
            }
            painter.fillRect(cell, background);
            painter.setPen(QPen(colors.grid, 1.0));
            painter.drawLine(cell.topRight(), cell.bottomRight());
            painter.drawLine(cell.bottomLeft(), cell.bottomRight());

            painter.setFont(dateFont);
            const bool isToday = date == today;
            const QRectF numberRect(cell.left() + 6.0, cell.top() + 4.0, 44.0, 21.0);
            if (isToday) {
                painter.setPen(Qt::NoPen);
                painter.setBrush(colors.accent);
                painter.drawRoundedRect(QRectF(cell.left() + 5.0, cell.top() + 5.0, 38.0, 20.0),
                                        10.0, 10.0);
                painter.setPen(QColor(Qt::white));
            } else {
                painter.setPen(weekend ? colors.danger : colors.text);
            }
            painter.drawText(numberRect, Qt::AlignLeft | Qt::AlignVCenter,
                             date.toString(QStringLiteral("dd.MM")));

            const auto events = collectCalendarEvents(m_store, date, showAlarms, showTasks);
            qreal y = cell.top() + 29.0;
            const qreal lineHeight = 15.0;
            int shown = 0;
            const int availableLines = std::max(0, static_cast<int>((cell.height() - 33.0)
                                                                    / lineHeight));
            for (const auto& item : events) {
                if (shown >= availableLines || y + lineHeight > cell.bottom() - 3.0) {
                    break;
                }
                painter.setPen(Qt::NoPen);
                painter.setBrush(item.done ? tinted(item.color, 0.45) : item.color);
                painter.drawRoundedRect(QRectF(cell.left() + 7.0, y + 4.0, 2.5, 8.0), 1.2, 1.2);
                painter.setFont(eventFont);
                painter.setPen(item.done ? colors.faint : colors.text);
                painter.drawText(QRectF(cell.left() + 14.0, y, cell.width() - 20.0, lineHeight),
                                 Qt::AlignLeft | Qt::AlignVCenter,
                                 painter.fontMetrics().elidedText(item.title, Qt::ElideRight,
                                                                  std::max(10, static_cast<int>(cell.width()) - 20)));
                y += lineHeight;
                ++shown;
            }
            if (shown > 0 && shown < events.size()) {
                painter.setFont(boardFont(font(), 6.8));
                painter.setPen(colors.faint);
                painter.drawText(QRectF(cell.left() + 14.0, y, cell.width() - 20.0, lineHeight),
                                 Qt::AlignLeft | Qt::AlignVCenter,
                                 tr("+%1").arg(events.size() - shown));
            }
            if (date == m_selected) {
                painter.setPen(QPen(colors.accent, 1.4));
                painter.setBrush(Qt::NoBrush);
                painter.drawRoundedRect(cell.adjusted(1.0, 1.0, -1.0, -1.0), 3.0, 3.0);
            }
        }
    }
}

void WeekBoard::mouseMoveEvent(QMouseEvent* event)
{
    const QDate date = dateAt(event->position().toPoint());
    if (date != m_hover) {
        m_hover = date;
        update();
    }
}

void WeekBoard::leaveEvent(QEvent* event)
{
    Q_UNUSED(event)
    if (m_hover.isValid()) {
        m_hover = QDate();
        update();
    }
}

void WeekBoard::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }
    const QDate date = dateAt(event->position().toPoint());
    if (!date.isValid()) {
        return;
    }
    m_selected = date;
    update();
    if (m_onDaySelected != nullptr) {
        m_onDaySelected(date);
    }
}

void WeekBoard::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }
    const QDate date = dateAt(event->position().toPoint());
    if (date.isValid() && m_onDayActivated != nullptr) {
        m_onDayActivated(date);
    }
}

class DayBoard::Canvas final : public QWidget {
public:
    explicit Canvas(Store& store, QWidget* parent = nullptr)
        : QWidget(parent)
        , m_store(store)
    {
        setMouseTracking(true);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setFixedHeight(1280);
    }

    void setDate(const QDate& date)
    {
        if (date.isValid()) {
            m_date = date;
            refresh();
        }
    }

    void setEventCallback(std::function<void(const QUuid&, bool)> callback)
    {
        m_onEvent = std::move(callback);
    }

    void refresh()
    {
        m_events = collectCalendarEvents(m_store, m_date, Settings::instance().calendarShowAlarms(),
                                         Settings::instance().calendarShowTasks());
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event)
        const ThemeColors& colors = ThemeManager::instance().current();
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.fillRect(rect(), colors.card);
        m_hits.clear();

        constexpr qreal top = 16.0;
        constexpr qreal gutter = 62.0;
        const qreal right = width() - 10.0;
        const qreal workingTop = top + 8.0 * kDayHourHeight;
        painter.fillRect(QRectF(gutter, workingTop, right - gutter, 10.0 * kDayHourHeight),
                         tinted(colors.headerTop, 0.22));

        QFont hourFont = boardFont(font(), 7.4, QFont::Medium);
        for (int hour = 0; hour < 24; ++hour) {
            const qreal y = top + hour * kDayHourHeight;
            painter.setPen(QPen(colors.grid, 0.8));
            painter.drawLine(QPointF(gutter, y), QPointF(right, y));
            painter.setPen(QPen(tinted(colors.grid, 0.55), 0.8, Qt::DotLine));
            painter.drawLine(QPointF(gutter, y + kDayHourHeight / 2.0),
                             QPointF(right, y + kDayHourHeight / 2.0));
            painter.setFont(hourFont);
            painter.setPen(colors.faint);
            painter.drawText(QRectF(6.0, y - 9.0, gutter - 14.0, 18.0),
                             Qt::AlignRight | Qt::AlignVCenter, shortTime(QTime(hour, 0)));
        }
        painter.setPen(QPen(colors.grid, 0.8));
        painter.drawLine(QPointF(gutter, top), QPointF(gutter, top + 24.0 * kDayHourHeight));
        painter.drawLine(QPointF(gutter, top + 24.0 * kDayHourHeight), QPointF(right, top + 24.0 * kDayHourHeight));

        struct Placed {
            CalendarEvent event;
            qreal y{0.0};
            int lane{0};
        };
        QVector<Placed> placed;
        QVector<qreal> laneEnds;
        constexpr qreal cardHeight = 42.0;
        for (const auto& item : m_events) {
            const qreal minutes = static_cast<qreal>(item.time.msecsSinceStartOfDay()) / 60000.0;
            const qreal y = std::clamp(top + minutes / 60.0 * kDayHourHeight, top,
                                       height() - cardHeight - 14.0);
            int lane = 0;
            while (lane < laneEnds.size() && laneEnds.at(lane) > y + 2.0) {
                ++lane;
            }
            if (lane == laneEnds.size()) {
                laneEnds.append(y + cardHeight + 4.0);
            } else {
                laneEnds[lane] = y + cardHeight + 4.0;
            }
            placed.append({item, y, lane});
        }

        const int lanes = std::max(1, static_cast<int>(laneEnds.size()));
        const qreal areaLeft = gutter + 8.0;
        const qreal laneWidth = (right - areaLeft) / lanes;
        for (const auto& entry : placed) {
            const CalendarEvent& item = entry.event;
            const QRectF card(areaLeft + entry.lane * laneWidth + 1.0, entry.y,
                              laneWidth - 4.0, cardHeight);
            const QColor color = item.done ? tinted(item.color, 0.55) : item.color;
            painter.setPen(QPen(tinted(color, 0.5), 1.0));
            painter.setBrush(tinted(color, 0.09));
            painter.drawRoundedRect(card, 5.0, 5.0);
            painter.setPen(Qt::NoPen);
            painter.setBrush(color);
            painter.drawRoundedRect(QRectF(card.left(), card.top(), 3.5, card.height()), 1.75, 1.75);

            const int textWidth = std::max(20, static_cast<int>(card.width()) - 26);
            painter.setFont(boardFont(font(), 8.8, QFont::DemiBold));
            painter.setPen(item.done ? colors.faint : colors.text);
            painter.drawText(QRectF(card.left() + 12.0, card.top() + 4.0, textWidth, 19.0),
                             Qt::AlignLeft | Qt::AlignVCenter,
                             painter.fontMetrics().elidedText(item.title, Qt::ElideRight, textWidth));
            painter.setFont(boardFont(font(), 7.4));
            painter.setPen(colors.dim);
            const QString detail = QStringLiteral("%1 · %2")
                                       .arg(shortTime(item.time), item.subtitle);
            painter.drawText(QRectF(card.left() + 12.0, card.top() + 23.0, textWidth, 15.0),
                             Qt::AlignLeft | Qt::AlignVCenter,
                             painter.fontMetrics().elidedText(detail, Qt::ElideRight, textWidth));
            m_hits.append({card, item.id, item.task});
        }

        if (m_date == QDate::currentDate()) {
            const QTime now = QTime::currentTime();
            const qreal y = top + (static_cast<qreal>(now.msecsSinceStartOfDay()) / 60000.0)
                                      / 60.0 * kDayHourHeight;
            painter.setPen(QPen(colors.danger, 1.5));
            painter.drawLine(QPointF(gutter, y), QPointF(right, y));
            painter.setPen(Qt::NoPen);
            painter.setBrush(colors.danger);
            painter.drawEllipse(QPointF(gutter, y), 3.5, 3.5);
            const QRectF pill(right - 54.0, y - 9.0, 50.0, 18.0);
            painter.setBrush(colors.card);
            painter.setPen(QPen(tinted(colors.danger, 0.55), 1.0));
            painter.drawRoundedRect(pill, 9.0, 9.0);
            painter.setFont(boardFont(font(), 6.9, QFont::DemiBold));
            painter.setPen(colors.danger);
            painter.drawText(pill, Qt::AlignCenter, shortTime(now));
        }
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
        bool overEvent = false;
        for (const auto& hit : m_hits) {
            if (hit.rect.contains(event->position())) {
                overEvent = true;
                break;
            }
        }
        setCursor(overEvent ? Qt::PointingHandCursor : Qt::ArrowCursor);
    }

    void leaveEvent(QEvent* event) override
    {
        Q_UNUSED(event)
        setCursor(Qt::ArrowCursor);
    }

    void mouseReleaseEvent(QMouseEvent* event) override
    {
        if (event->button() != Qt::LeftButton) {
            return;
        }
        for (const auto& hit : m_hits) {
            if (hit.rect.contains(event->position()) && m_onEvent != nullptr) {
                m_onEvent(hit.id, hit.task);
                return;
            }
        }
    }

private:
    struct Hit {
        QRectF rect;
        QUuid id;
        bool task{false};
    };

    Store& m_store;
    QDate m_date{QDate::currentDate()};
    QVector<CalendarEvent> m_events;
    QVector<Hit> m_hits;
    std::function<void(const QUuid&, bool)> m_onEvent;
};

DayBoard::DayBoard(Store& store, QWidget* parent)
    : QScrollArea(parent)
{
    setWidgetResizable(true);
    setFrameShape(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    viewport()->setAutoFillBackground(false);
    m_canvas = new Canvas(store, this);
    setWidget(m_canvas);
}

void DayBoard::setDate(const QDate& date)
{
    if (!date.isValid()) {
        return;
    }
    const bool changed = date != m_date;
    m_date = date;
    m_canvas->setDate(date);
    if (changed) {
        QTimer::singleShot(0, this, [this] {
            verticalScrollBar()->setValue(static_cast<int>(6.0 * kDayHourHeight));
        });
    }
}

void DayBoard::setEventCallback(std::function<void(const QUuid&, bool)> callback)
{
    m_canvas->setEventCallback(std::move(callback));
}

void DayBoard::refresh()
{
    m_canvas->refresh();
}

void DayBoard::tick()
{
    m_canvas->update();
}

class AgendaBoard::Canvas final : public QWidget {
public:
    struct Section {
        QDate date;
        QVector<CalendarEvent> events;
    };

    explicit Canvas(Store& store, QWidget* parent = nullptr)
        : QWidget(parent)
        , m_store(store)
    {
        setMouseTracking(true);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setMinimumHeight(360);
    }

    void setRange(const QDate& start, int days)
    {
        m_start = start;
        m_days = std::max(1, days);
        refresh();
    }

    void setStyle(AgendaBoard::Style style)
    {
        m_style = style;
        refresh();
    }

    void setEventCallback(std::function<void(const QUuid&, bool)> callback)
    {
        m_onEvent = std::move(callback);
    }

    void refresh()
    {
        m_sections.clear();
        const bool alarms = Settings::instance().calendarShowAlarms();
        const bool tasks = Settings::instance().calendarShowTasks();
        int totalHeight = 24;
        for (int i = 0; i < m_days; ++i) {
            const QDate date = m_start.addDays(i);
            QVector<CalendarEvent> events = collectCalendarEvents(m_store, date, alarms, tasks);
            if (events.isEmpty()) {
                continue;
            }
            Section section{date, std::move(events)};
            m_sections.append(section);
            totalHeight += m_style == AgendaBoard::Style::Agenda
                               ? 34 + m_sections.last().events.size() * 56 + 8
                               : 54 + m_sections.last().events.size() * 70 + 8;
        }
        setFixedHeight(std::max(360, totalHeight + 12));
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event)
        const ThemeColors& colors = ThemeManager::instance().current();
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.fillRect(rect(), colors.card);
        m_hits.clear();

        if (m_sections.isEmpty()) {
            painter.setPen(QPen(colors.faint, 1.2));
            const QRectF iconRect(width() / 2.0 - 16.0, height() / 2.0 - 54.0, 32.0, 32.0);
            icons::paint(painter, iconRect, icons::Icon::Calendar, colors.faint, 1.7);
            painter.setFont(boardFont(font(), 11.0, QFont::DemiBold));
            painter.setPen(colors.text);
            painter.drawText(QRectF(20.0, height() / 2.0 - 10.0, width() - 40.0, 26.0),
                             Qt::AlignCenter, tr("Нет событий"));
            painter.setFont(boardFont(font(), 8.0));
            painter.setPen(colors.faint);
            painter.drawText(QRectF(20.0, height() / 2.0 + 18.0, width() - 40.0, 24.0),
                             Qt::AlignCenter,
                             tr("В выбранном диапазоне нет будильников или задач."));
            return;
        }

        if (m_style == AgendaBoard::Style::Agenda) {
            qreal y = 16.0;
            for (const auto& section : m_sections) {
                painter.setFont(boardFont(font(), 10.0, QFont::DemiBold));
                painter.setPen(colors.text);
                painter.drawText(QRectF(20.0, y, width() - 40.0, 26.0),
                                 Qt::AlignLeft | Qt::AlignVCenter,
                                 tr("%1, %2")
                                     .arg(catalog::dayLong(static_cast<Qt::DayOfWeek>(
                                         section.date.dayOfWeek())))
                                     .arg(QStringLiteral("%1 %2")
                                              .arg(section.date.day())
                                              .arg(catalog::monthNameGenitive(section.date.month()))));
                painter.setFont(boardFont(font(), 7.2));
                painter.setPen(colors.faint);
                painter.drawText(QRectF(width() - 120.0, y, 100.0, 26.0),
                                 Qt::AlignRight | Qt::AlignVCenter,
                                 eventCountText(section.events.size()));
                y += 28.0;
                painter.setPen(QPen(colors.grid, 1.0));
                painter.drawLine(QPointF(20.0, y), QPointF(width() - 20.0, y));
                for (const auto& item : section.events) {
                    const QRectF row(18.0, y + 3.0, width() - 36.0, 50.0);
                    painter.setFont(boardFont(font(), 8.2, QFont::DemiBold));
                    painter.setPen(item.done ? colors.faint : item.color);
                    painter.drawText(QRectF(row.left() + 8.0, row.top(), 46.0, row.height()),
                                     Qt::AlignLeft | Qt::AlignVCenter, shortTime(item.time));
                    painter.setPen(Qt::NoPen);
                    painter.setBrush(tinted(item.color, item.done ? 0.35 : 0.8));
                    painter.drawRoundedRect(QRectF(row.left() + 58.0, row.top() + 11.0, 24.0, 24.0),
                                            6.0, 6.0);
                    painter.setPen(colors.text);
                    painter.setFont(boardFont(font(), 9.0, QFont::DemiBold));
                    painter.drawText(QRectF(row.left() + 90.0, row.top() + 4.0,
                                            row.width() - 100.0, 20.0),
                                     Qt::AlignLeft | Qt::AlignVCenter, item.title);
                    painter.setFont(boardFont(font(), 7.6));
                    painter.setPen(colors.dim);
                    painter.drawText(QRectF(row.left() + 90.0, row.top() + 24.0,
                                            row.width() - 100.0, 18.0),
                                     Qt::AlignLeft | Qt::AlignVCenter,
                                     painter.fontMetrics().elidedText(item.subtitle, Qt::ElideRight,
                                                                      std::max(20, static_cast<int>(row.width()) - 110)));
                    m_hits.append({row, item.id, item.task});
                    painter.setPen(QPen(tinted(colors.grid, 0.8), 1.0));
                    painter.drawLine(QPointF(20.0, row.bottom() + 2.0),
                                     QPointF(width() - 20.0, row.bottom() + 2.0));
                    y += 56.0;
                }
                y += 10.0;
            }
            return;
        }

        // Timeline style.
        qreal y = 18.0;
        for (const auto& section : m_sections) {
            const qreal sectionTop = y;
            const qreal sectionHeight = 42.0 + section.events.size() * 70.0;
            painter.setPen(QPen(tinted(colors.border, 0.9), 1.0));
            painter.drawLine(QPointF(98.0, sectionTop + 18.0), QPointF(98.0, sectionTop + sectionHeight));
            painter.setFont(boardFont(font(), 20.0, QFont::Light));
            painter.setPen(colors.text);
            painter.drawText(QRectF(18.0, sectionTop, 52.0, 30.0), Qt::AlignLeft | Qt::AlignVCenter,
                             QString::number(section.date.day()));
            painter.setFont(boardFont(font(), 7.6, QFont::DemiBold));
            painter.setPen(colors.dim);
            painter.drawText(QRectF(18.0, sectionTop + 32.0, 74.0, 18.0),
                             Qt::AlignLeft | Qt::AlignVCenter,
                             QStringLiteral("%1 · %2")
                                 .arg(catalog::dayShort(static_cast<Qt::DayOfWeek>(
                                      section.date.dayOfWeek())))
                                 .arg(catalog::monthNameGenitive(section.date.month())));
            qreal eventY = sectionTop + 30.0;
            for (const auto& item : section.events) {
                const QRectF card(116.0, eventY + 4.0, width() - 134.0, 62.0);
                painter.setPen(QPen(colors.border, 1.0));
                painter.setBrush(tinted(colors.panel, 0.65));
                painter.drawRoundedRect(card, 5.0, 5.0);
                painter.setPen(Qt::NoPen);
                painter.setBrush(item.done ? tinted(item.color, 0.4) : item.color);
                painter.drawRoundedRect(QRectF(card.left(), card.top(), 3.0, card.height()), 1.5, 1.5);
                painter.setBrush(colors.card);
                painter.drawEllipse(QPointF(98.0, card.center().y()), 6.0, 6.0);
                painter.setBrush(item.color);
                painter.drawEllipse(QPointF(98.0, card.center().y()), 3.5, 3.5);
                painter.setPen(colors.dim);
                painter.setFont(boardFont(font(), 7.8, QFont::DemiBold));
                painter.drawText(QRectF(card.left() + 12.0, card.top() + 8.0, 44.0, 18.0),
                                 Qt::AlignLeft | Qt::AlignVCenter, shortTime(item.time));
                painter.setFont(boardFont(font(), 9.2, QFont::DemiBold));
                painter.setPen(item.done ? colors.faint : colors.text);
                painter.drawText(QRectF(card.left() + 58.0, card.top() + 6.0,
                                        card.width() - 70.0, 22.0),
                                 Qt::AlignLeft | Qt::AlignVCenter, item.title);
                painter.setFont(boardFont(font(), 7.6));
                painter.setPen(colors.dim);
                painter.drawText(QRectF(card.left() + 12.0, card.top() + 30.0,
                                        card.width() - 24.0, 20.0),
                                 Qt::AlignLeft | Qt::AlignVCenter,
                                 painter.fontMetrics().elidedText(item.subtitle, Qt::ElideRight,
                                                                  std::max(20, static_cast<int>(card.width()) - 24)));
                m_hits.append({card, item.id, item.task});
                eventY += 70.0;
            }
            y = sectionTop + sectionHeight + 18.0;
        }
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
        bool overEvent = false;
        for (const auto& hit : m_hits) {
            if (hit.rect.contains(event->position())) {
                overEvent = true;
                break;
            }
        }
        setCursor(overEvent ? Qt::PointingHandCursor : Qt::ArrowCursor);
    }

    void mouseReleaseEvent(QMouseEvent* event) override
    {
        if (event->button() != Qt::LeftButton) {
            return;
        }
        for (const auto& hit : m_hits) {
            if (hit.rect.contains(event->position()) && m_onEvent != nullptr) {
                m_onEvent(hit.id, hit.task);
                return;
            }
        }
    }

private:
    struct Hit {
        QRectF rect;
        QUuid id;
        bool task{false};
    };

    Store& m_store;
    QDate m_start{QDate::currentDate()};
    int m_days{1};
    AgendaBoard::Style m_style{AgendaBoard::Style::Agenda};
    QVector<Section> m_sections;
    QVector<Hit> m_hits;
    std::function<void(const QUuid&, bool)> m_onEvent;
};

AgendaBoard::AgendaBoard(Store& store, QWidget* parent)
    : QScrollArea(parent)
{
    setWidgetResizable(true);
    setFrameShape(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    viewport()->setAutoFillBackground(false);
    m_canvas = new Canvas(store, this);
    setWidget(m_canvas);
}

void AgendaBoard::setRange(const QDate& start, int days)
{
    m_canvas->setRange(start, days);
    m_canvas->resize(viewport()->width(), m_canvas->height());
}

void AgendaBoard::setStyle(Style style)
{
    m_canvas->setStyle(style);
}

void AgendaBoard::setEventCallback(std::function<void(const QUuid&, bool)> callback)
{
    m_canvas->setEventCallback(std::move(callback));
}

void AgendaBoard::refresh()
{
    m_canvas->refresh();
}

} // namespace pac::ui
