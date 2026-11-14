#include "CalendarPage.h"

#include "CalendarBoards.h"
#include "CalendarData.h"
#include "DateToolsDialog.h"
#include "core/Catalog.h"
#include "core/Fonts.h"
#include "core/Schedule.h"
#include "core/Settings.h"
#include "core/Store.h"
#include "core/Theme.h"

#include <QAbstractButton>
#include <QComboBox>
#include <QGridLayout>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QButtonGroup>
#include <QDateTime>
#include <QEnterEvent>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>
#include <functional>
#include <utility>

namespace pac::ui {
namespace {

[[nodiscard]] QString countText(int count, const QString& one, const QString& few, const QString& many)
{
    const int mod10 = count % 10;
    const int mod100 = count % 100;
    QString word = many;
    if (mod10 == 1 && mod100 != 11) {
        word = one;
    } else if (mod10 >= 2 && mod10 <= 4 && (mod100 < 12 || mod100 > 14)) {
        word = few;
    }
    return QStringLiteral("%1 %2").arg(count).arg(word);
}

[[nodiscard]] QFont uiFont(const QFont& base, qreal size, QFont::Weight weight = QFont::Normal)
{
    QFont font = base;
    font.setPointSizeF(size);
    font.setWeight(weight);
    return font;
}

} // namespace

class CalendarView final : public QWidget {
public:
    using DateCallback = std::function<void(const QDate&)>;
    using IdCallback = std::function<void(const QUuid&)>;

    explicit CalendarView(Store& store, QWidget* parent = nullptr)
        : QWidget(parent)
        , m_store(store)
    {
        setMouseTracking(true);
        setMinimumSize(300, 250);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_selectedDate = QDate::currentDate();
        m_month = QDate(m_selectedDate.year(), m_selectedDate.month(), 1);
        m_year = m_selectedDate.year();
    }

    void setCallbacks(DateCallback daySelected, DateCallback dayActivated,
                      DateCallback monthActivated)
    {
        m_onDaySelected = std::move(daySelected);
        m_onDayActivated = std::move(dayActivated);
        m_onMonthActivated = std::move(monthActivated);
    }

    void setMonth(const QDate& date)
    {
        if (!date.isValid()) {
            return;
        }
        m_month = QDate(date.year(), date.month(), 1);
        update();
    }

    void setYear(int year)
    {
        m_year = year;
        update();
    }

    void setSelectedDate(const QDate& date)
    {
        m_selectedDate = date;
        update();
    }

    void setYearMode(bool yearMode)
    {
        m_yearMode = yearMode;
        update();
    }

    void setCompact(bool compact)
    {
        if (m_compact != compact) {
            m_compact = compact;
            update();
        }
    }

    [[nodiscard]] QDate selectedDate() const { return m_selectedDate; }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event)
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        if (m_yearMode) {
            paintYear(painter);
        } else {
            paintMonth(painter);
        }
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
        if (m_yearMode) {
            const int month = monthAt(event->position().toPoint());
            if (month != m_hoverMonth) {
                m_hoverMonth = month;
                setCursor(month > 0 ? Qt::PointingHandCursor : Qt::ArrowCursor);
                update();
            }
            return;
        }
        const QDate date = dateAt(event->position().toPoint());
        if (date != m_hoverDate) {
            m_hoverDate = date;
            update();
        }
    }

    void leaveEvent(QEvent* event) override
    {
        Q_UNUSED(event)
        m_hoverDate = QDate();
        m_hoverMonth = -1;
        update();
    }

    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() != Qt::LeftButton) {
            return;
        }
        if (m_yearMode) {
            const int month = monthAt(event->position().toPoint());
            if (month > 0 && m_onMonthActivated != nullptr) {
                m_onMonthActivated(QDate(m_year, month, 1));
            }
            return;
        }
        const QDate date = dateAt(event->position().toPoint());
        if (!date.isValid()) {
            return;
        }
        m_selectedDate = date;
        update();
        if (m_onDaySelected != nullptr) {
            m_onDaySelected(date);
        }
    }

    void mouseDoubleClickEvent(QMouseEvent* event) override
    {
        if (m_yearMode || event->button() != Qt::LeftButton) {
            return;
        }
        const QDate date = dateAt(event->position().toPoint());
        if (date.isValid() && m_onDayActivated != nullptr) {
            m_onDayActivated(date);
        }
    }

