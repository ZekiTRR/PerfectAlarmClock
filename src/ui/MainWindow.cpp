#include "MainWindow.h"

#include "AlarmsPage.h"
#include "CalendarPage.h"
#include "NavPane.h"
#include "NotificationPopup.h"
#include "SettingsDialog.h"
#include "PomodoroPage.h"
#include "StopwatchPage.h"
#include "TimersPage.h"
#include "ThemeEffects.h"
#include "TitleBar.h"
#include "TodoPage.h"
#include "WorldClockPage.h"
#include "Ribbon.h"
#include "UiKit.h"
#include "core/Catalog.h"
#include "core/Platform.h"
#include "core/Schedule.h"
#include "core/TimerPlan.h"
#include "core/Settings.h"
#include "core/SoundPlayer.h"
#include "core/Theme.h"

#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QButtonGroup>
#include <QCloseEvent>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QDesktopServices>
#include <QFileInfo>
#include <QMessageBox>
#include <QProcess>
#include <QUrl>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QScreen>
#include <QSettings>
#include <QRandomGenerator>
#include <QResizeEvent>
#include <QShortcut>
#include <QStackedWidget>
#include <QStatusBar>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWindow>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <windowsx.h>
#endif

#include <algorithm>

namespace pac::ui {
namespace {

[[nodiscard]] QString plural(int count, const QString& one, const QString& few, const QString& many)
{
    const int mod10 = count % 10;
    const int mod100 = count % 100;
    QString word;
    if (mod10 == 1 && mod100 != 11) {
        word = one;
    } else if (mod10 >= 2 && mod10 <= 4 && (mod100 < 12 || mod100 > 14)) {
        word = few;
    } else {
        word = many;
    }
    return QStringLiteral("%1 %2").arg(count).arg(word);
}

[[nodiscard]] QString countLabel(int count, const QString& one, const QString& few, const QString& many)
{
    return plural(count, one, few, many);
}

class ThemeSwatch final : public QAbstractButton {
public:
    explicit ThemeSwatch(const ThemeColors& colors, QWidget* parent = nullptr)
        : QAbstractButton(parent)
        , m_colors(colors)
    {
        setCheckable(true);
        setCursor(Qt::PointingHandCursor);
        setFixedSize(52, 30);
        setToolTip(colors.name);
        setChecked(ThemeManager::instance().current().id == colors.id);
        connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this] {
            setChecked(ThemeManager::instance().current().id == m_colors.id);
            update();
        });
        connect(this, &QAbstractButton::clicked, this, [this] {
            ThemeManager::instance().setTheme(m_colors.id);
        });
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event)
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const QRectF rect = QRectF(this->rect()).adjusted(1.0, 1.0, -1.0, -1.0);
        QPainterPath path;
        path.addRoundedRect(rect, 4.0, 4.0);
        painter.setClipPath(path);
        painter.fillRect(rect, m_colors.card);
        QLinearGradient ribbon(rect.topLeft(), QPointF(rect.left(), rect.top() + 11.0));
        ribbon.setColorAt(0.0, m_colors.ribbonTop);
        ribbon.setColorAt(1.0, m_colors.ribbonBottom);
        painter.fillRect(QRectF(rect.left(), rect.top(), rect.width(), 11.0), ribbon);
        painter.setPen(Qt::NoPen);
        painter.setBrush(m_colors.accent);
        painter.drawRoundedRect(QRectF(rect.left() + 4.0, rect.top() + 3.0, 14.0, 6.0), 1.5, 1.5);
        painter.setBrush(QColor(255, 255, 255, 180));
        painter.drawRoundedRect(QRectF(rect.left() + 20.0, rect.top() + 4.0, 13.0, 4.5), 1.0, 1.0);
        painter.setBrush(m_colors.panel);
        painter.drawRect(QRectF(rect.left() + 1.0, rect.top() + 20.0, 14.0, rect.height() - 21.0));
        painter.setBrush(m_colors.card);
        painter.drawRect(QRectF(rect.left() + 17.0, rect.top() + 20.0, rect.width() - 18.0,
                                rect.height() - 21.0));
        painter.setPen(QPen(m_colors.border, 1.0));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(path);
        if (isChecked()) {
            painter.setPen(QPen(m_colors.accent, 2.0));
            painter.drawPath(path);
            painter.setPen(Qt::NoPen);
            painter.setBrush(m_colors.accent);
            painter.drawEllipse(QPointF(rect.right() - 5.0, rect.top() + 5.0), 4.5, 4.5);
            painter.setPen(QPen(Qt::white, 1.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter.drawLine(QPointF(rect.right() - 7.0, rect.top() + 5.0),
                             QPointF(rect.right() - 5.4, rect.top() + 6.6));
            painter.drawLine(QPointF(rect.right() - 5.4, rect.top() + 6.6),
                             QPointF(rect.right() - 2.6, rect.top() + 3.4));
        }
    }

private:
    ThemeColors m_colors;
};

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("Perfect Alarm Clock"));
    setWindowIcon(appIcon());
    setWindowFlag(Qt::FramelessWindowHint, true);
    setMinimumSize(640, 500);

    m_store.load();
    restoreWindowState();
    m_alwaysOnTop = Settings::instance().raw()
                        .value(QStringLiteral("ui/alwaysOnTop"), false)
                        .toBool();
    if (m_alwaysOnTop) {
        setWindowFlag(Qt::WindowStaysOnTopHint, true);
    }

    auto* central = new QWidget(this);
    central->setObjectName(QStringLiteral("Central"));
    auto* rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    m_titleBar = new TitleBar(central);
    rootLayout->addWidget(m_titleBar);

    m_ribbon = new RibbonBar(central);
    rootLayout->addWidget(m_ribbon);

    connect(m_titleBar, &TitleBar::minimizeRequested, this, &QWidget::showMinimized);
    connect(m_titleBar, &TitleBar::maximizeRestoreRequested, this, [this] {
        if (isMaximized()) {
            showNormal();
        } else {
            showMaximized();
        }
    });
    connect(m_titleBar, &TitleBar::closeRequested, this, &QWidget::close);

    auto* content = new QWidget(central);
    auto* contentLayout = new QHBoxLayout(content);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);
    m_nav = new NavPane(content);
    contentLayout->addWidget(m_nav);
    m_pages = new QStackedWidget(content);
    m_pages->setObjectName(QStringLiteral("PageHost"));
    m_pages->setAttribute(Qt::WA_StyledBackground, true);
    contentLayout->addWidget(m_pages, 1);
    rootLayout->addWidget(content, 1);
    setCentralWidget(central);

    buildPages();
    buildRibbon();
    buildNavigation();
    applyChromeMode();
    buildStatusBar();
    buildTray();
    buildShortcuts();
    connectStore();

    m_ribbon->addPage(QStringLiteral("home"), tr("Главная"), icons::Icon::Home);
    m_ribbon->addPage(QStringLiteral("view"), tr("Вид"), icons::Icon::View);
    buildHomeRibbon();
    buildViewRibbon();
    m_ribbon->updateContentWidths();

    m_scheduler = new Scheduler(m_store, this);
    m_timerService = new TimerService(m_store, this);
    connect(m_scheduler, &Scheduler::alarmFired, this, &MainWindow::handleAlarmFired);
    connect(m_scheduler, &Scheduler::preAlert, this, &MainWindow::showPreAlertPopup);
    connect(m_timerService, &TimerService::timerFinished, this, &MainWindow::handleTimerFinished);
    connect(m_timerService, &TimerService::timerStageChanged, this,
            [this](const QUuid& id) {
                const CountdownTimer* timer = m_store.findTimer(id);
                if (timer == nullptr) {
                    return;
                }
                const TimerStage* stage = timerplan::currentStage(*timer);
                if (stage == nullptr) {
                    return;
                }
                if (!m_muted && timer->soundEnabled) {
                    QString soundId = stage->soundId.isEmpty() ? timer->soundId : stage->soundId;
                    if (SoundPlayer::resolveSoundPath(soundId).isEmpty()) {
                        soundId = QStringLiteral("builtin:chime");
                    }
                    SoundPlayer::instance().play(soundId, timer->volume, false, 0, false);
                }
                if (!stage->message.trimmed().isEmpty()) {
                    auto* popup = new NotificationPopup();
                    popup->setContent(tr("ЭТАП ТАЙМЕРА"),
                                      timer->label.isEmpty() ? tr("Таймер") : timer->label,
                                      stage->message, QStringLiteral("⏳"),
                                      QColor(timer->color), stage->name,
                                      QString(), icons::Icon::Info, tr("Закрыть"));
                    popup->setActivateOnShow(false);
                    QPointer<NotificationPopup> guarded(popup);
                    connect(popup, &NotificationPopup::dismissed, popup, &QWidget::close);
                    m_popups.append(guarded);
                    connect(popup, &QObject::destroyed, this, [this] {
                        m_popups.erase(std::remove_if(m_popups.begin(), m_popups.end(),
                                                      [](const QPointer<NotificationPopup>& item) {
                                                          return item.isNull();
                                                      }),
                                       m_popups.end());
                        repositionPopups();
                    });
                    repositionPopups();
                    popup->show();
                    popup->raise();
                    QTimer::singleShot(12'000, popup, &QWidget::close);
                }
            });

    connect(&Settings::instance(), &Settings::changed, this, [this] {
        applyPanelSettings();
        updateClock();
        updateNextAlarm();
        updateCounts();
        updateStatusIcons();
        for (auto* button : m_densityButtons) {
            if (button != nullptr) {
                button->setChecked(button->property("densityValue").toInt()
                                   == Settings::instance().rowDensity());
            }
        }
        if (auto* page = m_pageMap.value(QStringLiteral("alarms"))) {
            QMetaObject::invokeMethod(page, "applyDensity");
            QMetaObject::invokeMethod(page, "reload");
        }
        if (auto* page = m_pageMap.value(QStringLiteral("tasks"))) {
            QMetaObject::invokeMethod(page, "refreshAll");
        }
        const bool muted = Settings::instance().raw()
                               .value(QStringLiteral("notifications/muted"), false)
                               .toBool();
        if (muted != m_muted) {
            toggleMute(muted);
        }
    });
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this] {
        applyChromeMode();
        updateStatusIcons();
        rebuildTrayMenu();
        if (m_effects != nullptr) {
            m_effects->restartEffect();
            if (m_currentPage != nullptr) {
                m_effects->stackUnder(m_currentPage);
            }
        }
        if (m_searchAction != nullptr) {
            m_searchAction->setIcon(icons::qicon(icons::Icon::Search,
                                                 ThemeManager::instance().current().accent));
        }
    });

    m_muted = Settings::instance().raw().value(QStringLiteral("notifications/muted"), false).toBool();
    applyPanelSettings();
    updateMuteIcon();

    const QString lastPage = Settings::instance().raw()
                                 .value(QStringLiteral("ui/lastPage"), QStringLiteral("alarms"))
                                 .toString();
    showPage(m_pageMap.contains(lastPage) ? lastPage : QStringLiteral("alarms"));
    m_effects = new ThemeEffects(m_pages);
    m_effects->setGeometry(m_pages->rect());
    if (m_currentPage != nullptr) {
        m_effects->stackUnder(m_currentPage);
    }

    m_clockTimer = new QTimer(this);
    m_clockTimer->setSingleShot(true);
    m_clockTimer->setTimerType(Qt::PreciseTimer);
    connect(m_clockTimer, &QTimer::timeout, this, [this] {
        updateClock();
        scheduleClockTick();
    });

    updateClock();
    scheduleClockTick();
    updateNextAlarm();
    updateCounts();

    QApplication::instance()->installEventFilter(this);
    QApplication::instance()->installNativeEventFilter(this);
    m_titleBar->setMaximized(isMaximized());
    updateResponsiveLayout();

    if (Settings::instance().startMinimized() && QSystemTrayIcon::isSystemTrayAvailable()) {
        QTimer::singleShot(0, this, [this] { hide(); });
    }
}

