#pragma once

#include "UiKit.h"

#include <QElapsedTimer>
#include <QVector>

class QLabel;
class QStackedWidget;
class QTableWidget;
class QTimer;

namespace pac::ui {

class StopwatchPage final : public PageWidget {
    Q_OBJECT

public:
    explicit StopwatchPage(QWidget* parent = nullptr);

    void refreshClock(const QDateTime& now) override;

public slots:
    void primaryAction();

protected:
    void onActivated() override;
    void onDeactivated() override;
    void themeUpdated() override;

private:
    void buildUi();
    void toggleRunning();
    void recordLap();
    void resetStopwatch();
    void updateDisplay();
    void updateStartButton();
    void updateStatePill();
    void updateLapsTable();
    void updateLapsPalette();
    [[nodiscard]] qint64 elapsedMs() const;

    QLabel* m_timeLabel{nullptr};
    QLabel* m_statePill{nullptr};
    QLabel* m_lapHeader{nullptr};
    ThemedButton* m_startButton{nullptr};
    ThemedButton* m_lapButton{nullptr};
    ThemedButton* m_resetButton{nullptr};
    QTableWidget* m_lapsTable{nullptr};
    QStackedWidget* m_lapsStack{nullptr};
    QTimer* m_refreshTimer{nullptr};
    QElapsedTimer m_elapsed;
    qint64 m_accumulatedMs{0};
    bool m_running{false};
    QVector<qint64> m_lapTimes;
    QVector<qint64> m_lapTotals;
    qint64 m_lastLapTotal{0};
};

} // namespace pac::ui