private:
    [[nodiscard]] int weekColumnWidth() const
    {
        return Settings::instance().showWeekNumbers() ? (m_compact ? 26 : 32) : 0;
    }

    [[nodiscard]] qreal headerHeight() const { return m_compact ? 26.0 : 30.0; }

    [[nodiscard]] QDate gridStart() const
    {
        const int startDay = Settings::instance().weekStartsMonday() ? 1 : 7;
        const int offset = (m_month.dayOfWeek() - startDay + 7) % 7;
        return m_month.addDays(-offset);
    }

    [[nodiscard]] QRectF monthCellRect(int row, int column) const
    {
        const qreal header = headerHeight();
        const qreal weekWidth = weekColumnWidth();
        const qreal dayWidth = (width() - weekWidth) / 7.0;
        const qreal rowHeight = (height() - header) / 6.0;
        return QRectF(weekWidth + column * dayWidth, header + row * rowHeight, dayWidth, rowHeight);
    }

    [[nodiscard]] QDate dateAt(const QPoint& point) const
    {
        const qreal header = headerHeight();
        if (point.y() < header) {
            return {};
        }
        const qreal weekWidth = weekColumnWidth();
        if (point.x() < weekWidth) {
            return {};
        }
        const qreal dayWidth = (width() - weekWidth) / 7.0;
        const qreal rowHeight = (height() - header) / 6.0;
        if (dayWidth <= 0.0 || rowHeight <= 0.0) {
            return {};
        }
        const int column = static_cast<int>((point.x() - weekWidth) / dayWidth);
        const int row = static_cast<int>((point.y() - header) / rowHeight);
        if (row < 0 || row > 5 || column < 0 || column > 6) {
            return {};
        }
        return gridStart().addDays(row * 7 + column);
    }

    [[nodiscard]] QRectF miniMonthRect(int month) const
    {
        constexpr qreal margin = 12.0;
        constexpr qreal gap = 14.0;
        const qreal widthAvail = width() - margin * 2 - gap * 2;
        const qreal heightAvail = height() - margin * 2 - gap * 3;
        const qreal miniWidth = widthAvail / 3.0;
        const qreal miniHeight = heightAvail / 4.0;
        const int column = month % 3;
        const int row = month / 3;
        return QRectF(margin + column * (miniWidth + gap), margin + row * (miniHeight + gap),
                      miniWidth, miniHeight);
    }

    [[nodiscard]] int monthAt(const QPoint& point) const
    {
        for (int month = 1; month <= 12; ++month) {
            if (miniMonthRect(month - 1).contains(point)) {
                return month;
            }
        }
        return -1;
    }

    void paintMonth(QPainter& painter)
    {
        const ThemeColors& colors = ThemeManager::instance().current();
        const bool weekNumbers = Settings::instance().showWeekNumbers();
        const bool mondayStart = Settings::instance().weekStartsMonday();
        const bool showAlarms = Settings::instance().calendarShowAlarms();
        const bool showTasks = Settings::instance().calendarShowTasks();
        const QDate today = QDate::currentDate();
        const QDate start = gridStart();
        const qreal headerSize = headerHeight();
        const qreal weekWidth = weekColumnWidth();
        const qreal dayWidth = (width() - weekWidth) / 7.0;
        const qreal rowHeight = (height() - headerSize) / 6.0;

        painter.fillRect(rect(), colors.card);
        painter.fillRect(QRectF(0, 0, width(), headerSize), tinted(colors.headerTop, 0.92));
        painter.setPen(QPen(colors.grid, 1.0));
        painter.drawLine(0, headerSize, width(), headerSize);

        QFont headerFont = uiFont(font(), m_compact ? 7.4 : 8.2, QFont::DemiBold);
        painter.setFont(headerFont);
        const int startDay = mondayStart ? 1 : 7;
        for (int column = 0; column < 7; ++column) {
            const int day = ((startDay - 1 + column) % 7) + 1;
            const QRectF cell(weekWidth + column * dayWidth, 0, dayWidth, headerSize);
            painter.setPen(day == 6 || day == 7 ? colors.danger : colors.headerText);
            painter.drawText(cell, Qt::AlignCenter, catalog::dayShort(static_cast<Qt::DayOfWeek>(day)));
        }
        if (weekNumbers) {
            painter.setPen(colors.faint);
            painter.drawText(QRectF(0, 0, weekWidth, headerSize), Qt::AlignCenter,
                             QStringLiteral("№"));
            painter.setPen(QPen(colors.grid, 1.0));
            painter.drawLine(weekWidth, 0, weekWidth, height());
        }

        for (int row = 0; row < 6; ++row) {
            if (weekNumbers) {
                const QDate weekDate = start.addDays(row * 7);
                const QRectF weekCell(0, headerSize + row * rowHeight, weekWidth, rowHeight);
                painter.fillRect(weekCell, tinted(colors.panel, 0.55));
                QFont weekFont = uiFont(font(), 7.2);
                painter.setFont(weekFont);
                painter.setPen(colors.faint);
                painter.drawText(weekCell, Qt::AlignCenter,
                                 QString::number(weekDate.weekNumber()));
                painter.setPen(QPen(colors.grid, 1.0));
                painter.drawLine(weekCell.left(), weekCell.bottom(), weekCell.right(),
                                 weekCell.bottom());
            }
            for (int column = 0; column < 7; ++column) {
                const QDate date = start.addDays(row * 7 + column);
                const QRectF cell = monthCellRect(row, column);
                const bool inMonth = date.month() == m_month.month() && date.year() == m_month.year();
                const bool weekend = date.dayOfWeek() == 6 || date.dayOfWeek() == 7;
                QColor background = inMonth ? colors.card : tinted(colors.panel, 0.55);
                if (inMonth && weekend) {
                    background = tinted(colors.danger, 0.035);
                }
                if (inMonth && date == m_hoverDate) {
                    background = tinted(colors.accent, 0.075);
                }
                if (date == m_selectedDate) {
                    background = tinted(colors.accent, 0.13);
                }
                painter.fillRect(cell, background);

                painter.setPen(QPen(colors.grid, 1.0));
                painter.drawLine(cell.left(), cell.bottom(), cell.right(), cell.bottom());
                painter.drawLine(cell.right(), cell.top(), cell.right(), cell.bottom());

                const qreal numberX = cell.left() + (m_compact ? 7.0 : 8.0);
                const qreal numberY = cell.top() + (m_compact ? 4.0 : 6.0);
                QFont numberFont = uiFont(font(), m_compact ? 7.8 : 8.6, QFont::DemiBold);
                painter.setFont(numberFont);
                if (date == today) {
                    const qreal diameter = m_compact ? 18.0 : 23.0;
                    const QRectF circle(cell.left() + (m_compact ? 5.0 : 7.0),
                                        cell.top() + (m_compact ? 2.0 : 5.0), diameter, diameter);
                    painter.setPen(Qt::NoPen);
                    painter.setBrush(colors.accent);
                    painter.drawEllipse(circle);
                    painter.setPen(QColor(255, 255, 255));
                    painter.drawText(circle, Qt::AlignCenter, QString::number(date.day()));
                } else {
                    painter.setPen(inMonth ? (weekend ? colors.danger : colors.text) : colors.faint);
                    painter.drawText(QRectF(numberX, numberY, 26.0, m_compact ? 16.0 : 18.0),
                                     Qt::AlignLeft | Qt::AlignVCenter,
                                     QString::number(date.day()));
                }

                if (inMonth) {
                    const QVector<CalendarEvent> events = collectCalendarEvents(m_store, date, showAlarms,
                                                                        showTasks);
                    if (!events.isEmpty() && m_compact) {
                        const qreal dotY = cell.top() + 26.0;
                        int shown = 0;
                        const int maxDots = std::min<int>(4, static_cast<int>((cell.width() - 14.0) / 8.0));
                        for (const auto& event : events) {
                            if (shown >= maxDots) {
                                break;
                            }
                            painter.setPen(Qt::NoPen);
                            painter.setBrush(event.done ? tinted(event.color, 0.45) : event.color);
                            painter.drawEllipse(QPointF(cell.left() + 11.0 + shown * 8.0, dotY),
                                                2.4, 2.4);
                            ++shown;
                        }
                        if (events.size() > shown) {
                            QFont moreFont = uiFont(font(), 6.2, QFont::DemiBold);
                            painter.setFont(moreFont);
                            painter.setPen(colors.faint);
                            painter.drawText(QRectF(cell.left() + 11.0 + shown * 8.0, dotY - 7.0,
                                                    14.0, 14.0),
                                             Qt::AlignLeft | Qt::AlignVCenter,
                                             QStringLiteral("+"));
                        }
                    } else if (!events.isEmpty()) {
                        const qreal firstLine = cell.top() + 31.0;
                        const qreal lineHeight = 15.0;
                        const int available = std::max(0, static_cast<int>((cell.height() - 36.0) / lineHeight));
                        const int shown = std::min<int>(events.size(), std::max(0, available));
                        QFont eventFont = uiFont(font(), 7.2, QFont::Normal);
                        QFont timeFont = uiFont(font(), 6.6, QFont::Medium);
                        for (int i = 0; i < shown; ++i) {
                            const CalendarEvent& event = events.at(i);
                            const qreal y = firstLine + i * lineHeight;
                            if (y + lineHeight > cell.bottom() - 2.0) {
                                break;
                            }
                            QColor color = event.done ? tinted(event.color, 0.5) : event.color;
                            painter.setPen(Qt::NoPen);
                            painter.setBrush(color);
                            painter.drawRoundedRect(QRectF(cell.left() + 7.0, y + 3.0, 2.5, 9.0),
                                                    1.2, 1.2);
                            qreal textX = cell.left() + 13.0;
                            if (cell.width() > 118.0) {
                                painter.setFont(timeFont);
                                painter.setPen(event.done ? colors.faint : colors.dim);
                                const QString timeText = schedule::timeText(event.time,
                                                                            Settings::instance().use24h());
                                painter.drawText(QRectF(textX, y, 30.0, lineHeight),
                                                 Qt::AlignLeft | Qt::AlignVCenter, timeText);
                                textX += 31.0;
                            }
                            painter.setFont(eventFont);
                            painter.setPen(event.done ? colors.faint : colors.text);
                            const QFontMetrics metrics(eventFont);
                            const int availableWidth = static_cast<int>(cell.right() - textX - 5.0);
                            const QString title = metrics.elidedText(event.title, Qt::ElideRight,
                                                                     std::max(10, availableWidth));
                            painter.drawText(QRectF(textX, y, availableWidth, lineHeight),
                                             Qt::AlignLeft | Qt::AlignVCenter, title);
                        }
                        if (events.size() > shown) {
                            QFont moreFont = uiFont(font(), 6.9);
                            painter.setFont(moreFont);
                            painter.setPen(colors.faint);
                            painter.drawText(QRectF(cell.left() + 13.0,
                                                    firstLine + shown * lineHeight,
                                                    cell.width() - 18.0, 13.0),
                                             Qt::AlignLeft | Qt::AlignVCenter,
                                             tr("+%1 ещё").arg(events.size() - shown));
                        }
                    }
                }

                if (date == m_selectedDate) {
                    painter.setPen(QPen(colors.accent, 1.4));
                    painter.setBrush(Qt::NoBrush);
                    painter.drawRoundedRect(cell.adjusted(1.2, 1.2, -1.2, -1.2), 3.0, 3.0);
                }
            }
        }
    }

    void paintYear(QPainter& painter)
    {
        const ThemeColors& colors = ThemeManager::instance().current();
        const bool showAlarms = Settings::instance().calendarShowAlarms();
        const bool showTasks = Settings::instance().calendarShowTasks();
        const QDate today = QDate::currentDate();
        painter.fillRect(rect(), colors.card);

        for (int month = 1; month <= 12; ++month) {
            const QRectF box = miniMonthRect(month - 1);
            QPainterPath path;
            path.addRoundedRect(box, 6.0, 6.0);
            painter.fillPath(path, month == m_hoverMonth ? tinted(colors.hoverRow, 0.85)
                                                          : tinted(colors.panel, 0.35));
            const bool selectedMonth = m_selectedDate.year() == m_year
                                       && m_selectedDate.month() == month;
            painter.setPen(QPen(selectedMonth ? colors.accent : colors.grid,
                                selectedMonth ? 1.5 : 1.0));
            painter.setBrush(Qt::NoBrush);
            painter.drawPath(path);

            QFont monthFont = uiFont(font(), 9.2, QFont::DemiBold);
            painter.setFont(monthFont);
            painter.setPen(colors.text);
            painter.drawText(QRectF(box.left() + 8.0, box.top() + 5.0, box.width() - 16.0, 18.0),
                             Qt::AlignLeft | Qt::AlignVCenter, catalog::monthName(month));

            const QDate first(m_year, month, 1);
            const int startDay = Settings::instance().weekStartsMonday() ? 1 : 7;
            const int offset = (first.dayOfWeek() - startDay + 7) % 7;
            const QDate start = first.addDays(-offset);
            const qreal cellWidth = (box.width() - 16.0) / 7.0;
            const qreal headerTop = box.top() + 23.0;
            const qreal cellHeight = (box.height() - 32.0) / 6.0;

            QFont weekFont = uiFont(font(), 5.9);
            painter.setFont(weekFont);
            painter.setPen(colors.faint);
            for (int column = 0; column < 7; ++column) {
                const int day = ((startDay - 1 + column) % 7) + 1;
                painter.drawText(QRectF(box.left() + 8.0 + column * cellWidth, headerTop,
                                        cellWidth, 9.0),
                                 Qt::AlignCenter, catalog::dayShort(static_cast<Qt::DayOfWeek>(day)));
            }

            QFont dayFont = uiFont(font(), 6.3);
            for (int row = 0; row < 6; ++row) {
                for (int column = 0; column < 7; ++column) {
                    const QDate date = start.addDays(row * 7 + column);
                    const QRectF dayCell(box.left() + 8.0 + column * cellWidth,
                                         headerTop + 9.0 + row * cellHeight, cellWidth, cellHeight);
                    if (date.month() != month) {
                        continue;
                    }
                    const bool isToday = date == today;
                    painter.setFont(dayFont);
                    if (isToday) {
                        painter.setPen(Qt::NoPen);
                        painter.setBrush(colors.accent);
                        painter.drawEllipse(dayCell.center(), 6.8, 6.8);
                        painter.setPen(QColor(255, 255, 255));
                    } else {
                        painter.setPen(date.dayOfWeek() >= 6 ? colors.danger : colors.text);
                    }
                    painter.drawText(dayCell, Qt::AlignCenter, QString::number(date.day()));
                    const auto events = collectCalendarEvents(m_store, date, showAlarms, showTasks);
                    if (!events.isEmpty()) {
                        painter.setPen(Qt::NoPen);
                        painter.setBrush(events.first().color);
                        painter.drawEllipse(QPointF(dayCell.center().x(),
                                                    dayCell.bottom() - 1.5), 1.5, 1.5);
                    }
                }
            }
        }
    }

    Store& m_store;
    QDate m_month;
    QDate m_selectedDate;
    QDate m_hoverDate;
    int m_year{0};
    int m_hoverMonth{-1};
    bool m_yearMode{false};
    bool m_compact{false};
    DateCallback m_onDaySelected;
    DateCallback m_onDayActivated;
    DateCallback m_onMonthActivated;
};