MainWindow::~MainWindow()
{
    saveWindowState();
    if (QApplication::instance() != nullptr) {
        QApplication::instance()->removeNativeEventFilter(this);
    }
    SoundPlayer::instance().stop();
}

void MainWindow::ensureNativeResizeFrame()
{
#ifdef Q_OS_WIN
    const HWND hwnd = reinterpret_cast<HWND>(winId());
    if (hwnd == nullptr) {
        return;
    }
    m_nativeHandle = reinterpret_cast<quintptr>(hwnd);
    const bool wantFrame = !isMaximized() && !isFullScreen();
    const LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
    const LONG_PTR updated = wantFrame ? (style | WS_THICKFRAME) : (style & ~WS_THICKFRAME);
    if (updated == style) {
        return;
    }
    SetWindowLongPtr(hwnd, GWL_STYLE, updated);
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
#endif
}

bool MainWindow::nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result)
{
    Q_UNUSED(eventType)
#ifdef Q_OS_WIN
    auto* msg = static_cast<MSG*>(message);
    if (msg == nullptr || m_nativeHandle == 0
        || msg->hwnd != reinterpret_cast<HWND>(m_nativeHandle)) {
        return false;
    }
    if (msg->message == WM_NCCALCSIZE && msg->wParam == TRUE) {
        if (!isMaximized() && !isFullScreen()) {
            *result = 0;
            return true;
        }
    }
    if (msg->message == WM_NCHITTEST && !isMaximized() && !isFullScreen()) {
        RECT windowRect{};
        if (GetWindowRect(msg->hwnd, &windowRect) != FALSE) {
            const int x = GET_X_LPARAM(msg->lParam);
            const int y = GET_Y_LPARAM(msg->lParam);
            const int border = std::max(6, qRound(8.0 * devicePixelRatioF()));
            const bool left = x >= windowRect.left && x < windowRect.left + border;
            const bool right = x < windowRect.right && x >= windowRect.right - border;
            const bool top = y >= windowRect.top && y < windowRect.top + border;
            const bool bottom = y < windowRect.bottom && y >= windowRect.bottom - border;
            if (top && left) {
                *result = HTTOPLEFT;
                return true;
            }
            if (top && right) {
                *result = HTTOPRIGHT;
                return true;
            }
            if (bottom && left) {
                *result = HTBOTTOMLEFT;
                return true;
            }
            if (bottom && right) {
                *result = HTBOTTOMRIGHT;
                return true;
            }
            if (left) {
                *result = HTLEFT;
                return true;
            }
            if (right) {
                *result = HTRIGHT;
                return true;
            }
            if (top) {
                *result = HTTOP;
                return true;
            }
            if (bottom) {
                *result = HTBOTTOM;
                return true;
            }
        }
    }
#endif
    return false;
}

PageWidget* MainWindow::activePage() const
{
    return m_currentPage;
}

void MainWindow::addPage(const QString& id, PageWidget* page)
{
    m_pages->addWidget(page);
    m_pageMap.insert(id, page);
    m_pageOrder.append(id);
}

void MainWindow::buildPages()
{
    auto* alarmsPage = new AlarmsPage(m_store, m_pages);
    addPage(QStringLiteral("alarms"), alarmsPage);
    auto* timersPage = new TimersPage(m_store, m_pages);
    addPage(QStringLiteral("timers"), timersPage);
    auto* stopwatchPage = new StopwatchPage(m_pages);
    addPage(QStringLiteral("stopwatch"), stopwatchPage);
    auto* pomodoroPage = new PomodoroPage(m_store, m_pages);
    addPage(QStringLiteral("pomodoro"), pomodoroPage);
    auto* calendarPage = new CalendarPage(m_store, m_pages);
    addPage(QStringLiteral("calendar"), calendarPage);
    auto* clocksPage = new WorldClockPage(m_store, m_pages);
    addPage(QStringLiteral("clocks"), clocksPage);
    auto* todoPage = new TodoPage(m_store, m_pages);
    addPage(QStringLiteral("tasks"), todoPage);

    connect(calendarPage, &CalendarPage::addAlarmRequested, alarmsPage,
            &AlarmsPage::createAlarmOnDate);
    connect(calendarPage, &CalendarPage::addTaskRequested, todoPage,
            &TodoPage::createTaskOnDate);
    connect(calendarPage, &CalendarPage::editAlarmRequested, alarmsPage,
            &AlarmsPage::editAlarmById);
    connect(calendarPage, &CalendarPage::editTaskRequested, todoPage,
            &TodoPage::editTodoById);
}

void MainWindow::buildNavigation()
{
    m_nav->addItem(QStringLiteral("alarms"), tr("Будильники"), icons::Icon::Alarm,
                   QColor(QStringLiteral("#e2842b")));
    m_nav->addItem(QStringLiteral("timers"), tr("Таймеры"), icons::Icon::Timer,
                   QColor(QStringLiteral("#d69a2a")));
    m_nav->addItem(QStringLiteral("stopwatch"), tr("Секундомер"), icons::Icon::Stopwatch,
                   QColor(QStringLiteral("#3f7fd6")));
    m_nav->addItem(QStringLiteral("pomodoro"), tr("Помодоро"), icons::Icon::Pomodoro,
                   QColor(QStringLiteral("#d9534f")));
    m_nav->addItem(QStringLiteral("calendar"), tr("Календарь"), icons::Icon::Calendar,
                   QColor(QStringLiteral("#6a63b5")));
    m_nav->addItem(QStringLiteral("clocks"), tr("Мировое время"), icons::Icon::Globe,
                   QColor(QStringLiteral("#2e8b8b")));
    m_nav->addItem(QStringLiteral("tasks"), tr("Задачи"), icons::Icon::Tasks,
                   QColor(QStringLiteral("#4c9a62")));
    m_nav->addItem(QStringLiteral("settings"), tr("Настройки"), icons::Icon::Settings,
                   ThemeManager::instance().current().accent);

    connect(m_nav, &NavPane::currentChanged, this, [this](const QString& id) {
        if (id == QStringLiteral("settings")) {
            const QString restore = m_currentPage != nullptr
                                        ? m_pageOrder.at(m_pages->indexOf(m_currentPage))
                                        : QStringLiteral("alarms");
            openSettings();
            m_nav->setCurrent(restore);
            return;
        }
        showPage(id);
    });
    connect(m_nav, &NavPane::nextAlarmClicked, this, [this] { showPage(QStringLiteral("alarms")); });
}

void MainWindow::buildRibbon()
{
    connect(m_ribbon, &RibbonBar::newAlarmRequested, this, &MainWindow::newAlarm);
    connect(m_ribbon, &RibbonBar::settingsRequested, this, &MainWindow::openSettings);
    connect(m_ribbon, &RibbonBar::helpRequested, this, &MainWindow::showHelp);
    connect(m_ribbon, &RibbonBar::fileMenuRequested, this, &MainWindow::showFileMenu);
}

