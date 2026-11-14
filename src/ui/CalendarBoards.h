#pragma once

#include <QDate>
#include <QScrollArea>
#include <QUuid>
#include <QWidget>

#include <functional>

namespace pac {
class Store;
}

namespace pac::ui {

class WeekBoard final : public QWidget {
public:
    explicit WeekBoard(Store& store, QWidget* parent = nullptr);

    void setWeeks(int weeks);
    void setStartDate(const QDate& date);
    void setSelectedDate(const QDate& date);
    void setCallbacks(std::function<void(const QDate&)> daySelected,
                      std::function<void(const QDate&)> dayActivated);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    [[nodiscard]] QDate dateAt(const QPoint& point) const;
    [[nodiscard]] int headerHeight() const;
    [[nodiscard]] QRectF cellRect(int index) const;

    Store& m_store;
    int m_weeks{1};
    QDate m_start;
    QDate m_selected;
    QDate m_hover;
    std::function<void(const QDate&)> m_onDaySelected;
    std::function<void(const QDate&)> m_onDayActivated;
};

class DayBoard final : public QScrollArea {
public:
    explicit DayBoard(Store& store, QWidget* parent = nullptr);

    void setDate(const QDate& date);
    void setEventCallback(std::function<void(const QUuid&, bool)> callback);
    void refresh();
    void tick();

private:
    class Canvas;
    Canvas* m_canvas{nullptr};
    QDate m_date;
};

class AgendaBoard final : public QScrollArea {
public:
    enum class Style {
        Agenda,
        Timeline
    };

    explicit AgendaBoard(Store& store, QWidget* parent = nullptr);

    void setRange(const QDate& start, int days);
    void setStyle(Style style);
    void setEventCallback(std::function<void(const QUuid&, bool)> callback);
    void refresh();

private:
    class Canvas;
    Canvas* m_canvas{nullptr};
};

} // namespace pac::ui