namespace {

class AgendaRow final : public QWidget {
public:
    explicit AgendaRow(const CalendarEvent& event, QWidget* parent = nullptr)
        : QWidget(parent)
        , m_event(event)
    {
        setCursor(Qt::PointingHandCursor);
        setFixedHeight(54);
        setToolTip(event.subtitle);
        auto* layout = new QHBoxLayout(this);
        layout->setContentsMargins(8, 5, 8, 5);
        layout->setSpacing(8);

        auto* time = new QLabel(schedule::timeText(event.time, Settings::instance().use24h()), this);
        time->setObjectName(QStringLiteral("Mono"));
        time->setFont(fonts::digitFont(8.0, QFont::DemiBold));
        time->setStyleSheet(QStringLiteral("color:%1;").arg(event.color.name()));
        time->setFixedWidth(44);
        layout->addWidget(time);

        auto* chip = new QLabel(this);
        chip->setFixedSize(28, 28);
        chip->setAlignment(Qt::AlignCenter);
        chip->setStyleSheet(QStringLiteral("background: rgba(%1,%2,%3,34); border-radius: 21px;")
                                .arg(event.color.red())
                                .arg(event.color.green())
                                .arg(event.color.blue()));
        if (event.task) {
            chip->setPixmap(icons::pixmap(icons::Icon::Tasks, 15, event.color));
        } else {
            chip->setPixmap(emojiPixmap(event.emoji.isEmpty() ? QStringLiteral("⏰") : event.emoji, 15));
        }
        layout->addWidget(chip);

        auto* textColumn = new QVBoxLayout;
        textColumn->setSpacing(0);
        auto* title = new QLabel(this);
        QFont titleFont = title->font();
        titleFont.setPointSizeF(8.8);
        titleFont.setWeight(QFont::DemiBold);
        titleFont.setStrikeOut(event.done);
        title->setFont(titleFont);
        title->setText(QFontMetrics(titleFont).elidedText(event.title, Qt::ElideRight, 182));
        title->setStyleSheet(QStringLiteral("color:%1;")
                                 .arg(event.done ? ThemeManager::instance().current().faint.name()
                                                 : ThemeManager::instance().current().text.name()));
        textColumn->addWidget(title);
        auto* subtitle = new QLabel(this);
        subtitle->setObjectName(QStringLiteral("Muted"));
        subtitle->setText(QFontMetrics(subtitle->font())
                              .elidedText(event.subtitle, Qt::ElideRight, 182));
        textColumn->addWidget(subtitle);
        layout->addLayout(textColumn, 1);
    }