void MainWindow::buildHomeRibbon()
{
    const QColor blue(QStringLiteral("#2a66a5"));
    const QColor teal(QStringLiteral("#2e8b8b"));
    const QColor green(QStringLiteral("#4c9a62"));
    const QColor orange(QStringLiteral("#bd7a2f"));
    const QColor red(QStringLiteral("#b9504a"));
    const QColor slate(QStringLiteral("#5b7186"));
    const QColor purple(QStringLiteral("#6a63b5"));

    auto* create = m_ribbon->addGroup(QStringLiteral("home"), tr("Создать"));
    auto* newAlarmButton = create->addLarge(icons::Icon::AlarmPlus, tr("Будильник"),
                                            tr("Новый будильник (Ctrl+N)"), blue);
    connect(newAlarmButton, &QAbstractButton::clicked, this, &MainWindow::newAlarm);
    auto* newGroupButton = create->addLarge(icons::Icon::FolderPlus, tr("Группа"),
                                            tr("Новая группа будильников"), teal);
    connect(newGroupButton, &QAbstractButton::clicked, this, &MainWindow::newGroup);
    auto* newTaskButton = create->addLarge(icons::Icon::TaskPlus, tr("Задача"),
                                           tr("Новая задача"), green);
    connect(newTaskButton, &QAbstractButton::clicked, this, &MainWindow::newTask);
    auto* newTimerButton = create->addSmall(icons::Icon::Timer, tr("Таймер"),
                                            tr("Новый таймер"), QColor(QStringLiteral("#d69a2a")));
    connect(newTimerButton, &QAbstractButton::clicked, this, &MainWindow::newTimer);

    auto* edit = m_ribbon->addGroup(QStringLiteral("home"), tr("Правка"));
    auto* editButton = edit->addLarge(icons::Icon::Edit, tr("Изменить"),
                                      tr("Изменить выбранный элемент"), orange);
    connect(editButton, &QAbstractButton::clicked, this, &MainWindow::editSelected);
    auto* copyButton = edit->addSmall(icons::Icon::Copy, tr("Копировать"),
                                      tr("Создать копию"), slate);
    connect(copyButton, &QAbstractButton::clicked, this, &MainWindow::duplicateSelected);
    auto* deleteButton = edit->addSmall(icons::Icon::Delete, tr("Удалить"),
                                        tr("Удалить выбранное (Del)"), red);
    connect(deleteButton, &QAbstractButton::clicked, this, &MainWindow::deleteSelected);
    auto* toggleButton = edit->addSmall(icons::Icon::Check, tr("Включить"),
                                        tr("Включить или выключить выбранное"), green);
    connect(toggleButton, &QAbstractButton::clicked, this, [this] {
        QMetaObject::invokeMethod(m_currentPage, "toggleSelected");
    });

    auto* tools = m_ribbon->addGroup(QStringLiteral("home"), tr("Инструменты"));
    auto* settingsButton = tools->addLarge(icons::Icon::Settings, tr("Настройки"),
                                           tr("Настройки приложения"), slate);
    connect(settingsButton, &QAbstractButton::clicked, this, &MainWindow::openSettings);
    auto* calendarButton = tools->addSmall(icons::Icon::Calendar, tr("Календарь"),
                                           tr("Открыть календарь"), purple);
    connect(calendarButton, &QAbstractButton::clicked, this, &MainWindow::openCalendar);
    auto* infoButton = tools->addSmall(icons::Icon::Info, tr("О программе"),
                                       tr("Информация о приложении"), blue);
    connect(infoButton, &QAbstractButton::clicked, this, &MainWindow::showAbout);
    auto* collapseButton = tools->addSmall(icons::Icon::ChevronUp, tr("Свернуть всё"),
                                           tr("Свернуть все группы"), QColor(QStringLiteral("#7a8796")));
    connect(collapseButton, &QAbstractButton::clicked, this, [this] {
        QMetaObject::invokeMethod(m_currentPage, "collapseAll");
    });

    m_ribbon->addSpacer(QStringLiteral("home"));
    auto* search = m_ribbon->addGroup(QStringLiteral("home"), tr("Поиск"));
    auto* searchWidget = new QWidget(m_ribbon);
    auto* searchLayout = new QVBoxLayout(searchWidget);
    searchLayout->setContentsMargins(2, 7, 2, 0);
    searchLayout->setSpacing(4);
    m_searchEdit = new QLineEdit(searchWidget);
    m_searchEdit->setFixedWidth(228);
    m_searchEdit->setPlaceholderText(tr("Поиск будильников…"));
    m_searchEdit->setClearButtonEnabled(true);
    m_searchAction = m_searchEdit->addAction(
        icons::qicon(icons::Icon::Search, ThemeManager::instance().current().accent),
        QLineEdit::LeadingPosition);
    searchLayout->addWidget(m_searchEdit);
    auto* searchHint = makeLabel(tr("Ctrl+F — быстрый поиск"), QStringLiteral("Faint"), searchWidget);
    searchHint->setContentsMargins(22, 0, 0, 0);
    searchLayout->addWidget(searchHint);
    search->addWidget(searchWidget);
    connect(m_searchEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        QMetaObject::invokeMethod(m_pageMap.value(QStringLiteral("alarms")), "setSearch",
                                  Q_ARG(QString, text));
    });
    if (auto* alarmsPage = qobject_cast<AlarmsPage*>(m_pageMap.value(QStringLiteral("alarms")))) {
        connect(alarmsPage, &AlarmsPage::searchResetRequested, this, [this] {
            if (m_searchEdit != nullptr) {
                m_searchEdit->clear();
            }
        });
    }
}

void MainWindow::buildViewRibbon()
{
    auto* themes = m_ribbon->addGroup(QStringLiteral("view"), tr("Темы Office"));
    auto* gallery = new QWidget(m_ribbon);
    auto* grid = new QGridLayout(gallery);
    grid->setContentsMargins(2, 2, 4, 0);
    grid->setHorizontalSpacing(3);
    grid->setVerticalSpacing(3);
    int index = 0;
    for (const auto& colors : ThemeManager::instance().themes()) {
        auto* swatch = new ThemeSwatch(colors, gallery);
        grid->addWidget(swatch, index / 6, index % 6);
        ++index;
    }
    themes->addWidget(gallery);

    auto* panels = m_ribbon->addGroup(QStringLiteral("view"), tr("Панели"));
    m_navToggle = panels->addSmall(icons::Icon::Navigation, tr("Навигация"),
                                   tr("Показать или скрыть панель навигации"),
                                   ThemeManager::instance().current().accent);
    m_navToggle->setCheckable(true);
    m_navToggle->setChecked(Settings::instance().showNavPane());
    connect(m_navToggle, &QAbstractButton::clicked, this, [this](bool checked) {
        Settings::instance().raw().setValue(QStringLiteral("appearance/showNavPane"), checked);
        Settings::instance().notifyChanged();
    });
    m_statusToggle = panels->addSmall(icons::Icon::StatusBar, tr("Строка состояния"),
                                      tr("Показать или скрыть строку состояния"),
                                      ThemeManager::instance().current().accent);
    m_statusToggle->setCheckable(true);
    m_statusToggle->setChecked(Settings::instance().showStatusBar());
    connect(m_statusToggle, &QAbstractButton::clicked, this, [this](bool checked) {
        Settings::instance().raw().setValue(QStringLiteral("appearance/showStatusBar"), checked);
        Settings::instance().notifyChanged();
    });
    m_secondsToggle = panels->addSmall(icons::Icon::Clock, tr("Секунды"),
                                       tr("Показывать секунды в заголовке окна"),
                                       ThemeManager::instance().current().accent);
    m_secondsToggle->setCheckable(true);
    m_secondsToggle->setChecked(Settings::instance().showSecondsInTitle());
    connect(m_secondsToggle, &QAbstractButton::clicked, this, [this](bool checked) {
        Settings::instance().raw().setValue(QStringLiteral("general/showSecondsInTitle"), checked);
        Settings::instance().notifyChanged();
    });

    auto* density = m_ribbon->addGroup(QStringLiteral("view"), tr("Плотность"));
    auto* densityGroup = new QButtonGroup(density);
    densityGroup->setExclusive(true);
    const auto addDensity = [&](icons::Icon icon, const QString& text, const QString& tip, int value) {
        auto* button = density->addSmall(icon, text, tip, ThemeManager::instance().current().accent);
        button->setCheckable(true);
        button->setChecked(Settings::instance().rowDensity() == value);
        button->setProperty("densityValue", value);
        m_densityButtons.append(button);
        densityGroup->addButton(button, value);
        connect(button, &QAbstractButton::clicked, this, [value] {
            Settings::instance().raw().setValue(QStringLiteral("appearance/rowDensity"), value);
            Settings::instance().notifyChanged();
        });
    };
    addDensity(icons::Icon::Density, tr("Компактно"), tr("Плотные строки"), 0);
    addDensity(icons::Icon::Density, tr("Обычно"), tr("Стандартные строки"), 1);
    addDensity(icons::Icon::Density, tr("Просторно"), tr("Просторные строки"), 2);

    auto* windowGroup = m_ribbon->addGroup(QStringLiteral("view"), tr("Окно"));
    auto* fullscreen = windowGroup->addLarge(icons::Icon::Fullscreen, tr("Весь экран"),
                                            tr("Полноэкранный режим"),
                                            QColor(QStringLiteral("#2a66a5")));
    fullscreen->setCheckable(true);
    connect(fullscreen, &QAbstractButton::clicked, this, [this, fullscreen](bool checked) {
        if (checked) {
            showFullScreen();
        } else {
            showNormal();
        }
    });
    auto* reset = windowGroup->addSmall(icons::Icon::Reset, tr("Сбросить вид"),
                                        tr("Вернуть исходный размер окна"), QColor(QStringLiteral("#6c7a89")));
    connect(reset, &QAbstractButton::clicked, this, [this] {
        showNormal();
        resize(1280, 840);
        if (const QScreen* screen = this->screen()) {
            move(screen->availableGeometry().center() - rect().center());
        }
    });
    auto* collapseRibbon = windowGroup->addSmall(icons::Icon::ChevronUp, tr("Свернуть ленту"),
                                                tr("Оставить только вкладки"), QColor(QStringLiteral("#6c7a89")));
    connect(collapseRibbon, &QAbstractButton::clicked, this,
            [this] { m_ribbon->setRibbonCollapsed(true); });
}

