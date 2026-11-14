#pragma once

#include "NotificationPopup.h"
#include "UiKit.h"
#include "core/Types.h"

#include <QDateTime>
#include <QPointer>

class QBoxLayout;
class QFrame;
class QLabel;
class QScrollArea;
class QTimer;

namespace pac {
class Store;
}

namespace pac::ui {

class GoalBar;
class PomodoroRing;
class SessionDots;

class PomodoroPage final : public PageWidget {
    Q_OBJECT

public:
    explicit PomodoroPage(Store& store, QWidget* parent = nullptr);

    void refreshClock(const QDateTime& now) override;

public slots:
    void primaryAction();

protected:
    void onActivated() override;
    void onDeactivated() override;
    void themeUpdated() override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void buildUi();
    void startPause();
    void resetPhase();
    void skipPhase();
    void tick();
    void completePhase();
    void advancePhase(bool completedNaturally, bool startNext);
    void updateUi();
    void updateTimeDisplay();
    void updateStats();
    void updateSettingsSummary();
    void updatePhasePill();
    void ensureStatsToday();
    void openSettings();
    void applyResponsiveLayout();
    void showCompletionPopup(PomodoroPhase finished, bool nextRunning);
    [[nodiscard]] int phaseMinutes(PomodoroPhase phase) const;
    [[nodiscard]] qint64 phaseDurationMs(PomodoroPhase phase) const;
    [[nodiscard]] QColor phaseColor(PomodoroPhase phase) const;
    [[nodiscard]] QString phaseTitle(PomodoroPhase phase) const;
    [[nodiscard]] QString formatTime(qint64 milliseconds) const;

    Store& m_store;
    PomodoroPhase m_phase{PomodoroPhase::Work};
    bool m_running{false};
    qint64 m_phaseMs{25LL * 60 * 1000};
    qint64 m_remainingMs{25LL * 60 * 1000};
    QDateTime m_endAt;
    int m_completedInSet{0};

    PomodoroRing* m_ring{nullptr};
    SessionDots* m_dots{nullptr};
    QScrollArea* m_contentScroll{nullptr};
    QBoxLayout* m_contentLayout{nullptr};
    QFrame* m_statsCard{nullptr};
    bool m_narrow{false};
    QLabel* m_phaseLabel{nullptr};
    QLabel* m_timeLabel{nullptr};
    QLabel* m_sessionLabel{nullptr};
    QLabel* m_hintLabel{nullptr};
    QLabel* m_completedCount{nullptr};
    QLabel* m_goalCount{nullptr};
    QLabel* m_focusValue{nullptr};
    QLabel* m_remainingValue{nullptr};
    QLabel* m_workValue{nullptr};
    QLabel* m_shortValue{nullptr};
    QLabel* m_longValue{nullptr};
    QLabel* m_everyValue{nullptr};
    ThemedButton* m_startButton{nullptr};
    ThemedButton* m_skipButton{nullptr};
    ThemedButton* m_resetButton{nullptr};
    GoalBar* m_goalBar{nullptr};
    QTimer* m_timer{nullptr};
    QPointer<NotificationPopup> m_popup;
};

} // namespace pac::ui
