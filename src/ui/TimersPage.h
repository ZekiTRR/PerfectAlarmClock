#pragma once

#include "UiKit.h"

#include <QVector>
#include <QUuid>

class QHBoxLayout;
class QLabel;
class QPushButton;
class QScrollArea;
class QStackedWidget;
class QVBoxLayout;

namespace pac {
class Store;
}

namespace pac::ui {

class TimerCard;

class TimersPage final : public PageWidget {
    Q_OBJECT

public:
    explicit TimersPage(Store& store, QWidget* parent = nullptr);

    void refreshClock(const QDateTime& now) override;

public slots:
    void createTimer();

protected:
    void onActivated() override;
    void themeUpdated() override;
    void applyThemeMode() override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void buildUi();
    void rebuildCards();
    void updateEmptyState();
    void quickStart(int minutes);
    void startPause(const QUuid& id);
    void resetTimer(const QUuid& id);
    void editTimer(const QUuid& id);
    void removeTimer(const QUuid& id);
    void skipStage(const QUuid& id);
    void previousStage(const QUuid& id);
    void addMinute(const QUuid& id);
    void refreshCards(const QDateTime& now);

    Store& m_store;
    QStackedWidget* m_stack{nullptr};
    QScrollArea* m_scroll{nullptr};
    QHBoxLayout* m_toolbarLayout{nullptr};
    QWidget* m_pageHeader{nullptr};
    QLabel* m_pageTitle{nullptr};
    QLabel* m_quickLabel{nullptr};
    QVector<QPushButton*> m_quickButtons;
    QVBoxLayout* m_cardsLayout{nullptr};
    QWidget* m_empty{nullptr};
    QLabel* m_emptyTitle{nullptr};
    QLabel* m_emptyHint{nullptr};
    QVector<TimerCard*> m_cards;
    bool m_fluent{false};
};

} // namespace pac::ui