void MainWindow::buildStatusBar()
{
    m_statusIcon = new QLabel(this);
    m_statusIcon->setContentsMargins(4, 0, 5, 0);
    m_statusNext = new QLabel(tr("Нет запланированных будильников"), this);
    m_statusNext->setObjectName(QStringLiteral("StatusText"));
    m_statusNext->setMinimumWidth(0);
    m_statusNext->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

    statusBar()->addWidget(m_statusIcon);
    statusBar()->addWidget(m_statusNext, 1);

    m_statusCounts = new QLabel(this);
    m_statusCounts->setObjectName(QStringLiteral("StatusText"));
    statusBar()->addPermanentWidget(m_statusCounts);
    statusBar()->addPermanentWidget(new QLabel(QStringLiteral("   "), this));

    m_muteButton = new QToolButton(this);
    m_muteButton->setCheckable(true);
    m_muteButton->setAutoRaise(true);
    m_muteButton->setFixedSize(22, 20);
    m_muteButton->setIconSize(QSize(16, 16));
    m_muteButton->setCursor(Qt::PointingHandCursor);
    m_muteButton->setChecked(m_muted);
    connect(m_muteButton, &QToolButton::clicked, this, &MainWindow::toggleMute);
    statusBar()->addPermanentWidget(m_muteButton);
    statusBar()->setSizeGripEnabled(false);
}

void MainWindow::buildTray()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        return;
    }
    m_tray = new QSystemTrayIcon(appIcon(), this);
    m_tray->setToolTip(QStringLiteral("Perfect Alarm Clock"));
    rebuildTrayMenu();
    connect(m_tray, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
            showAndRaise();
        }
    });
    m_tray->show();
}

void MainWindow::rebuildTrayMenu()
{
    if (m_tray == nullptr) {
        return;
    }
    m_alwaysOnTopAction = nullptr;
    if (m_trayMenu != nullptr) {
        m_tray->setContextMenu(nullptr);
        delete m_trayMenu;
    }
    const QColor accent = ThemeManager::instance().current().accent;
    m_trayMenu = new QMenu(this);
    auto* openAction = m_trayMenu->addAction(icons::qicon(icons::Icon::Home, accent),
                                             tr("Открыть Perfect Alarm Clock"));
    connect(openAction, &QAction::triggered, this, &MainWindow::showAndRaise);
    auto* alarmAction = m_trayMenu->addAction(icons::qicon(icons::Icon::AlarmPlus, accent),
                                              tr("Новый будильник"));
    connect(alarmAction, &QAction::triggered, this, [this] {
        showAndRaise();
        newAlarm();
    });
    auto* calendarAction = m_trayMenu->addAction(icons::qicon(icons::Icon::Calendar, accent),
                                                 tr("Календарь"));
    connect(calendarAction, &QAction::triggered, this, [this] {
        showAndRaise();
        showPage(QStringLiteral("calendar"));
    });
    m_trayMenu->addSeparator();
    auto* stopSignal = m_trayMenu->addAction(icons::qicon(icons::Icon::Stop, accent),
                                             tr("Остановить активный сигнал"));
    connect(stopSignal, &QAction::triggered, this, [this] {
        for (const auto& popup : m_popups) {
            if (!popup.isNull() && popup->isVisible() && popup->isUrgent()) {
                popup->dismissNow();
            }
        }
    });
    auto* snoozeSignal = m_trayMenu->addAction(icons::qicon(icons::Icon::Snooze, accent),
                                               tr("Отложить активный сигнал"));
    connect(snoozeSignal, &QAction::triggered, this, [this] {
        for (const auto& popup : m_popups) {
            if (!popup.isNull() && popup->isVisible() && popup->isUrgent()) {
                popup->triggerPrimary();
                break;
            }
        }
    });
    auto* pauseMenu = m_trayMenu->addMenu(icons::qicon(icons::Icon::Pause, accent),
                                          tr("Пауза будильников"));
    const QDateTime now = QDateTime::currentDateTime();
    const auto addPause = [this, pauseMenu](const QString& text, const QDateTime& target) {
        auto* action = pauseMenu->addAction(text);
        connect(action, &QAction::triggered, this, [target] {
            Settings::instance().setAlarmsSuspendedUntil(target);
        });
    };
    addPause(tr("На 30 минут"), now.addSecs(30 * 60));
    addPause(tr("На 1 час"), now.addSecs(60 * 60));
    addPause(tr("На 2 часа"), now.addSecs(2 * 60 * 60));
    auto* resume = pauseMenu->addAction(tr("Снять паузу"));
    connect(resume, &QAction::triggered, this, [] {
        Settings::instance().setAlarmsSuspendedUntil(QDateTime());
    });
    m_trayMenu->addSeparator();
    auto* settingsAction = m_trayMenu->addAction(icons::qicon(icons::Icon::Settings, accent),
                                                 tr("Настройки"));
    connect(settingsAction, &QAction::triggered, this, [this] {
        showAndRaise();
        openSettings();
    });
    m_trayMenu->addSeparator();
    m_alwaysOnTopAction = m_trayMenu->addAction(icons::qicon(icons::Icon::Pin, accent),
                                                tr("Поверх всех окон"));
    m_alwaysOnTopAction->setCheckable(true);
    m_alwaysOnTopAction->setChecked(m_alwaysOnTop);
    m_alwaysOnTopAction->setToolTip(tr("Не позволять другим окнам перекрывать приложение"));
    connect(m_alwaysOnTopAction, &QAction::toggled, this, &MainWindow::setAlwaysOnTop);
    m_trayMenu->addSeparator();
    auto* quitAction = m_trayMenu->addAction(icons::qicon(icons::Icon::Close, accent), tr("Выход"));
    connect(quitAction, &QAction::triggered, this, [this] {
        m_quitting = true;
        saveWindowState();
        qApp->quit();
    });
    m_tray->setContextMenu(m_trayMenu);
}

void MainWindow::buildShortcuts()
{
    auto* newAlarmShortcut = new QShortcut(QKeySequence::New, this);
    connect(newAlarmShortcut, &QShortcut::activated, this, &MainWindow::newAlarm);
    auto* findShortcut = new QShortcut(QKeySequence::Find, this);
    connect(findShortcut, &QShortcut::activated, this, [this] {
        showPage(QStringLiteral("alarms"));
        if (ThemeManager::instance().current().flavor == ThemeFlavor::WindowsClock) {
            QMetaObject::invokeMethod(m_pageMap.value(QStringLiteral("alarms")), "focusSearch");
            return;
        }
        if (m_searchEdit != nullptr) {
            m_searchEdit->setFocus(Qt::ShortcutFocusReason);
            m_searchEdit->selectAll();
        }
    });
    auto* settingsShortcut = new QShortcut(QKeySequence(QStringLiteral("Ctrl+,")), this);
    connect(settingsShortcut, &QShortcut::activated, this, &MainWindow::openSettings);
    auto* helpShortcut = new QShortcut(QKeySequence::HelpContents, this);
    connect(helpShortcut, &QShortcut::activated, this, &MainWindow::showHelp);
    auto* dismissShortcut = new QShortcut(QKeySequence(QStringLiteral("Ctrl+Alt+D")), this);
    dismissShortcut->setContext(Qt::ApplicationShortcut);
    connect(dismissShortcut, &QShortcut::activated, this, [this] {
        for (const auto& popup : m_popups) {
            if (!popup.isNull() && popup->isVisible() && popup->isUrgent()) {
                popup->dismissNow();
            }
        }
    });
    auto* primaryShortcut = new QShortcut(QKeySequence(Qt::Key_Space), m_pages);
    primaryShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(primaryShortcut, &QShortcut::activated, this, [this] {
        if (m_currentPage != nullptr) {
            QMetaObject::invokeMethod(m_currentPage, "primaryAction");
        }
    });
}

void MainWindow::connectStore()
{
    connect(&m_store, &Store::changed, this, [this] {
        updateNextAlarm();
        updateCounts();
    });
    connect(&m_store, &Store::alarmsChanged, this, [this] {
        updateNextAlarm();
        updateCounts();
    });
}

void MainWindow::showPage(const QString& id)
{
    auto it = m_pageMap.find(id);
    if (it == m_pageMap.end() || it.value() == m_currentPage) {
        return;
    }
    if (m_currentPage != nullptr) {
        m_currentPage->deactivate();
    }
    m_currentPage = it.value();
    m_pages->setCurrentWidget(m_currentPage);
    m_currentPage->activate();
    m_nav->setCurrent(id);
    m_currentPage->refreshClock(QDateTime::currentDateTime());
    Settings::instance().raw().setValue(QStringLiteral("ui/lastPage"), id);
    if (m_effects != nullptr) {
        m_effects->setGeometry(m_pages->rect());
        if (m_currentPage != nullptr) {
            m_effects->stackUnder(m_currentPage);
        }
    }
}

void MainWindow::scheduleClockTick()
{
    if (m_clockTimer == nullptr || !isVisible() || isMinimized()) {
        return;
    }
    const int delay = std::max(1, 1000 - QTime::currentTime().msec());
    m_clockTimer->start(delay);
}