    void setClickCallback(std::function<void()> callback)
    {
        m_onClick = std::move(callback);
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event)
        if (m_hovered) {
            QPainter painter(this);
            painter.setRenderHint(QPainter::Antialiasing, true);
            painter.setPen(QPen(tinted(ThemeManager::instance().current().border, 0.6), 1.0));
            painter.setBrush(tinted(ThemeManager::instance().current().hoverRow, 0.85));
            painter.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), 5.0, 5.0);
        }
    }

    void mouseReleaseEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton && rect().contains(event->position().toPoint())
            && m_onClick != nullptr) {
            m_onClick();
        }
        QWidget::mouseReleaseEvent(event);
    }

    void enterEvent(QEnterEvent* event) override
    {
        m_hovered = true;
        update();
        QWidget::enterEvent(event);
    }

    void leaveEvent(QEvent* event) override
    {
        m_hovered = false;
        update();
        QWidget::leaveEvent(event);
    }

private:
    CalendarEvent m_event;
    std::function<void()> m_onClick;
    bool m_hovered{false};
};

} // namespace

CalendarPage::CalendarPage(Store& store, QWidget* parent)
    : PageWidget(parent)
    , m_store(store)
{
    m_selectedDate = QDate::currentDate();
    m_displayMonth = QDate(m_selectedDate.year(), m_selectedDate.month(), 1);
    m_displayYear = m_selectedDate.year();
    m_lastToday = QDate::currentDate();
    buildUi();
    connect(&m_store, &Store::alarmsChanged, this, &CalendarPage::scheduleRefresh);
    connect(&m_store, &Store::todosChanged, this, &CalendarPage::scheduleRefresh);
    connect(&Settings::instance(), &Settings::changed, this, &CalendarPage::scheduleRefresh);
    m_compact = width() < 860;
    applyResponsiveState();
    refreshViews();
    updateHeader();
}

