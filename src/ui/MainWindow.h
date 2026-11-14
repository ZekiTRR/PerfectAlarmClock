#pragma once

#include "core/Icons.h"
#include "core/Scheduler.h"
#include "core/Store.h"

#include <QAbstractNativeEventFilter>
#include <QColor>
#include <QHash>
#include <QMainWindow>
#include <QPointer>
#include <QVector>

class QLabel;
class QLineEdit;
class QMenu;
class QStackedWidget;
class QSystemTrayIcon;
class QTimer;
class QToolButton;
class QAction;

namespace pac::ui {

class NotificationPopup;
class RibbonButton;
class ThemeEffects;
class TitleBar;

class NavPane;
class PageWidget;
class RibbonBar;

class MainWindow final : public QMainWindow, public QAbstractNativeEventFilter {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    void showPage(const QString& id);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;
    bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override;
    void closeEvent(QCloseEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void changeEvent(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void buildPages();
    void buildRibbon();
    void buildHomeRibbon();
    void buildViewRibbon();
    void buildNavigation();
    void buildStatusBar();
    void buildTray();
    void buildShortcuts();
    void connectStore();

    void addPage(const QString& id, PageWidget* page);
    void showFileMenu(const QPoint& globalPosition);
    void toggleNavigation(bool enabled);
    void applyPanelSettings();
    void updateClock();
    void scheduleClockTick();
    void updateNextAlarm();
    void updateCounts();
    void updateStatusIcons();
    void updateMuteIcon();
    void rebuildTrayMenu();
    void showAndRaise();
    void saveWindowState();
    void restoreWindowState();
    void updateResponsiveLayout();
    void updateStatusText();
    void applyChromeMode();
    void setAlwaysOnTop(bool enabled);
    void ensureNativeResizeFrame();

    void newAlarm();
    void newTimer();
    void newGroup();
    void newTask();
    void editSelected();
    void duplicateSelected();
    void deleteSelected();
    void openSettings();
    void openCalendar();
    void showAbout();
    void showHelp();
    void toggleMute(bool muted);
    void handleAlarmFired(const QUuid& id);
    void showPreAlertPopup(const QUuid& id, int minutes);
    void showPostAlarmPopup(const Alarm& alarm);
    void handleTimerFinished(const QUuid& id);
    void showAlarmPopup(const QUuid& id);
    void repositionPopups();
    void stopSoundIfNoPopups(int fadeOutSeconds = 0);

    [[nodiscard]] PageWidget* activePage() const;

    Store m_store;
    Scheduler* m_scheduler{nullptr};
    TimerService* m_timerService{nullptr};

    TitleBar* m_titleBar{nullptr};
    RibbonBar* m_ribbon{nullptr};
    NavPane* m_nav{nullptr};
    QStackedWidget* m_pages{nullptr};
    QHash<QString, PageWidget*> m_pageMap;
    QVector<QString> m_pageOrder;
    PageWidget* m_currentPage{nullptr};

    QLabel* m_statusIcon{nullptr};
    QLabel* m_statusNext{nullptr};
    QLabel* m_statusCounts{nullptr};
    QToolButton* m_muteButton{nullptr};

    QSystemTrayIcon* m_tray{nullptr};
    QMenu* m_trayMenu{nullptr};
    QTimer* m_clockTimer{nullptr};
    QDateTime m_lastClockUpdate;
    QLineEdit* m_searchEdit{nullptr};
    QAction* m_searchAction{nullptr};
    QAction* m_alwaysOnTopAction{nullptr};
    RibbonButton* m_navToggle{nullptr};
    RibbonButton* m_statusToggle{nullptr};
    RibbonButton* m_secondsToggle{nullptr};
    QVector<RibbonButton*> m_densityButtons;

    bool m_quitting{false};
    bool m_muted{false};
    bool m_alwaysOnTop{false};
    quintptr m_nativeHandle{0};
    QString m_statusNextText;
    int m_lastStatusMinute{-1};
    QVector<QPointer<NotificationPopup>> m_popups;
    ThemeEffects* m_effects{nullptr};
};

} // namespace pac::ui