void MainWindow::updateClock()
{
    const QDateTime now = QDateTime::currentDateTime();
    if (m_lastClockUpdate.isValid()) {
        const qint64 jump = m_lastClockUpdate.secsTo(now);
        if (jump > 5 || jump < -5) {
            if (m_scheduler != nullptr) {
                m_scheduler->refresh();
            }
            if (m_timerService != nullptr) {
                m_timerService->refresh();
            }
        }
    }
    m_lastClockUpdate = now;
    const QDateTime suspendedUntil = Settings::instance().alarmsSuspendedUntil();
    if (suspendedUntil.isValid() && suspendedUntil <= now) {
        Settings::instance().setAlarmsSuspendedUntil(QDateTime());
    }
    const QString timeText = Settings::instance().use24h()
                                 ? now.time().toString(Settings::instance().showSecondsInTitle()
                                                           ? QStringLiteral("HH:mm:ss")
                                                           : QStringLiteral("HH:mm"))
                                 : now.time().toString(Settings::instance().showSecondsInTitle()
                                                           ? QStringLiteral("h:mm:ss AP")
                                                           : QStringLiteral("h:mm AP"));
    const QString title = QStringLiteral("%1 %2 %3")
                              .arg(now.date().toString(QStringLiteral("dd.MM.yyyy")), timeText,
                                   catalog::dayShort(static_cast<Qt::DayOfWeek>(now.date().dayOfWeek())));
    if (m_titleBar != nullptr) {
        m_titleBar->setClockText(title);
    }
    if (m_currentPage != nullptr) {
        m_currentPage->refreshClock(now);
    }
    if (m_lastStatusMinute != now.time().minute()) {
        m_lastStatusMinute = now.time().minute();
        updateNextAlarm();
    }
}

void MainWindow::updateNextAlarm()
{
    const QDateTime now = QDateTime::currentDateTime();
    const QDateTime suspendedUntil = Settings::instance().alarmsSuspendedUntil();
    const bool use24h = Settings::instance().use24h();
    if (suspendedUntil > now) {
        const QString until = schedule::dateTimeText(suspendedUntil, now, use24h);
        m_nav->setNextAlarm(tr("Пауза"), tr("до %1").arg(until), QStringLiteral("⏸"), false);
        m_statusNextText = tr("Будильники на паузе до %1").arg(until);
        m_statusNext->setText(m_statusNextText);
        m_statusNext->setToolTip(m_statusNextText);
        if (m_tray != nullptr) {
            m_tray->setToolTip(tr("Perfect Alarm Clock\nБудильники на паузе до %1").arg(until));
        }
        return;
    }
    const Alarm* best = nullptr;
    QDateTime bestAt;
    for (const auto& alarm : m_store.data().alarms) {
        if (!alarm.enabled) {
            continue;
        }
        const QDateTime next = schedule::nextOccurrence(alarm, now);
        if (!next.isValid()) {
            continue;
        }
        if (!bestAt.isValid() || next < bestAt) {
            bestAt = next;
            best = &alarm;
        }
    }

    if (best != nullptr) {
        const QString line = schedule::dateTimeText(bestAt, now, use24h);
        const QString label = best->label.isEmpty() ? tr("Без названия") : best->label;
        m_nav->setNextAlarm(line, label, best->icon, true);
        m_statusNextText = tr("Следующий будильник: %1 — %2").arg(line, label);
        m_statusNext->setText(m_statusNextText);
        m_statusNext->setToolTip(m_statusNextText);
        if (m_tray != nullptr) {
            m_tray->setToolTip(QStringLiteral("Perfect Alarm Clock\n%1 · %2").arg(line, label));
        }
    } else {
        const bool hasAlarms = !m_store.data().alarms.empty();
        const QString line = hasAlarms ? tr("Все выключены") : tr("Нет будильников");
        const QString detail = hasAlarms ? tr("Включите будильник")
                                         : tr("Нажмите, чтобы создать");
        m_nav->setNextAlarm(line, detail, QString(), false);
        m_statusNextText = hasAlarms ? tr("Все будильники выключены")
                                     : tr("Нет запланированных будильников");
        m_statusNext->setText(m_statusNextText);
        m_statusNext->setToolTip(m_statusNextText);
        if (m_tray != nullptr) {
            m_tray->setToolTip(QStringLiteral("Perfect Alarm Clock"));
        }
    }
}

void MainWindow::updateCounts()
{
    int alarms = 0;
    int enabledAlarms = 0;
    for (const auto& alarm : m_store.data().alarms) {
        ++alarms;
        if (alarm.enabled) {
            ++enabledAlarms;
        }
    }
    const int timers = static_cast<int>(m_store.data().timers.size());
    int openTasks = 0;
    for (const auto& todo : m_store.data().todos) {
        if (!todo.done) {
            ++openTasks;
        }
    }

    const QString alarmsText = countLabel(alarms, tr("будильник"), tr("будильника"),
                                          tr("будильников"));
    const QString timersText = countLabel(timers, tr("таймер"), tr("таймера"), tr("таймеров"));
    QString statusText = QStringLiteral("%1 · %2").arg(alarmsText, timersText);
    if (openTasks > 0) {
        statusText += QStringLiteral(" · ") + countLabel(openTasks, tr("задача"), tr("задачи"),
                                                         tr("задач"));
    }
    m_statusCounts->setText(statusText);
    m_nav->setBadge(QStringLiteral("alarms"),
                    enabledAlarms > 0 ? QString::number(enabledAlarms) : QString());
    m_nav->setBadge(QStringLiteral("timers"), timers > 0 ? QString::number(timers) : QString());
    m_nav->setBadge(QStringLiteral("tasks"), openTasks > 0 ? QString::number(openTasks) : QString());
    updateStatusIcons();
}

void MainWindow::updateStatusIcons()
{
    const ThemeColors& colors = ThemeManager::instance().current();
    if (m_statusIcon != nullptr) {
        m_statusIcon->setPixmap(icons::pixmap(icons::Icon::Alarm, 14, colors.statusText));
    }
    updateMuteIcon();
}

void MainWindow::updateMuteIcon()
{
    if (m_muteButton == nullptr) {
        return;
    }
    const QColor color = ThemeManager::instance().current().statusText;
    m_muteButton->setIcon(icons::qicon(m_muted ? icons::Icon::Mute : icons::Icon::Volume, color));
    m_muteButton->setToolTip(m_muted ? tr("Включить звук") : tr("Выключить звук"));
    m_muteButton->setChecked(m_muted);
}

void MainWindow::toggleMute(bool muted)
{
    m_muted = muted;
    Settings::instance().raw().setValue(QStringLiteral("notifications/muted"), muted);
    Settings::instance().raw().sync();
    if (muted) {
        SoundPlayer::instance().stop();
    }
    updateMuteIcon();
}

void MainWindow::showFileMenu(const QPoint& globalPosition)
{
    const QColor accent = ThemeManager::instance().current().accent;
    QMenu menu(this);
    auto* newAlarmAction = menu.addAction(icons::qicon(icons::Icon::AlarmPlus, accent),
                                          tr("Новый будильник"));
    newAlarmAction->setShortcut(QKeySequence::New);
    connect(newAlarmAction, &QAction::triggered, this, &MainWindow::newAlarm);
    auto* newTimerAction = menu.addAction(icons::qicon(icons::Icon::Timer, accent),
                                          tr("Новый таймер"));
    connect(newTimerAction, &QAction::triggered, this, [this] {
        showPage(QStringLiteral("timers"));
        QMetaObject::invokeMethod(m_pageMap.value(QStringLiteral("timers")), "createTimer");
    });
    auto* newTaskAction = menu.addAction(icons::qicon(icons::Icon::TaskPlus, accent),
                                         tr("Новая задача"));
    connect(newTaskAction, &QAction::triggered, this, &MainWindow::newTask);
    menu.addSeparator();
    auto* calendarAction = menu.addAction(icons::qicon(icons::Icon::Calendar, accent),
                                          tr("Календарь"));
    connect(calendarAction, &QAction::triggered, this, &MainWindow::openCalendar);
    auto* settingsAction = menu.addAction(icons::qicon(icons::Icon::Settings, accent),
                                          tr("Настройки"));
    connect(settingsAction, &QAction::triggered, this, &MainWindow::openSettings);
    menu.addSeparator();
    auto* folderAction = menu.addAction(icons::qicon(icons::Icon::Folder, accent),
                                        tr("Открыть папку с данными"));
    connect(folderAction, &QAction::triggered, this, [this] {
        platform::revealInExplorer(m_store.dataFilePath());
    });
    auto* helpAction = menu.addAction(icons::qicon(icons::Icon::Help, accent), tr("Справка"));
    connect(helpAction, &QAction::triggered, this, &MainWindow::showHelp);
    menu.addSeparator();
    auto* quitAction = menu.addAction(icons::qicon(icons::Icon::Close, accent), tr("Выход"));
    connect(quitAction, &QAction::triggered, this, [this] {
        m_quitting = true;
        saveWindowState();
        qApp->quit();
    });
    menu.exec(globalPosition);
}

void MainWindow::applyChromeMode()
{
    const bool fluent = ThemeManager::instance().current().flavor == ThemeFlavor::WindowsClock;
    if (m_ribbon != nullptr) {
        m_ribbon->setVisible(!fluent);
    }
    if (m_nav != nullptr) {
        m_nav->setFluentMode(fluent);
    }
    if (m_titleBar != nullptr) {
        m_titleBar->setFluentMode(fluent);
    }
    for (auto* page : m_pageMap) {
        if (page != nullptr) {
            page->applyThemeMode();
        }
    }
    applyPanelSettings();
    updateResponsiveLayout();
}