void CalendarPage::buildUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 12, 16, 12);
    root->setSpacing(10);

    auto* header = new QHBoxLayout;
    header->setSpacing(6);
    auto* prev = new ThemedToolButton(icons::Icon::ChevronLeft, tr("Предыдущий период"), this);
    prev->setFixedSize(28, 28);
    auto* next = new ThemedToolButton(icons::Icon::ChevronRight, tr("Следующий период"), this);
    next->setFixedSize(28, 28);
    header->addWidget(prev);
    header->addWidget(next);
    m_title = new QLabel(this);
    QFont titleFont = m_title->font();
    titleFont.setPointSizeF(16.0);
    titleFont.setWeight(QFont::DemiBold);
    m_title->setFont(titleFont);
    m_title->setMinimumWidth(0);
    m_title->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    header->addWidget(m_title);
    m_todayButton = new ThemedButton(tr("Сегодня"), icons::Icon::Calendar, this);
    m_todayButton->setMinimumHeight(28);
    header->addWidget(m_todayButton);
    header->addStretch(1);

    m_viewCombo = new QComboBox(this);
    m_viewCombo->setMinimumWidth(172);
    m_viewCombo->addItem(tr("Месяц"), static_cast<int>(Mode::Month));
    m_viewCombo->addItem(tr("Неделя"), static_cast<int>(Mode::Week));
    m_viewCombo->addItem(tr("День"), static_cast<int>(Mode::Day));
    m_viewCombo->addItem(tr("Год"), static_cast<int>(Mode::Year));
    m_viewCombo->addItem(tr("Повестка"), static_cast<int>(Mode::Agenda));
    m_viewCombo->addItem(tr("Таймлайн"), static_cast<int>(Mode::Timeline));
    m_viewCombo->addItem(tr("4 недели"), static_cast<int>(Mode::MultiWeek));
    m_viewCombo->addItem(tr("Несколько месяцев"), static_cast<int>(Mode::MultiMonth));
    header->addWidget(m_viewCombo);
    m_toolsButton = new ThemedButton(tr("Инструменты"), icons::Icon::Calendar, this);
    m_toolsButton->setMinimumHeight(28);
    m_toolsButton->setToolTip(tr("Арифметика дат, время и часовые пояса"));
    connect(m_toolsButton, &QPushButton::clicked, this, [this] {
        DateToolsDialog dialog(this);
        dialog.exec();
    });
    header->addWidget(m_toolsButton);
    m_addAlarmButton = new ThemedButton(tr("Будильник"), icons::Icon::AlarmPlus, this, true);
    m_addAlarmButton->setMinimumHeight(28);
    header->addWidget(m_addAlarmButton);
    root->addLayout(header);

    auto* content = new QHBoxLayout;
    content->setSpacing(10);

    m_calendarCard = makeCard(this);
    auto* calendarLayout = new QVBoxLayout(m_calendarCard);
    calendarLayout->setContentsMargins(8, 8, 8, 8);
    m_viewStack = new QStackedWidget(m_calendarCard);
    m_viewStack->setObjectName(QStringLiteral("Transparent"));
    m_view = new CalendarView(m_store, m_viewStack);
    m_viewStack->addWidget(m_view);
    m_weekBoard = new WeekBoard(m_store, m_viewStack);
    m_viewStack->addWidget(m_weekBoard);
    m_dayBoard = new DayBoard(m_store, m_viewStack);
    m_viewStack->addWidget(m_dayBoard);
    m_agendaBoard = new AgendaBoard(m_store, m_viewStack);
    m_viewStack->addWidget(m_agendaBoard);
    buildMultiMonth();
    calendarLayout->addWidget(m_viewStack);
    content->addWidget(m_calendarCard, 1);

    m_agendaCard = makeCard(this);
    m_agendaCard->setFixedWidth(326);
    auto* agendaLayout = new QVBoxLayout(m_agendaCard);
    agendaLayout->setContentsMargins(16, 14, 16, 14);
    agendaLayout->setSpacing(6);

    auto* dayHeader = new QHBoxLayout;
    dayHeader->setSpacing(10);
    m_agendaDay = new QLabel(m_agendaCard);
    QFont dayFont = m_agendaDay->font();
    dayFont.setPointSizeF(28.0);
    dayFont.setWeight(QFont::Light);
    m_agendaDay->setFont(dayFont);
    m_agendaDay->setFixedWidth(52);
    m_agendaDay->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    dayHeader->addWidget(m_agendaDay);
    auto* dateColumn = new QVBoxLayout;
    dateColumn->setSpacing(0);
    m_agendaWeekday = new QLabel(m_agendaCard);
    QFont weekdayFont = m_agendaWeekday->font();
    weekdayFont.setPointSizeF(10.5);
    weekdayFont.setWeight(QFont::DemiBold);
    m_agendaWeekday->setFont(weekdayFont);
    dateColumn->addWidget(m_agendaWeekday);
    m_agendaDate = new QLabel(m_agendaCard);
    m_agendaDate->setObjectName(QStringLiteral("Muted"));
    dateColumn->addWidget(m_agendaDate);
    dayHeader->addLayout(dateColumn, 1);
    m_todayPill = new QLabel(tr("Сегодня"), m_agendaCard);
    m_todayPill->setAlignment(Qt::AlignCenter);
    m_todayPill->setFixedHeight(22);
    m_todayPill->setStyleSheet(
        QStringLiteral("background: rgba(42,102,165,32); border: 1px solid rgba(42,102,165,100);"
                       " border-radius: 10px; padding: 1px 9px; color: %1;")
            .arg(ThemeManager::instance().current().accent.name()));
    dayHeader->addWidget(m_todayPill, 0, Qt::AlignTop);
    agendaLayout->addLayout(dayHeader);

    m_agendaSummary = new QLabel(m_agendaCard);
    m_agendaSummary->setObjectName(QStringLiteral("Muted"));
    agendaLayout->addWidget(m_agendaSummary);
    agendaLayout->addWidget(makeHorizontalRule(m_agendaCard));
    agendaLayout->addSpacing(2);

    m_agendaScroll = new QScrollArea(m_agendaCard);
    m_agendaScroll->setWidgetResizable(true);
    m_agendaScroll->setFrameShape(QFrame::NoFrame);
    m_agendaScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_agendaRowsHost = new QWidget(m_agendaScroll);
    m_agendaRowsHost->setObjectName(QStringLiteral("Transparent"));
    m_agendaRowsLayout = new QVBoxLayout(m_agendaRowsHost);
    m_agendaRowsLayout->setContentsMargins(0, 0, 4, 0);
    m_agendaRowsLayout->setSpacing(2);
    m_agendaRowsLayout->addStretch(1);
    m_agendaScroll->setWidget(m_agendaRowsHost);
    agendaLayout->addWidget(m_agendaScroll, 1);

    auto* buttons = new QHBoxLayout;
    buttons->setSpacing(6);
    auto* agendaAlarm = new ThemedButton(tr("Будильник"), icons::Icon::AlarmPlus, m_agendaCard, true);
    agendaAlarm->setMinimumHeight(28);
    auto* agendaTask = new ThemedButton(tr("Задача"), icons::Icon::TaskPlus, m_agendaCard);
    agendaTask->setMinimumHeight(28);
    buttons->addWidget(agendaAlarm);
    buttons->addWidget(agendaTask);
    buttons->addStretch(1);
    agendaLayout->addLayout(buttons);
    content->addWidget(m_agendaCard);
    root->addLayout(content, 1);

    connect(prev, &QToolButton::clicked, this, [this] { navigate(-1); });
    connect(next, &QToolButton::clicked, this, [this] { navigate(1); });
    connect(m_todayButton, &QPushButton::clicked, this, &CalendarPage::goToday);
    connect(m_viewCombo, &QComboBox::currentIndexChanged, this, [this] {
        setMode(static_cast<Mode>(m_viewCombo->currentData().toInt()));
    });
    connect(m_addAlarmButton, &QPushButton::clicked, this, [this] {
        emit addAlarmRequested(m_selectedDate.isValid() ? m_selectedDate : QDate::currentDate());
    });
    connect(agendaAlarm, &QPushButton::clicked, this, [this] {
        emit addAlarmRequested(m_selectedDate.isValid() ? m_selectedDate : QDate::currentDate());
    });
    connect(agendaTask, &QPushButton::clicked, this, [this] {
        emit addTaskRequested(m_selectedDate.isValid() ? m_selectedDate : QDate::currentDate());
    });

    m_view->setCallbacks(
        [this](const QDate& date) { setSelectedDate(date); },
        [this](const QDate& date) {
            setSelectedDate(date);
            emit addAlarmRequested(date);
        },
        [this](const QDate& date) {
            setMode(Mode::Month);
            setMonth(date);
        });
    m_weekBoard->setCallbacks([this](const QDate& date) { setSelectedDate(date); },
                              [this](const QDate& date) {
                                  setSelectedDate(date);
                                  emit addAlarmRequested(date);
                              });
    m_agendaBoard->setEventCallback([this](const QUuid& id, bool task) {
        if (task) {
            emit editTaskRequested(id);
        } else {
            emit editAlarmRequested(id);
        }
    });
    m_dayBoard->setEventCallback([this](const QUuid& id, bool task) {
        if (task) {
            emit editTaskRequested(id);
        } else {
            emit editAlarmRequested(id);
        }
    });
}

