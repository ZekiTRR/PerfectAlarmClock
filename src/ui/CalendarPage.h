#pragma once

#include "UiKit.h"

#include <QDate>
#include <QUuid>
#include <QVector>

class QComboBox;
class QLabel;
class QScrollArea;
class QStackedWidget;
class QVBoxLayout;
class QWidget;

namespace pac {
class Store;
}

namespace pac::ui {

class AgendaBoard;
class CalendarView;
class DayBoard;
class WeekBoard;

class CalendarPage final : public PageWidget {
    Q_OBJECT

public:
    explicit CalendarPage(Store& store, QWidget* parent = nullptr);

    void refreshClock(const QDateTime& now) override;

signals:
    void addAlarmRequested(const QDate& date);
    void addTaskRequested(const QDate& date);
    void editAlarmRequested(const QUuid& id);
    void editTaskRequested(const QUuid& id);

protected:
    void onActivated() override;
    void themeUpdated() override;
    void resizeEvent(QResizeEvent* event) override;

private:
    enum class Mode {
        Month,
        Week,
        Day,
        Year,
        Agenda,
        Timeline,
        MultiWeek,
        MultiMonth
    };

    void buildUi();
    void buildMultiMonth();
    void updateHeader();
    void updateViewCombo();
    void updateAgenda();
    void rebuildAgendaRows();
    void navigate(int delta);
    void goToday();
    void setMode(Mode mode);
    void setSelectedDate(const QDate& date);
    void setMonth(const QDate& date);
    void refreshViews();
    void scheduleRefresh();
    void refreshData();
    void applyResponsiveState();
    void applyTitleText();
    [[nodiscard]] QDate weekStartDate(const QDate& date) const;
    [[nodiscard]] QString rangeText(const QDate& from, const QDate& to) const;

    Store& m_store;
    Mode m_mode{Mode::Month};
    CalendarView* m_view{nullptr};
    WeekBoard* m_weekBoard{nullptr};
    DayBoard* m_dayBoard{nullptr};
    AgendaBoard* m_agendaBoard{nullptr};
    QWidget* m_calendarCard{nullptr};
    QStackedWidget* m_viewStack{nullptr};
    QScrollArea* m_multiMonthScroll{nullptr};
    QVector<CalendarView*> m_multiViews;
    QVector<QLabel*> m_multiLabels;
    QWidget* m_agendaCard{nullptr};
    QLabel* m_title{nullptr};
    QComboBox* m_viewCombo{nullptr};
    QPushButton* m_todayButton{nullptr};
    QPushButton* m_toolsButton{nullptr};
    QPushButton* m_addAlarmButton{nullptr};
    QString m_titleText;
    bool m_compact{false};
    QLabel* m_agendaDay{nullptr};
    QLabel* m_agendaWeekday{nullptr};
    QLabel* m_agendaDate{nullptr};
    QLabel* m_todayPill{nullptr};
    QLabel* m_agendaSummary{nullptr};
    QScrollArea* m_agendaScroll{nullptr};
    QWidget* m_agendaRowsHost{nullptr};
    QVBoxLayout* m_agendaRowsLayout{nullptr};
    QDate m_selectedDate;
    QDate m_displayMonth;
    QDate m_multiWeekStart;
    int m_displayYear{0};
    QDate m_lastToday;
};

} // namespace pac::ui