void MainWindow::updateResponsiveLayout()
{
    const bool fluent = ThemeManager::instance().current().flavor == ThemeFlavor::WindowsClock;
    if (m_nav != nullptr) {
        m_nav->setCompact(width() < (fluent ? 860 : 940));
    }
    if (m_titleBar != nullptr) {
        m_titleBar->setClockVisible(!fluent && width() >= 880);
    }
    if (m_statusCounts != nullptr) {
        m_statusCounts->setVisible(!fluent && width() >= 980);
    }
    updateStatusText();
}

void MainWindow::updateStatusText()
{
    if (m_statusNext != nullptr) {
        m_statusNext->setToolTip(m_statusNextText);
    }
}

void MainWindow::setAlwaysOnTop(bool enabled)
{
    if (m_alwaysOnTop == enabled) {
        if (m_alwaysOnTopAction != nullptr) {
            m_alwaysOnTopAction->setChecked(enabled);
        }
        return;
    }
    m_alwaysOnTop = enabled;
    Settings::instance().raw().setValue(QStringLiteral("ui/alwaysOnTop"), enabled);
    Settings::instance().raw().sync();
    if (m_alwaysOnTopAction != nullptr) {
        m_alwaysOnTopAction->setChecked(enabled);
    }

    const bool visible = isVisible();
    const bool maximized = isMaximized();
    const bool fullScreen = isFullScreen();
    const bool minimized = isMinimized();
    const QRect normalRect = geometry();

    setWindowFlag(Qt::WindowStaysOnTopHint, enabled);

    if (visible) {
        if (fullScreen) {
            showFullScreen();
        } else if (maximized) {
            showMaximized();
        } else {
            show();
            setGeometry(normalRect);
            if (minimized) {
                showMinimized();
            }
        }
        if (!minimized) {
            raise();
            activateWindow();
        }
    }
    ensureNativeResizeFrame();
}

void MainWindow::applyPanelSettings()
{
    const bool fluent = ThemeManager::instance().current().flavor == ThemeFlavor::WindowsClock;
    if (m_nav != nullptr) {
        m_nav->setVisible(fluent || Settings::instance().showNavPane());
    }
    if (statusBar() != nullptr) {
        statusBar()->setVisible(!fluent && Settings::instance().showStatusBar());
    }
    if (m_ribbon != nullptr) {
        m_ribbon->setVisible(!fluent);
    }
    if (m_navToggle != nullptr) {
        m_navToggle->setChecked(Settings::instance().showNavPane());
    }
    if (m_statusToggle != nullptr) {
        m_statusToggle->setChecked(Settings::instance().showStatusBar());
    }
    if (m_secondsToggle != nullptr) {
        m_secondsToggle->setChecked(Settings::instance().showSecondsInTitle());
    }
}

void MainWindow::toggleNavigation(bool enabled)
{
    Settings::instance().raw().setValue(QStringLiteral("appearance/showNavPane"), enabled);
    Settings::instance().notifyChanged();
}

void MainWindow::showAndRaise()
{
    if (isMinimized()) {
        showNormal();
    } else {
        show();
    }
    raise();
    activateWindow();
    QApplication::alert(this);
}

void MainWindow::saveWindowState()
{
    QSettings& settings = Settings::instance().raw();
    settings.setValue(QStringLiteral("ui/geometry"), saveGeometry());
    settings.setValue(QStringLiteral("ui/maximized"), isMaximized());
    settings.setValue(QStringLiteral("ui/lastPage"), m_nav != nullptr ? m_nav->currentId()
                                                                       : QStringLiteral("alarms"));
    settings.sync();
}

void MainWindow::restoreWindowState()
{
    QSettings& settings = Settings::instance().raw();
    const QByteArray geometry = settings.value(QStringLiteral("ui/geometry")).toByteArray();
    bool restored = false;
    if (!geometry.isEmpty()) {
        restored = restoreGeometry(geometry);
    }
    if (!restored) {
        resize(1280, 840);
        if (const QScreen* screen = this->screen()) {
            move(screen->availableGeometry().center() - rect().center());
        }
    }
    if (settings.value(QStringLiteral("ui/maximized"), false).toBool()) {
        setWindowState(windowState() | Qt::WindowMaximized);
    }
}

void MainWindow::newAlarm()
{
    showPage(QStringLiteral("alarms"));
    QMetaObject::invokeMethod(m_pageMap.value(QStringLiteral("alarms")), "createAlarm");
}

void MainWindow::newTimer()
{
    showPage(QStringLiteral("timers"));
    QMetaObject::invokeMethod(m_pageMap.value(QStringLiteral("timers")), "createTimer");
}

void MainWindow::newGroup()
{
    showPage(QStringLiteral("alarms"));
    QMetaObject::invokeMethod(m_pageMap.value(QStringLiteral("alarms")), "createGroup");
}

void MainWindow::newTask()
{
    showPage(QStringLiteral("tasks"));
    QMetaObject::invokeMethod(m_pageMap.value(QStringLiteral("tasks")), "createTask");
}

void MainWindow::editSelected()
{
    QMetaObject::invokeMethod(m_currentPage, "editSelected");
}

void MainWindow::duplicateSelected()
{
    QMetaObject::invokeMethod(m_currentPage, "duplicateSelected");
}

void MainWindow::deleteSelected()
{
    QMetaObject::invokeMethod(m_currentPage, "deleteSelected");
}

void MainWindow::openCalendar()
{
    showPage(QStringLiteral("calendar"));
}

void MainWindow::openSettings()
{
    SettingsDialog dialog(this);
    dialog.exec();
}

void MainWindow::showAbout()
{
    QMessageBox::about(
        this, tr("О программе"),
        tr("<h3>Perfect Alarm Clock</h3>"
           "<p>Будильники, таймеры, секундомер, помодоро, календарь и мировое время "
           "в одном приложении.</p>"
           "<p><b>Версия 1.0</b><br>Qt %1 · C++20 · CMake</p>")
            .arg(QString::fromLatin1(qVersion())));
}

void MainWindow::showHelp()
{
    QMessageBox::information(
        this, tr("Справка"),
        tr("<h3>Быстрый старт</h3>"
           "<p><b>Ctrl+N</b> — новый будильник.<br>"
           "<b>Ctrl+F</b> — поиск по будильникам.<br>"
           "<b>Ctrl+,</b> — настройки.</p>"
           "<p>Двойной щелчок по строке открывает редактор. "
           "Щелчок по флажку включает или выключает будильник. "
           "При закрытии окна приложение остаётся в трее, чтобы будильники сработали.</p>"));
}

void MainWindow::handleAlarmFired(const QUuid& id)
{
    showAlarmPopup(id);
}