void CalendarPage::buildMultiMonth()
{
    m_multiMonthScroll = new QScrollArea(m_viewStack);
    m_multiMonthScroll->setWidgetResizable(true);
    m_multiMonthScroll->setFrameShape(QFrame::NoFrame);
    m_multiMonthScroll->setObjectName(QStringLiteral("Transparent"));
    m_multiMonthScroll->viewport()->setAutoFillBackground(false);

    auto* content = new QWidget(m_multiMonthScroll);
    content->setObjectName(QStringLiteral("Transparent"));
    auto* grid = new QGridLayout(content);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(8);
    grid->setVerticalSpacing(8);

    for (int i = 0; i < 4; ++i) {
        auto* card = makeCard(content);
        auto* layout = new QVBoxLayout(card);
        layout->setContentsMargins(10, 7, 10, 9);
        layout->setSpacing(3);
        auto* label = new QLabel(card);
        QFont monthFont = label->font();
        monthFont.setPointSizeF(10.0);
        monthFont.setWeight(QFont::DemiBold);
        label->setFont(monthFont);
        layout->addWidget(label);
        auto* view = new CalendarView(m_store, card);
        view->setCompact(true);
        view->setMinimumSize(300, 220);
        view->setCallbacks(
            [this](const QDate& date) { setSelectedDate(date); },
            [this](const QDate& date) {
                setSelectedDate(date);
                emit addAlarmRequested(date);
            },
            [this](const QDate& date) {
                setMode(Mode::Month);
                setMonth(date);
            });
        layout->addWidget(view, 1);
        grid->addWidget(card, i / 2, i % 2);
        m_multiViews.append(view);
        m_multiLabels.append(label);
    }
    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 1);
    grid->setRowStretch(0, 1);
    grid->setRowStretch(1, 1);
    m_multiMonthScroll->setWidget(content);
    m_viewStack->addWidget(m_multiMonthScroll);
}

void CalendarPage::refreshViews()
{
    switch (m_mode) {
    case Mode::Month:
        m_view->setYearMode(false);
        m_view->setMonth(m_displayMonth);
        m_view->setSelectedDate(m_selectedDate);
        m_viewStack->setCurrentWidget(m_view);
        break;
    case Mode::Year:
        m_view->setYearMode(true);
        m_view->setYear(m_displayYear);
        m_view->setSelectedDate(m_selectedDate);
        m_viewStack->setCurrentWidget(m_view);
        break;
    case Mode::Week:
        m_weekBoard->setWeeks(1);
        m_weekBoard->setStartDate(weekStartDate(m_selectedDate));
        m_weekBoard->setSelectedDate(m_selectedDate);
        m_viewStack->setCurrentWidget(m_weekBoard);
        break;
    case Mode::MultiWeek:
        m_weekBoard->setWeeks(4);
        m_weekBoard->setStartDate(m_multiWeekStart);
        m_weekBoard->setSelectedDate(m_selectedDate);
        m_viewStack->setCurrentWidget(m_weekBoard);
        break;
    case Mode::Day:
        m_dayBoard->setDate(m_selectedDate);
        m_viewStack->setCurrentWidget(m_dayBoard);
        break;
    case Mode::Agenda:
        m_agendaBoard->setStyle(AgendaBoard::Style::Agenda);
        m_agendaBoard->setRange(m_selectedDate, 30);
        m_viewStack->setCurrentWidget(m_agendaBoard);
        break;
    case Mode::Timeline:
        m_agendaBoard->setStyle(AgendaBoard::Style::Timeline);
        m_agendaBoard->setRange(m_selectedDate, 14);
        m_viewStack->setCurrentWidget(m_agendaBoard);
        break;
    case Mode::MultiMonth:
        for (int i = 0; i < m_multiViews.size(); ++i) {
            const QDate month = m_displayMonth.addMonths(i);
            m_multiLabels.at(i)->setText(QStringLiteral("%1 %2")
                                             .arg(catalog::monthName(month.month()))
                                             .arg(month.year()));
            auto* view = m_multiViews.at(i);
            view->setMonth(month);
            view->setSelectedDate(m_selectedDate);
        }
        m_viewStack->setCurrentWidget(m_multiMonthScroll);
        break;
    }

    const bool sideAgenda = (m_mode == Mode::Month || m_mode == Mode::Week
                             || m_mode == Mode::MultiWeek)
                            && !m_compact;
    m_agendaCard->setVisible(sideAgenda);
    if (sideAgenda) {
        updateAgenda();
    }
}

void CalendarPage::resizeEvent(QResizeEvent* event)
{
    PageWidget::resizeEvent(event);
    const bool compact = width() < 860;
    if (compact == m_compact) {
        applyTitleText();
        return;
    }
    m_compact = compact;
    applyResponsiveState();
    refreshViews();
}

void CalendarPage::applyResponsiveState()
{
    if (m_todayButton != nullptr) {
        m_todayButton->setText(m_compact ? QString() : tr("Сегодня"));
        m_todayButton->setToolTip(tr("Сегодня"));
    }
    if (m_toolsButton != nullptr) {
        m_toolsButton->setText(m_compact ? QString() : tr("Инструменты"));
        m_toolsButton->setToolTip(tr("Арифметика дат, время и часовые пояса"));
    }
    if (m_addAlarmButton != nullptr) {
        m_addAlarmButton->setText(m_compact ? QString() : tr("Будильник"));
        m_addAlarmButton->setToolTip(tr("Новый будильник"));
    }
    if (m_viewCombo != nullptr) {
        m_viewCombo->setMinimumWidth(m_compact ? 118 : 172);
    }
    if (m_title != nullptr) {
        m_title->setMinimumWidth(m_compact ? 80 : 120);
    }
    applyTitleText();
}