void MainWindow::showPreAlertPopup(const QUuid& id, int minutes)
{
    if (!Settings::instance().preAlertsEnabled()) {
        return;
    }
    const Alarm* alarm = m_store.findAlarm(id);
    if (alarm == nullptr) {
        return;
    }
    const QString title = alarm->label.isEmpty() ? tr("Будильник") : alarm->label;
    const QString details = tr("Через %1 мин · %2")
                                .arg(minutes)
                                .arg(schedule::timeText(alarm->time, Settings::instance().use24h()));
    auto* popup = new NotificationPopup();
    popup->setContent(tr("СКОРО БУДИЛЬНИК"), title, details,
                      alarm->icon.isEmpty() ? QStringLiteral("⏰") : alarm->icon,
                      QColor(alarm->color), tr("Напоминание"), QString(),
                      icons::Icon::Bell, tr("Понятно"));
    popup->setActivateOnShow(false);
    QPointer<NotificationPopup> guarded(popup);
    connect(popup, &NotificationPopup::dismissed, popup, &QWidget::close);
    connect(popup, &QObject::destroyed, this, [this] {
        m_popups.erase(std::remove_if(m_popups.begin(), m_popups.end(),
                                      [](const QPointer<NotificationPopup>& item) {
                                          return item.isNull();
                                      }),
                       m_popups.end());
        repositionPopups();
    });
    m_popups.append(guarded);
    repositionPopups();
    popup->show();
    popup->raise();
    QTimer::singleShot(12'000, popup, &QWidget::close);
}

void MainWindow::showAlarmPopup(const QUuid& id)
{
    Alarm* alarm = m_store.findAlarm(id);
    if (alarm == nullptr) {
        return;
    }
    const QString title = alarm->label.isEmpty() ? tr("Будильник") : alarm->label;
    platform::flashTaskbar();
    const int playVolume = std::clamp(alarm->volume
                                          + alarm->snoozeCount * alarm->snoozeVolumeIncrease,
                                      0, 100);
    const int fadeOutSeconds = alarm->fadeOutSeconds;
    QString alarmSoundId = alarm->soundId;
    if (alarm->randomSound) {
        const auto& sounds = catalog::availableSounds();
        if (!sounds.empty()) {
            alarmSoundId = sounds.at(
                QRandomGenerator::global()->bounded(static_cast<int>(sounds.size()))).id;
        }
    } else if (alarm->snoozeCount > 0 && !alarm->repeatSoundId.isEmpty()) {
        alarmSoundId = alarm->repeatSoundId;
    }
    if (SoundPlayer::resolveSoundPath(alarmSoundId).isEmpty()) {
        alarmSoundId = QStringLiteral("builtin:chime");
    }
    if (!m_muted && alarm->soundEnabled) {
        SoundPlayer::instance().play(alarmSoundId, playVolume, alarm->loopSound,
                                     alarm->maxSoundMinutes, alarm->fadeIn,
                                     alarm->fadeInSeconds);
    }
    const bool isOnce = alarm->repeat == RepeatKind::Once;
    if (!alarm->popupEnabled) {
        if (isOnce) {
            alarm->enabled = false;
            alarm->snoozeUntil = QDateTime();
            alarm->snoozeCount = 0;
            m_store.commit(Domain::Alarms);
            QMetaObject::invokeMethod(m_pageMap.value(QStringLiteral("alarms")), "reload");
        }
        updateNextAlarm();
        updateCounts();
        return;
    }

    int snoozeMinutes = std::max(1, alarm->snoozeMinutes);
    int snoozeAlt = std::max(1, alarm->snoozeMinutesAlt);
    if (alarm->snoozeDecrease) {
        const int reduction = alarm->snoozeCount * std::max(1, alarm->snoozeDecreaseStep);
        snoozeMinutes = std::max(1, snoozeMinutes - reduction);
        snoozeAlt = std::max(1, snoozeAlt - reduction);
    }
    const bool canSnooze = alarm->snoozeEnabled && alarm->maxSnoozes > 0
                           && alarm->snoozeCount < alarm->maxSnoozes;
    QList<int> snoozeOptions;
    if (canSnooze) {
        snoozeOptions.append(snoozeMinutes);
        if (snoozeAlt != snoozeMinutes) {
            snoozeOptions.append(snoozeAlt);
        }
    }
    const QString nextAt = QTime::currentTime()
                               .addSecs(static_cast<qint64>(snoozeMinutes) * 60)
                               .toString(QStringLiteral("HH:mm"));
    const QString hint = canSnooze ? tr("до %1").arg(nextAt)
                                   : tr("Лимит отложений");
    const QString timeLine = schedule::timeText(alarm->time, Settings::instance().use24h());
    const QString details = timeLine + QStringLiteral(" · ") + schedule::repeatSummary(*alarm);
    auto* popup = new NotificationPopup();
    popup->setContent(tr("БУДИЛЬНИК"), title, details,
                      alarm->icon.isEmpty() ? QStringLiteral("⏰") : alarm->icon,
                      QColor(alarm->color), hint,
                      canSnooze ? tr("Отложить") : QString(),
                      icons::Icon::Snooze, isOnce ? tr("Выключить") : tr("Остановить"));
    popup->setSnoozeOptions(snoozeOptions, snoozeMinutes);
    QPointer<NotificationPopup> guarded(popup);
    connect(popup, &NotificationPopup::primaryClicked, this,
            [this, guarded, id, snoozeMinutes, fadeOutSeconds] {
                const int chosen = guarded != nullptr ? guarded->selectedSnoozeMinutes() : 0;
                const int minutes = chosen > 0 ? chosen : snoozeMinutes;
                Alarm* current = m_store.findAlarm(id);
                if (current != nullptr) {
                    current->snoozeUntil = QDateTime::currentDateTime().addSecs(
                        static_cast<qint64>(minutes) * 60);
                    ++current->snoozeCount;
                    m_store.commit(Domain::Alarms);
                    QMetaObject::invokeMethod(m_pageMap.value(QStringLiteral("alarms")), "reload");
                }
                if (guarded != nullptr) {
                    guarded->close();
                }
                stopSoundIfNoPopups(fadeOutSeconds);
                updateNextAlarm();
                updateCounts();
            });
    connect(popup, &NotificationPopup::dismissed, this,
            [this, guarded, id, isOnce, fadeOutSeconds] {
        Alarm* current = m_store.findAlarm(id);
        if (current != nullptr) {
            const bool limitReached = current->maxSnoozes > 0
                                      && current->snoozeCount >= current->maxSnoozes;
            current->snoozeUntil = QDateTime();
            current->snoozeCount = 0;
            if (isOnce
                || (limitReached
                    && current->afterLastSnooze == SnoozeEndAction::DisableAlarm)
                || current->disableAfterDismiss) {
                current->enabled = false;
            }
            m_store.commit(Domain::Alarms);
            QMetaObject::invokeMethod(m_pageMap.value(QStringLiteral("alarms")), "reload");
        }
        Alarm postAlarm;
        bool hasPost = false;
        if (Alarm* updated = m_store.findAlarm(id); updated != nullptr) {
            postAlarm = *updated;
            hasPost = postAlarm.postSummary || postAlarm.postTasks
                      || !postAlarm.postMessage.isEmpty() || !postAlarm.postOpen.isEmpty();
        }
        if (guarded != nullptr) {
            guarded->close();
        }
        stopSoundIfNoPopups(fadeOutSeconds);
        updateNextAlarm();
        updateCounts();
        if (hasPost) {
            showPostAlarmPopup(postAlarm);
        }
    });
    connect(popup, &QObject::destroyed, this, [this] {
        m_popups.erase(std::remove_if(m_popups.begin(), m_popups.end(),
                                      [](const QPointer<NotificationPopup>& item) {
                                          return item.isNull();
                                      }),
                       m_popups.end());
        repositionPopups();
    });
    m_popups.append(guarded);
    repositionPopups();
    popup->show();
    popup->raise();
    popup->activateWindow();
    popup->setFocus(Qt::OtherFocusReason);

    if (alarm->backupEnabled) {
        const int backupMinutes = std::max(1, alarm->backupAfterMinutes);
        const bool persistent = alarm->persistent;
        QString backupSound = alarm->repeatSoundId.isEmpty() ? alarm->soundId
                                                             : alarm->repeatSoundId;
        if (SoundPlayer::resolveSoundPath(backupSound).isEmpty()) {
            backupSound = QStringLiteral("builtin:chime");
        }
        const int backupVolume = std::min(100, playVolume + 15);
        auto* backupTimer = new QTimer(popup);
        backupTimer->setSingleShot(!persistent);
        connect(backupTimer, &QTimer::timeout, this,
                [this, guarded, id, backupSound, backupVolume, persistent, backupTimer,
                 backupMinutes] {
                    if (guarded == nullptr || !guarded->isVisible()) {
                        return;
                    }
                    Alarm* current = m_store.findAlarm(id);
                    const QDateTime now = QDateTime::currentDateTime();
                    if (current == nullptr || !current->enabled
                        || (current->snoozeUntil.isValid() && current->snoozeUntil > now)) {
                        return;
                    }
                    if (!m_muted) {
                        SoundPlayer::instance().play(backupSound, backupVolume, true, 0, false);
                    }
                    guarded->raise();
                    guarded->activateWindow();
                    guarded->setFocus(Qt::OtherFocusReason);
                    if (persistent) {
                        backupTimer->start(backupMinutes * 60 * 1000);
                    }
                });
        backupTimer->start(backupMinutes * 60 * 1000);
    }
}

void MainWindow::showPostAlarmPopup(const Alarm& alarm)
{
    if (alarm.postOpen.trimmed().isEmpty() && !alarm.postSummary && !alarm.postTasks
        && alarm.postMessage.trimmed().isEmpty()) {
        return;
    }

    const QString target = alarm.postOpen.trimmed();
    if (!target.isEmpty()) {
        const QUrl url = QFileInfo::exists(target) ? QUrl::fromLocalFile(target)
                                                   : QUrl::fromUserInput(target);
        QDesktopServices::openUrl(url);
    }

    const QDateTime now = QDateTime::currentDateTime();
    QStringList lines;
    if (alarm.postSummary) {
        lines.append(tr("Сегодня: %1, %2 %3 %4")
                         .arg(catalog::dayLong(static_cast<Qt::DayOfWeek>(now.date().dayOfWeek())),
                              QString::number(now.date().day()),
                              catalog::monthNameGenitive(now.date().month()),
                              QString::number(now.date().year())));
        const QDateTime next = [this, &now] {
            QDateTime best;
            for (const auto& entry : m_store.data().alarms) {
                if (!entry.enabled) {
                    continue;
                }
                const QDateTime candidate = schedule::nextOccurrence(entry, now);
                if (candidate.isValid() && (!best.isValid() || candidate < best)) {
                    best = candidate;
                }
            }
            return best;
        }();
        if (next.isValid()) {
            lines.append(tr("Следующий будильник: %1")
                             .arg(schedule::dateTimeText(next, now,
                                                         Settings::instance().use24h())));
        }
    }
    if (alarm.postTasks) {
        QStringList tasks;
        for (const auto& todo : m_store.data().todos) {
            if (!todo.done && todo.hasDue && todo.dueDate == now.date()) {
                tasks.append(todo.title);
            }
        }
        lines.append(tasks.isEmpty() ? tr("Задач на сегодня нет.")
                                     : tr("Задачи на сегодня: %1").arg(tasks.size()));
        for (int i = 0; i < tasks.size() && i < 6; ++i) {
            lines.append(QStringLiteral("• ") + tasks.at(i));
        }
    }
    if (!alarm.postMessage.trimmed().isEmpty()) {
        lines.append(alarm.postMessage.trimmed());
    }
    if (alarm.postSummary) {
        static const QStringList quotes = {
            tr("Сегодня хороший день, чтобы начать с одного небольшого шага."),
            tr("Спокойное утро задаёт ритм всему дню."),
            tr("Сделайте самое важное — остальное подождёт."),
            tr("Маленькие привычки создают большие перемены."),
            tr("Время — это единственный ресурс, который нельзя вернуть."),
            tr("Сначала дыхание, потом решения."),
            tr("Один шаг вперёд лучше идеального плана.")
        };
        lines.append(QStringLiteral("— ") + quotes.at(now.date().dayOfYear() % quotes.size()));
    }

    if (alarm.postSummary || alarm.postTasks || !alarm.postMessage.trimmed().isEmpty()) {
        QMessageBox box(this);
        box.setWindowTitle(tr("После будильника"));
        box.setWindowFlag(Qt::WindowStaysOnTopHint, true);
        box.setIcon(QMessageBox::Information);
        box.setText(alarm.label.isEmpty() ? tr("Доброе утро") : alarm.label);
        box.setTextFormat(Qt::PlainText);
        box.setInformativeText(lines.join(QStringLiteral("\n")));
        auto* tasksButton = alarm.postTasks
                                ? box.addButton(tr("Открыть задачи"), QMessageBox::ActionRole)
                                : nullptr;
        auto* closeButton = box.addButton(QMessageBox::Close);
        closeButton->setText(tr("Закрыть"));
        box.setDefaultButton(closeButton);
        box.exec();
        if (tasksButton != nullptr && box.clickedButton() == tasksButton) {
            showAndRaise();
            showPage(QStringLiteral("tasks"));
        }
    }
}

void MainWindow::handleTimerFinished(const QUuid& id)
{
    const CountdownTimer* timer = m_store.findTimer(id);
    if (timer == nullptr) {
        return;
    }
    if (!m_muted && timer->soundEnabled) {
        SoundPlayer::instance().play(timer->soundId, timer->volume, timer->loopSound,
                                     timer->popupEnabled ? 0 : 2, false);
    }
    if (!timer->popupEnabled) {
        return;
    }
    const QString title = timer->label.isEmpty() ? tr("Таймер") : timer->label;
    const QString details = tr("Время вышло · %1")
                                .arg(schedule::formatTimer(static_cast<qint64>(timer->durationSec) * 1000));
    auto* popup = new NotificationPopup();
    popup->setContent(tr("ТАЙМЕР"), title, details, QStringLiteral("⏳"),
                      QColor(timer->color), tr("Готово"),
                      tr("Заново"), icons::Icon::Reset, tr("Закрыть"));
    QPointer<NotificationPopup> guarded(popup);
    connect(popup, &NotificationPopup::primaryClicked, this, [this, guarded, id] {
        CountdownTimer* current = m_store.findTimer(id);
        if (current != nullptr) {
            current->state = TimerState::Running;
            current->endAt = QDateTime::currentDateTime().addSecs(current->durationSec);
            current->remainingMs = static_cast<qint64>(current->durationSec) * 1000;
            m_store.commit(Domain::Timers);
        }
        if (guarded != nullptr) {
            guarded->close();
        }
        stopSoundIfNoPopups();
    });
    connect(popup, &NotificationPopup::dismissed, this, [this, guarded] {
        if (guarded != nullptr) {
            guarded->close();
        }
        stopSoundIfNoPopups();
    });
    connect(popup, &QObject::destroyed, this, [this] {
        m_popups.erase(std::remove_if(m_popups.begin(), m_popups.end(),
                                      [](const QPointer<NotificationPopup>& item) {
                                          return item.isNull();
                                      }),
                       m_popups.end());
        repositionPopups();
    });
    m_popups.append(guarded);
    repositionPopups();
    popup->show();
    popup->raise();
    popup->activateWindow();
    popup->setFocus(Qt::OtherFocusReason);
}

void MainWindow::repositionPopups()
{
    const QScreen* target = screen() != nullptr ? screen() : QApplication::primaryScreen();
    if (target == nullptr) {
        return;
    }
    const QRect area = target->availableGeometry();
    int bottom = area.bottom() - 12;
    for (auto it = m_popups.rbegin(); it != m_popups.rend(); ++it) {
        NotificationPopup* popup = it->data();
        if (popup == nullptr) {
            continue;
        }
        popup->move(area.right() - popup->width() - 12, bottom - popup->height());
        bottom -= popup->height() + 8;
    }
}

void MainWindow::stopSoundIfNoPopups(int fadeOutSeconds)
{
    for (const auto& popup : m_popups) {
        if (!popup.isNull() && popup->isVisible() && popup->isUrgent()) {
            return;
        }
    }
    if (fadeOutSeconds > 0) {
        SoundPlayer::instance().stopWithFade(fadeOutSeconds);
    } else {
        SoundPlayer::instance().stop();
    }
}

bool MainWindow::nativeEvent(const QByteArray& eventType, void* message, qintptr* result)
{
#ifdef Q_OS_WIN
    if (eventType == QByteArrayLiteral("windows_generic_MSG")) {
        auto* msg = static_cast<MSG*>(message);
        if (msg->message == WM_NCHITTEST && !isMaximized() && !isFullScreen()) {
            RECT windowRect{};
            if (GetWindowRect(reinterpret_cast<HWND>(winId()), &windowRect) != FALSE) {
                const int x = GET_X_LPARAM(msg->lParam);
                const int y = GET_Y_LPARAM(msg->lParam);
                const int border = std::max(6, qRound(8.0 * devicePixelRatioF()));
                const bool left = x >= windowRect.left && x < windowRect.left + border;
                const bool right = x < windowRect.right && x >= windowRect.right - border;
                const bool top = y >= windowRect.top && y < windowRect.top + border;
                const bool bottom = y < windowRect.bottom && y >= windowRect.bottom - border;
                if (top && left) {
                    *result = HTTOPLEFT;
                    return true;
                }
                if (top && right) {
                    *result = HTTOPRIGHT;
                    return true;
                }
                if (bottom && left) {
                    *result = HTBOTTOMLEFT;
                    return true;
                }
                if (bottom && right) {
                    *result = HTBOTTOMRIGHT;
                    return true;
                }
                if (left) {
                    *result = HTLEFT;
                    return true;
                }
                if (right) {
                    *result = HTRIGHT;
                    return true;
                }
                if (top) {
                    *result = HTTOP;
                    return true;
                }
                if (bottom) {
                    *result = HTBOTTOM;
                    return true;
                }
            }
        }
    }
#else
    Q_UNUSED(eventType)
    Q_UNUSED(message)
    Q_UNUSED(result)
#endif
    return QMainWindow::nativeEvent(eventType, message, result);
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_pages && event->type() == QEvent::Resize && m_effects != nullptr) {
        m_effects->setGeometry(m_pages->rect());
        if (m_currentPage != nullptr) {
            m_effects->stackUnder(m_currentPage);
        }
    }
    if (event->type() == QEvent::MouseButtonPress && isVisible() && !isMaximized()
        && !isFullScreen() && windowHandle() != nullptr) {
        auto* mouseEvent = static_cast<QMouseEvent*>(event);
        auto* target = qobject_cast<QWidget*>(watched);
        if (mouseEvent->button() == Qt::LeftButton && target != nullptr
            && target->window() == this) {
            const QPoint local = mapFromGlobal(mouseEvent->globalPosition().toPoint());
            constexpr int border = 6;
            Qt::Edges edges;
            if (local.x() <= border) {
                edges |= Qt::LeftEdge;
            } else if (local.x() >= width() - border) {
                edges |= Qt::RightEdge;
            }
            if (local.y() <= border) {
                edges |= Qt::TopEdge;
            } else if (local.y() >= height() - border) {
                edges |= Qt::BottomEdge;
            }
            if (edges != 0) {
                windowHandle()->startSystemResize(edges);
                return true;
            }
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (m_quitting) {
        saveWindowState();
        event->accept();
        return;
    }
    if (Settings::instance().closeToTray() && m_tray != nullptr && m_tray->isVisible()) {
        saveWindowState();
        hide();
        event->ignore();
        return;
    }
    m_quitting = true;
    saveWindowState();
    event->accept();
    qApp->quit();
}

void MainWindow::showEvent(QShowEvent* event)
{
    QMainWindow::showEvent(event);
    ensureNativeResizeFrame();
    if (m_effects != nullptr) {
        m_effects->setSuspended(false);
        if (m_currentPage != nullptr) {
            m_effects->stackUnder(m_currentPage);
        }
        QTimer::singleShot(60, m_effects, &ThemeEffects::restartEffect);
    }
    if (m_clockTimer != nullptr && !m_clockTimer->isActive()) {
        updateClock();
        scheduleClockTick();
    }
    if (m_currentPage != nullptr && !m_currentPage->isActive()) {
        m_currentPage->activate();
    }
}

void MainWindow::hideEvent(QHideEvent* event)
{
    QMainWindow::hideEvent(event);
    if (m_effects != nullptr) {
        m_effects->setSuspended(true);
    }
    if (m_clockTimer != nullptr) {
        m_clockTimer->stop();
    }
    if (m_currentPage != nullptr) {
        m_currentPage->deactivate();
    }
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    updateResponsiveLayout();
}

void MainWindow::changeEvent(QEvent* event)
{
    QMainWindow::changeEvent(event);
    if (event->type() != QEvent::WindowStateChange) {
        return;
    }
    if (m_titleBar != nullptr) {
        m_titleBar->setMaximized(isMaximized());
        m_titleBar->setVisible(!isFullScreen());
    }
    ensureNativeResizeFrame();
    if (isMinimized()) {
        if (m_effects != nullptr) {
            m_effects->setSuspended(true);
        }
        if (m_clockTimer != nullptr) {
            m_clockTimer->stop();
        }
        if (m_currentPage != nullptr) {
            m_currentPage->deactivate();
        }
        if (Settings::instance().minimizeToTray() && m_tray != nullptr && m_tray->isVisible()) {
            QTimer::singleShot(0, this, [this] { hide(); });
        }
    } else if (isVisible()) {
        if (m_effects != nullptr) {
            m_effects->setSuspended(false);
        }
        if (m_clockTimer != nullptr && !m_clockTimer->isActive()) {
            updateClock();
            scheduleClockTick();
        }
        if (m_currentPage != nullptr && !m_currentPage->isActive()) {
            m_currentPage->activate();
        }
    }
}

} // namespace pac::ui