void CalendarPage::applyTitleText()
{
    if (m_title == nullptr) {
        return;
    }
    if (m_titleText.isEmpty()) {
        m_title->clear();
        return;
    }
    const int available = m_compact ? 150 : 340;
    const QFontMetrics metrics(m_title->font());
    m_title->setText(metrics.elidedText(m_titleText, Qt::ElideRight, available));
    m_title->setToolTip(m_titleText);
}

void CalendarPage::updateHeader()
{
    QString title;
    switch (m_mode) {
    case Mode::Month:
        title = QStringLiteral("%1 %2")
                    .arg(catalog::monthName(m_displayMonth.month()))
                    .arg(m_displayMonth.year());
        break;
    case Mode::Year:
        title = QString::number(m_displayYear);
        break;
    case Mode::Week:
        title = rangeText(weekStartDate(m_selectedDate), weekStartDate(m_selectedDate).addDays(6));
        break;
    case Mode::MultiWeek:
        title = rangeText(m_multiWeekStart, m_multiWeekStart.addDays(27));
        break;
    case Mode::Day:
        title = QStringLiteral("%1, %2")
                    .arg(catalog::dayLong(static_cast<Qt::DayOfWeek>(m_selectedDate.dayOfWeek())))
                    .arg(schedule::dateText(m_selectedDate));
        break;
    case Mode::Agenda:
        title = rangeText(m_selectedDate, m_selectedDate.addDays(29));
        break;
    case Mode::Timeline:
        title = rangeText(m_selectedDate, m_selectedDate.addDays(13));
        break;
    case Mode::MultiMonth: {
        const QDate last = m_displayMonth.addMonths(3);
        if (last.year() == m_displayMonth.year()) {
            title = QStringLiteral("%1 — %2 %3")
                        .arg(catalog::monthName(m_displayMonth.month()),
                             catalog::monthName(last.month()))
                        .arg(last.year());
        } else {
            title = QStringLiteral("%1 %2 — %3 %4")
                        .arg(catalog::monthName(m_displayMonth.month()))
                        .arg(m_displayMonth.year())
                        .arg(catalog::monthName(last.month()))
                        .arg(last.year());
        }
        break;
    }
    }
    m_titleText = title;
    applyTitleText();
    updateViewCombo();
}

void CalendarPage::updateViewCombo()
{
    if (m_viewCombo == nullptr) {
        return;
    }
    const QSignalBlocker blocker(m_viewCombo);
    const int index = m_viewCombo->findData(static_cast<int>(m_mode));
    if (index >= 0) {
        m_viewCombo->setCurrentIndex(index);
    }
}

void CalendarPage::updateAgenda()
{
    if (!m_selectedDate.isValid()) {
        m_selectedDate = QDate::currentDate();
    }
    m_agendaDay->setText(QString::number(m_selectedDate.day()));
    m_agendaWeekday->setText(catalog::dayLong(static_cast<Qt::DayOfWeek>(m_selectedDate.dayOfWeek())));
    m_agendaDate->setText(QStringLiteral("%1 %2")
                              .arg(catalog::monthNameGenitive(m_selectedDate.month()))
                              .arg(m_selectedDate.year()));
    m_todayPill->setVisible(m_selectedDate == QDate::currentDate());
    m_todayPill->setStyleSheet(
        QStringLiteral("background: rgba(42,102,165,32); border: 1px solid rgba(42,102,165,100);"
                       " border-radius: 10px; padding: 1px 9px; color: %1;")
            .arg(ThemeManager::instance().current().accent.name()));

    const bool showAlarms = Settings::instance().calendarShowAlarms();
    const bool showTasks = Settings::instance().calendarShowTasks();
    const QVector<CalendarEvent> events = collectCalendarEvents(m_store, m_selectedDate, showAlarms,
                                                        showTasks);
    int alarms = 0;
    int tasks = 0;
    for (const auto& event : events) {
        if (event.task) {
            ++tasks;
        } else {
            ++alarms;
        }
    }
    QStringList summary;
    if (alarms > 0) {
        summary.append(countText(alarms, tr("будильник"), tr("будильника"), tr("будильников")));
    }
    if (tasks > 0) {
        summary.append(countText(tasks, tr("задача"), tr("задачи"), tr("задач")));
    }
    m_agendaSummary->setText(summary.isEmpty() ? tr("Событий нет") : summary.join(QStringLiteral(" · ")));
    rebuildAgendaRows();
}

void CalendarPage::rebuildAgendaRows()
{
    while (QLayoutItem* item = m_agendaRowsLayout->takeAt(0)) {
        if (item->widget() != nullptr) {
            delete item->widget();
        }
        delete item;
    }
    m_agendaRowsLayout->addStretch(1);

    const QVector<CalendarEvent> events =
        collectCalendarEvents(m_store, m_selectedDate, Settings::instance().calendarShowAlarms(),
                      Settings::instance().calendarShowTasks());
    if (events.isEmpty()) {
        auto* empty = new QWidget(m_agendaRowsHost);
        auto* layout = new QVBoxLayout(empty);
        layout->setContentsMargins(10, 20, 10, 16);
        layout->setSpacing(6);
        auto* icon = new QLabel(empty);
        icon->setAlignment(Qt::AlignHCenter);
        icon->setPixmap(icons::pixmap(icons::Icon::Calendar, 32,
                                      ThemeManager::instance().current().faint));
        layout->addWidget(icon);
        auto* title = new QLabel(tr("Свободный день"), empty);
        title->setAlignment(Qt::AlignHCenter);
        QFont font = title->font();
        font.setWeight(QFont::DemiBold);
        title->setFont(font);
        layout->addWidget(title);
        auto* hint = new QLabel(tr("Добавьте будильник или задачу на эту дату."), empty);
        hint->setObjectName(QStringLiteral("Faint"));
        hint->setAlignment(Qt::AlignHCenter);
        hint->setWordWrap(true);
        layout->addWidget(hint);
        m_agendaRowsLayout->insertWidget(0, empty);
        return;
    }

    const auto addSection = [this](const QString& title, const QVector<CalendarEvent>& section,
                                   bool task) {
        if (section.isEmpty()) {
            return;
        }
        auto* label = new QLabel(title, m_agendaRowsHost);
        label->setObjectName(QStringLiteral("SectionLabel"));
        label->setContentsMargins(8, 8, 0, 3);
        m_agendaRowsLayout->insertWidget(m_agendaRowsLayout->count() - 1, label);
        for (const auto& event : section) {
            auto* row = new AgendaRow(event, m_agendaRowsHost);
            if (task) {
                row->setClickCallback([this, id = event.id] { emit editTaskRequested(id); });
            } else {
                row->setClickCallback([this, id = event.id] { emit editAlarmRequested(id); });
            }
            m_agendaRowsLayout->insertWidget(m_agendaRowsLayout->count() - 1, row);
        }
    };
    QVector<CalendarEvent> alarmEvents;
    QVector<CalendarEvent> taskEvents;
    for (const auto& event : events) {
        (event.task ? taskEvents : alarmEvents).append(event);
    }
    addSection(tr("БУДИЛЬНИКИ"), alarmEvents, false);
    addSection(tr("ЗАДАЧИ"), taskEvents, true);
}

void CalendarPage::navigate(int delta)
{
    switch (m_mode) {
    case Mode::Month: {
        m_displayMonth = m_displayMonth.addMonths(delta);
        const int maxDay = m_displayMonth.daysInMonth();
        m_selectedDate = QDate(m_displayMonth.year(), m_displayMonth.month(),
                               std::min(m_selectedDate.day(), maxDay));
        break;
    }
    case Mode::Year:
        m_displayYear += delta;
        m_displayMonth = QDate(m_displayYear, m_selectedDate.month(), 1);
        m_selectedDate = QDate(m_displayYear, m_selectedDate.month(),
                               std::min(m_selectedDate.day(), m_displayMonth.daysInMonth()));
        break;
    case Mode::Week:
        m_selectedDate = m_selectedDate.addDays(7 * delta);
        break;
    case Mode::MultiWeek:
        m_multiWeekStart = m_multiWeekStart.addDays(28 * delta);
        m_selectedDate = m_multiWeekStart;
        break;
    case Mode::Day:
        m_selectedDate = m_selectedDate.addDays(delta);
        break;
    case Mode::Agenda:
        m_selectedDate = m_selectedDate.addDays(30 * delta);
        break;
    case Mode::Timeline:
        m_selectedDate = m_selectedDate.addDays(14 * delta);
        break;
    case Mode::MultiMonth:
        m_displayMonth = m_displayMonth.addMonths(4 * delta);
        m_selectedDate = QDate(m_displayMonth.year(), m_displayMonth.month(),
                               std::min(m_selectedDate.day(), m_displayMonth.daysInMonth()));
        break;
    }
    refreshViews();
    updateHeader();
}

void CalendarPage::goToday()
{
    const QDate today = QDate::currentDate();
    m_selectedDate = today;
    m_displayMonth = QDate(today.year(), today.month(), 1);
    m_displayYear = today.year();
    m_multiWeekStart = weekStartDate(today);
    refreshViews();
    updateHeader();
}

void CalendarPage::setMode(Mode mode)
{
    if (m_mode == mode) {
        updateViewCombo();
        return;
    }
    m_mode = mode;
    if (mode == Mode::Month || mode == Mode::MultiMonth || mode == Mode::Year) {
        m_displayMonth = QDate(m_selectedDate.year(), m_selectedDate.month(), 1);
        m_displayYear = m_selectedDate.year();
    }
    if (mode == Mode::MultiWeek) {
        m_multiWeekStart = weekStartDate(m_selectedDate);
    }
    refreshViews();
    updateHeader();
}

void CalendarPage::setSelectedDate(const QDate& date)
{
    if (!date.isValid()) {
        return;
    }
    m_selectedDate = date;
    if (m_mode == Mode::Month) {
        m_displayMonth = QDate(date.year(), date.month(), 1);
    }
    refreshViews();
    updateHeader();
}

void CalendarPage::setMonth(const QDate& date)
{
    if (!date.isValid()) {
        return;
    }
    m_mode = Mode::Month;
    m_displayMonth = QDate(date.year(), date.month(), 1);
    m_displayYear = date.year();
    m_selectedDate = date;
    refreshViews();
    updateHeader();
}

QDate CalendarPage::weekStartDate(const QDate& date) const
{
    const int firstDay = Settings::instance().weekStartsMonday() ? 1 : 7;
    const int offset = (date.dayOfWeek() - firstDay + 7) % 7;
    return date.addDays(-offset);
}

QString CalendarPage::rangeText(const QDate& from, const QDate& to) const
{
    QString left = QStringLiteral("%1 %2").arg(from.day()).arg(catalog::monthNameGenitive(from.month()));
    QString right = QStringLiteral("%1 %2").arg(to.day()).arg(catalog::monthNameGenitive(to.month()));
    if (from.year() == to.year()) {
        return QStringLiteral("%1 — %2 %3").arg(left, right).arg(to.year());
    }
    return QStringLiteral("%1 %2 — %3 %4").arg(left).arg(from.year()).arg(right).arg(to.year());
}

void CalendarPage::scheduleRefresh()
{
    QTimer::singleShot(0, this, [this] { refreshData(); });
}

void CalendarPage::refreshData()
{
    m_view->update();
    m_weekBoard->update();
    m_dayBoard->refresh();
    m_agendaBoard->refresh();
    for (auto* view : m_multiViews) {
        view->update();
    }
    const bool sideAgenda = m_mode == Mode::Month || m_mode == Mode::Week
                            || m_mode == Mode::MultiWeek;
    if (sideAgenda) {
        updateAgenda();
    }
}

void CalendarPage::refreshClock(const QDateTime& now)
{
    const QDate today = now.date();
    if (today != m_lastToday) {
        m_lastToday = today;
        m_view->update();
        m_weekBoard->update();
        m_agendaBoard->refresh();
        for (auto* view : m_multiViews) {
            view->update();
        }
        const bool sideAgenda = m_mode == Mode::Month || m_mode == Mode::Week
                                || m_mode == Mode::MultiWeek;
        if (sideAgenda) {
            updateAgenda();
        }
    }
    if (m_mode == Mode::Day && m_selectedDate == today) {
        m_dayBoard->tick();
    }
}

void CalendarPage::onActivated()
{
    m_lastToday = QDate::currentDate();
    refreshViews();
    updateHeader();
}

void CalendarPage::themeUpdated()
{
    m_view->update();
    m_weekBoard->update();
    m_dayBoard->refresh();
    m_agendaBoard->refresh();
    for (auto* view : m_multiViews) {
        view->update();
    }
    const bool sideAgenda = m_mode == Mode::Month || m_mode == Mode::Week
                            || m_mode == Mode::MultiWeek;
    if (sideAgenda) {
        updateAgenda();
    }
}

} // namespace pac::ui
