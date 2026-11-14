#include "PomodoroPage.h"

#include "NotificationPopup.h"
#include "PomodoroSettingsDialog.h"
#include "core/Platform.h"
#include "core/Fonts.h"
#include "core/Settings.h"
#include "core/SoundPlayer.h"
#include "core/Store.h"
#include "core/Theme.h"

#include <QBoxLayout>
#include <QFont>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QResizeEvent>
#include <QScreen>
#include <QScrollArea>
#include <QShortcut>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>
#include <numbers>

namespace pac::ui {
namespace {

const QColor kWorkColor(QStringLiteral("#d9534f"));
const QColor kShortColor(QStringLiteral("#4c9a62"));
const QColor kLongColor(QStringLiteral("#3f7fd6"));

} // namespace

class PomodoroRing final : public QWidget {
public:
    explicit PomodoroRing(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setFixedSize(336, 336);
        setAttribute(Qt::WA_TranslucentBackground);
    }

    void setProgress(qreal progress, const QColor& color)
    {
        m_progress = std::clamp(progress, 0.0, 1.0);
        m_color = color;
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event)
        const ThemeColors& colors = ThemeManager::instance().current();
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const qreal thickness = 12.0;
        const QRectF ring = QRectF(rect()).adjusted(thickness, thickness, -thickness, -thickness);

        QPen track(tinted(colors.border, 0.72), thickness, Qt::SolidLine, Qt::RoundCap);
        painter.setPen(track);
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(ring);

        if (m_progress > 0.001) {
            QPen progressPen(m_color, thickness, Qt::SolidLine, Qt::RoundCap);
            painter.setPen(progressPen);
            const int span = -static_cast<int>(m_progress * 360.0 * 16.0);
            painter.drawArc(ring, 90 * 16, span);

            const qreal angle = (90.0 - m_progress * 360.0) * (std::numbers::pi / 180.0);
            const QPointF center = ring.center();
            const qreal radius = ring.width() / 2.0;
            const QPointF dot(center.x() + std::cos(angle) * radius,
                              center.y() + std::sin(angle) * radius);
            painter.setPen(Qt::NoPen);
            painter.setBrush(m_color);
            painter.drawEllipse(dot, thickness / 2.0 + 2.0, thickness / 2.0 + 2.0);
            painter.setBrush(colors.card);
            painter.drawEllipse(dot, 3.0, 3.0);
        }
    }

private:
    qreal m_progress{0.0};
    QColor m_color{kWorkColor};
};

class SessionDots final : public QWidget {
public:
    explicit SessionDots(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setFixedSize(96, 12);
    }

    void setState(int completed, int total, const QColor& color)
    {
        m_completed = std::max(0, completed);
        m_total = std::max(1, total);
        m_color = color;
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event)
        const ThemeColors& colors = ThemeManager::instance().current();
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const qreal gap = 8.0;
        const qreal dot = 6.0;
        const qreal totalWidth = m_total * dot + (m_total - 1) * gap;
        qreal x = (width() - totalWidth) / 2.0;
        const qreal y = height() / 2.0;
        for (int i = 0; i < m_total; ++i) {
            if (i < m_completed) {
                painter.setPen(Qt::NoPen);
                painter.setBrush(m_color);
                painter.drawEllipse(QPointF(x + dot / 2.0, y), dot / 2.0, dot / 2.0);
            } else {
                painter.setPen(QPen(tinted(colors.dim, 0.8), 1.0));
                painter.setBrush(Qt::NoBrush);
                painter.drawEllipse(QPointF(x + dot / 2.0, y), dot / 2.0 - 0.5, dot / 2.0 - 0.5);
            }
            x += dot + gap;
        }
    }

private:
    int m_completed{0};
    int m_total{4};
    QColor m_color{kWorkColor};
};

class GoalBar final : public QWidget {
public:
    explicit GoalBar(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setFixedHeight(8);
    }

    void setProgress(qreal progress, const QColor& color)
    {
        m_progress = std::clamp(progress, 0.0, 1.0);
        m_color = color;
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event)
        const ThemeColors& colors = ThemeManager::instance().current();
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(Qt::NoPen);
        painter.setBrush(tinted(colors.border, 0.8));
        painter.drawRoundedRect(QRectF(rect()), 4.0, 4.0);
        if (m_progress > 0.0) {
            const QRectF fill(0.0, 0.0, width() * m_progress, height());
            painter.setBrush(m_color);
            painter.drawRoundedRect(fill, 4.0, 4.0);
        }
    }

private:
    qreal m_progress{0.0};
    QColor m_color{kWorkColor};
};

PomodoroPage::PomodoroPage(Store& store, QWidget* parent)
    : PageWidget(parent)
    , m_store(store)
{
    m_phaseMs = phaseDurationMs(m_phase);
    m_remainingMs = m_phaseMs;
    buildUi();
    applyResponsiveLayout();
    ensureStatsToday();
    updateStats();
    updateSettingsSummary();
    updateUi();
    connect(&m_store, &Store::pomodoroChanged, this, [this] {
        updateStats();
        updateSettingsSummary();
    });
}

int PomodoroPage::phaseMinutes(PomodoroPhase phase) const
{
    const auto& settings = m_store.data().pomodoro;
    switch (phase) {
    case PomodoroPhase::Work:
        return std::max(1, settings.workMinutes);
    case PomodoroPhase::ShortBreak:
        return std::max(1, settings.shortBreakMinutes);
    case PomodoroPhase::LongBreak:
        return std::max(1, settings.longBreakMinutes);
    }
    return 25;
}

qint64 PomodoroPage::phaseDurationMs(PomodoroPhase phase) const
{
    return static_cast<qint64>(phaseMinutes(phase)) * 60 * 1000;
}

QColor PomodoroPage::phaseColor(PomodoroPhase phase) const
{
    switch (phase) {
    case PomodoroPhase::Work:
        return kWorkColor;
    case PomodoroPhase::ShortBreak:
        return kShortColor;
    case PomodoroPhase::LongBreak:
        return kLongColor;
    }
    return kWorkColor;
}

QString PomodoroPage::phaseTitle(PomodoroPhase phase) const
{
    switch (phase) {
    case PomodoroPhase::Work:
        return tr("Фокус");
    case PomodoroPhase::ShortBreak:
        return tr("Короткий перерыв");
    case PomodoroPhase::LongBreak:
        return tr("Длинный перерыв");
    }
    return {};
}

QString PomodoroPage::formatTime(qint64 milliseconds) const
{
    const qint64 totalSeconds = (std::max<qint64>(0, milliseconds) + 999) / 1000;
    const qint64 seconds = totalSeconds % 60;
    const qint64 minutes = totalSeconds / 60;
    if (minutes >= 60) {
        return QStringLiteral("%1:%2:%3")
            .arg(minutes / 60)
            .arg(minutes % 60, 2, 10, QLatin1Char('0'))
            .arg(seconds, 2, 10, QLatin1Char('0'));
    }
    return QStringLiteral("%1:%2")
        .arg(minutes, 2, 10, QLatin1Char('0'))
        .arg(seconds, 2, 10, QLatin1Char('0'));
}

void PomodoroPage::buildUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 12, 16, 12);
    root->setSpacing(10);

    auto* header = new QHBoxLayout;
    header->setSpacing(9);
    auto* tomato = new QLabel(this);
    tomato->setPixmap(emojiPixmap(QStringLiteral("🍅"), 22));
    header->addWidget(tomato);
    auto* title = new QLabel(tr("Помодоро"), this);
    QFont titleFont = title->font();
    titleFont.setPointSizeF(12.0);
    titleFont.setWeight(QFont::DemiBold);
    title->setFont(titleFont);
    header->addWidget(title);
    auto* subtitle = new QLabel(tr("Одна задача за раз"), this);
    subtitle->setObjectName(QStringLiteral("Muted"));
    header->addWidget(subtitle);
    header->addStretch(1);
    auto* settingsButton = new ThemedButton(tr("Настройки"), icons::Icon::Settings, this);
    settingsButton->setMinimumHeight(28);
    connect(settingsButton, &QPushButton::clicked, this, &PomodoroPage::openSettings);
    header->addWidget(settingsButton);
    root->addLayout(header);

    m_contentScroll = new QScrollArea(this);
    m_contentScroll->setObjectName(QStringLiteral("Transparent"));
    m_contentScroll->setWidgetResizable(true);
    m_contentScroll->setFrameShape(QFrame::NoFrame);
    m_contentScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_contentScroll->viewport()->setAutoFillBackground(false);
    auto* contentHost = new QWidget(m_contentScroll);
    contentHost->setObjectName(QStringLiteral("Transparent"));
    m_contentLayout = new QBoxLayout(QBoxLayout::LeftToRight, contentHost);
    m_contentLayout->setContentsMargins(0, 0, 0, 0);
    m_contentLayout->setSpacing(10);

    auto* timerCard = makeCard(contentHost);
    auto* timerLayout = new QVBoxLayout(timerCard);
    timerLayout->setContentsMargins(22, 14, 22, 16);
    timerLayout->setSpacing(4);
    timerLayout->addStretch(1);

    m_ring = new PomodoroRing(timerCard);
    auto* ringLayout = new QVBoxLayout(m_ring);
    ringLayout->setContentsMargins(50, 40, 50, 38);
    ringLayout->setSpacing(2);
    ringLayout->addStretch(1);
    m_phaseLabel = new QLabel(m_ring);
    m_phaseLabel->setAlignment(Qt::AlignCenter);
    QFont phaseFont = m_phaseLabel->font();
    phaseFont.setPointSizeF(8.5);
    phaseFont.setWeight(QFont::DemiBold);
    m_phaseLabel->setFont(phaseFont);
    m_phaseLabel->setFixedHeight(22);
    ringLayout->addWidget(m_phaseLabel, 0, Qt::AlignHCenter);
    m_timeLabel = new QLabel(m_ring);
    m_timeLabel->setFont(fonts::digitFont(40.0, QFont::Light));
    m_timeLabel->setAlignment(Qt::AlignCenter);
    ringLayout->addWidget(m_timeLabel);
    m_sessionLabel = new QLabel(m_ring);
    m_sessionLabel->setObjectName(QStringLiteral("Muted"));
    m_sessionLabel->setAlignment(Qt::AlignCenter);
    ringLayout->addWidget(m_sessionLabel);
    ringLayout->addSpacing(2);
    m_dots = new SessionDots(m_ring);
    ringLayout->addWidget(m_dots, 0, Qt::AlignHCenter);
    ringLayout->addStretch(1);
    timerLayout->addWidget(m_ring, 0, Qt::AlignHCenter);

    auto* buttons = new QHBoxLayout;
    buttons->setSpacing(8);
    buttons->addStretch(1);
    m_startButton = new ThemedButton(tr("Старт"), icons::Icon::Play, timerCard, true);
    m_startButton->setMinimumSize(136, 34);
    m_startButton->setIconSizePx(16);
    m_skipButton = new ThemedButton(tr("Пропустить"), icons::Icon::SkipForward, timerCard);
    m_skipButton->setMinimumHeight(34);
    m_resetButton = new ThemedButton(tr("Сброс"), icons::Icon::Reset, timerCard);
    m_resetButton->setMinimumHeight(34);
    buttons->addWidget(m_startButton);
    buttons->addWidget(m_skipButton);
    buttons->addWidget(m_resetButton);
    buttons->addStretch(1);
    timerLayout->addSpacing(6);
    timerLayout->addLayout(buttons);
    m_hintLabel = new QLabel(timerCard);
    m_hintLabel->setObjectName(QStringLiteral("Faint"));
    m_hintLabel->setAlignment(Qt::AlignCenter);
    timerLayout->addWidget(m_hintLabel);
    timerLayout->addStretch(1);
    m_contentLayout->addWidget(timerCard, 1);

    auto* statsCard = makeCard(contentHost);
    m_statsCard = statsCard;
    statsCard->setFixedWidth(310);
    auto* statsLayout = new QVBoxLayout(statsCard);
    statsLayout->setContentsMargins(20, 16, 20, 16);
    statsLayout->setSpacing(8);
    auto* statsTitle = new QLabel(tr("Сегодня"), statsCard);
    QFont statsTitleFont = statsTitle->font();
    statsTitleFont.setPointSizeF(12.0);
    statsTitleFont.setWeight(QFont::DemiBold);
    statsTitle->setFont(statsTitleFont);
    statsLayout->addWidget(statsTitle);

    auto* countRow = new QHBoxLayout;
    countRow->setSpacing(5);
    m_completedCount = new QLabel(QStringLiteral("0"), statsCard);
    QFont countFont = m_completedCount->font();
    countFont.setPointSizeF(34.0);
    countFont.setWeight(QFont::Light);
    m_completedCount->setFont(countFont);
    countRow->addWidget(m_completedCount, 0, Qt::AlignBottom);
    m_goalCount = new QLabel(statsCard);
    m_goalCount->setObjectName(QStringLiteral("Muted"));
    QFont goalFont = m_goalCount->font();
    goalFont.setPointSizeF(13.0);
    m_goalCount->setFont(goalFont);
    countRow->addWidget(m_goalCount, 0, Qt::AlignBottom);
    countRow->addStretch(1);
    statsLayout->addLayout(countRow);
    auto* countCaption = new QLabel(tr("помидоров сфокусировано"), statsCard);
    countCaption->setObjectName(QStringLiteral("Muted"));
    statsLayout->addWidget(countCaption);
    m_goalBar = new GoalBar(statsCard);
    statsLayout->addWidget(m_goalBar);

    auto* divider = makeHorizontalRule(statsCard);
    statsLayout->addSpacing(6);
    statsLayout->addWidget(divider);
    statsLayout->addSpacing(6);

    const auto addStatRow = [statsCard, statsLayout](const QString& name, QLabel*& value) {
        auto* row = new QHBoxLayout;
        auto* label = new QLabel(name, statsCard);
        label->setObjectName(QStringLiteral("Muted"));
        row->addWidget(label);
        row->addStretch(1);
        value = new QLabel(statsCard);
        QFont font = value->font();
        font.setWeight(QFont::DemiBold);
        value->setFont(font);
        row->addWidget(value);
        statsLayout->addLayout(row);
    };
    addStatRow(tr("Фокус сегодня"), m_focusValue);
    addStatRow(tr("До цели"), m_remainingValue);

    auto* cycleCaption = new QLabel(tr("ЦИКЛ"), statsCard);
    cycleCaption->setObjectName(QStringLiteral("SectionLabel"));
    statsLayout->addSpacing(8);
    statsLayout->addWidget(cycleCaption);
    const auto addCycleRow = [statsCard, statsLayout](const QString& name, QLabel*& value) {
        auto* row = new QHBoxLayout;
        auto* label = new QLabel(name, statsCard);
        label->setObjectName(QStringLiteral("Muted"));
        row->addWidget(label);
        row->addStretch(1);
        value = new QLabel(statsCard);
        row->addWidget(value);
        statsLayout->addLayout(row);
    };
    addCycleRow(tr("Фокус"), m_workValue);
    addCycleRow(tr("Короткий перерыв"), m_shortValue);
    addCycleRow(tr("Длинный перерыв"), m_longValue);
    addCycleRow(tr("Длинный каждые"), m_everyValue);
    statsLayout->addStretch(1);
    auto* quote = new QLabel(tr("«Сделайте один шаг — и следующий станет проще»"), statsCard);
    quote->setObjectName(QStringLiteral("Faint"));
    quote->setWordWrap(true);
    statsLayout->addWidget(quote);
    m_contentLayout->addWidget(statsCard);
    m_contentScroll->setWidget(contentHost);
    root->addWidget(m_contentScroll, 1);

    connect(m_startButton, &QPushButton::clicked, this, &PomodoroPage::startPause);
    connect(m_skipButton, &QPushButton::clicked, this, &PomodoroPage::skipPhase);
    connect(m_resetButton, &QPushButton::clicked, this, &PomodoroPage::resetPhase);

    m_timer = new QTimer(this);
    m_timer->setInterval(250);
    m_timer->setTimerType(Qt::PreciseTimer);
    connect(m_timer, &QTimer::timeout, this, &PomodoroPage::tick);

}

void PomodoroPage::startPause()
{
    if (m_running) {
        m_remainingMs = m_endAt.isValid()
                            ? std::max<qint64>(0, QDateTime::currentDateTime().msecsTo(m_endAt))
                            : m_remainingMs;
        m_running = false;
        m_timer->stop();
    } else {
        if (m_remainingMs <= 0) {
            m_remainingMs = m_phaseMs;
        }
        m_endAt = QDateTime::currentDateTime().addMSecs(m_remainingMs);
        m_running = true;
        m_timer->setInterval(isActive() ? 250 : 1000);
        m_timer->start();
    }
    updateUi();
}

void PomodoroPage::resetPhase()
{
    m_running = false;
    m_timer->stop();
    m_phaseMs = phaseDurationMs(m_phase);
    m_remainingMs = m_phaseMs;
    m_endAt = QDateTime();
    updateUi();
}

void PomodoroPage::skipPhase()
{
    const bool wasRunning = m_running;
    m_running = false;
    m_timer->stop();
    advancePhase(false, wasRunning);
}

void PomodoroPage::tick()
{
    if (!m_running) {
        return;
    }
    const qint64 remaining = m_endAt.isValid()
                                 ? QDateTime::currentDateTime().msecsTo(m_endAt)
                                 : m_remainingMs;
    m_remainingMs = std::max<qint64>(0, remaining);
    if (m_remainingMs <= 0) {
        completePhase();
        return;
    }
    updateTimeDisplay();
}

void PomodoroPage::completePhase()
{
    const PomodoroPhase finished = m_phase;
    m_running = false;
    m_timer->stop();
    m_remainingMs = 0;

    if (finished == PomodoroPhase::Work) {
        ensureStatsToday();
        ++m_completedInSet;
        auto& stats = m_store.data().pomodoroStats;
        ++stats.completedToday;
        stats.focusMinutesToday += std::max(1, m_store.data().pomodoro.workMinutes);
        m_store.commit(Domain::Pomodoro);
    }

    const auto& settings = m_store.data().pomodoro;
    const bool startNext = finished == PomodoroPhase::Work ? settings.autoStartBreaks
                                                           : settings.autoStartWork;
    if (settings.soundEnabled
        && !Settings::instance().raw().value(QStringLiteral("notifications/muted"), false).toBool()) {
        SoundPlayer::instance().play(settings.soundId, settings.volume, false, 0, false);
    }
    advancePhase(true, startNext);
    platform::flashTaskbar();
    if (settings.notifications) {
        showCompletionPopup(finished, m_running);
    }
}

void PomodoroPage::advancePhase(bool completedNaturally, bool startNext)
{
    Q_UNUSED(completedNaturally)
    const PomodoroPhase finished = m_phase;
    PomodoroPhase next = PomodoroPhase::Work;
    if (finished == PomodoroPhase::Work) {
        next = m_completedInSet >= std::max(2, m_store.data().pomodoro.longBreakEvery)
                   ? PomodoroPhase::LongBreak
                   : PomodoroPhase::ShortBreak;
    } else if (finished == PomodoroPhase::ShortBreak) {
        next = PomodoroPhase::Work;
    } else if (finished == PomodoroPhase::LongBreak) {
        m_completedInSet = 0;
        next = PomodoroPhase::Work;
    }

    m_phase = next;
    m_phaseMs = phaseDurationMs(next);
    m_remainingMs = m_phaseMs;
    m_endAt = QDateTime();
    m_running = startNext;
    if (m_running) {
        m_endAt = QDateTime::currentDateTime().addMSecs(m_remainingMs);
        m_timer->setInterval(isActive() ? 250 : 1000);
        m_timer->start();
    } else {
        m_timer->stop();
    }
    updateUi();
}

void PomodoroPage::updateUi()
{
    if (m_timeLabel == nullptr) {
        return;
    }
    updateTimeDisplay();
    const QColor color = phaseColor(m_phase);
    m_phaseLabel->setText(phaseTitle(m_phase));
    updatePhasePill();
    if (m_phase == PomodoroPhase::Work) {
        const int every = std::max(2, m_store.data().pomodoro.longBreakEvery);
        m_sessionLabel->setText(tr("Сессия %1 из %2")
                                    .arg(std::min(m_completedInSet + 1, every))
                                    .arg(every));
    } else {
        m_sessionLabel->setText(m_phase == PomodoroPhase::LongBreak ? tr("Длинный перерыв")
                                                                    : tr("Короткий перерыв"));
    }
    m_dots->setState(m_completedInSet, std::max(2, m_store.data().pomodoro.longBreakEvery), color);
    m_startButton->setText(m_running ? tr("Пауза") : tr("Старт"));
    m_startButton->setGlyph(m_running ? icons::Icon::Pause : icons::Icon::Play);
    m_resetButton->setEnabled(m_running || m_remainingMs < m_phaseMs);
    const auto& settings = m_store.data().pomodoro;
    if (m_phase == PomodoroPhase::Work) {
        m_hintLabel->setText(settings.autoStartBreaks
                                 ? tr("Перерыв начнётся автоматически · пробел — пауза")
                                 : tr("Пробел — старт/пауза"));
    } else {
        m_hintLabel->setText(settings.autoStartWork
                                 ? tr("Фокус начнётся автоматически · пробел — пауза")
                                 : tr("Пробел — старт/пауза"));
    }
    updateStats();
    updateSettingsSummary();
}

void PomodoroPage::updateTimeDisplay()
{
    if (m_timeLabel == nullptr) {
        return;
    }
    m_timeLabel->setText(formatTime(m_remainingMs));
    const qreal progress = m_phaseMs > 0
                               ? 1.0 - static_cast<qreal>(m_remainingMs) / static_cast<qreal>(m_phaseMs)
                               : 0.0;
    m_ring->setProgress(progress, phaseColor(m_phase));
}

void PomodoroPage::updatePhasePill()
{
    const QColor color = phaseColor(m_phase);
    m_phaseLabel->setStyleSheet(
        QStringLiteral("background: rgba(%1,%2,%3,26); border: 1px solid rgba(%1,%2,%3,95);"
                       " border-radius: 10px; padding: 1px 12px; color: %4;")
            .arg(color.red())
            .arg(color.green())
            .arg(color.blue())
            .arg(color.name()));
}

void PomodoroPage::updateStats()
{
    if (m_completedCount == nullptr) {
        return;
    }
    const auto& settings = m_store.data().pomodoro;
    const auto& stats = m_store.data().pomodoroStats;
    const int goal = std::max(1, settings.dailyGoal);
    const int completed = std::max(0, stats.completedToday);
    const QColor goalColor = completed >= goal ? ThemeManager::instance().current().success
                                               : ThemeManager::instance().current().text;
    m_completedCount->setText(QString::number(completed));
    m_completedCount->setStyleSheet(QStringLiteral("color:%1;").arg(goalColor.name()));
    m_goalCount->setText(QStringLiteral("/ %1").arg(goal));
    m_focusValue->setText(tr("%1 мин").arg(stats.focusMinutesToday));
    m_remainingValue->setText(QString::number(std::max(0, goal - completed)));
    const qreal progress = static_cast<qreal>(std::min(completed, goal)) / goal;
    m_goalBar->setProgress(progress, completed >= goal ? ThemeManager::instance().current().success
                                                       : kWorkColor);
}

void PomodoroPage::updateSettingsSummary()
{
    if (m_workValue == nullptr) {
        return;
    }
    const auto& settings = m_store.data().pomodoro;
    m_workValue->setText(tr("%1 мин").arg(settings.workMinutes));
    m_shortValue->setText(tr("%1 мин").arg(settings.shortBreakMinutes));
    m_longValue->setText(tr("%1 мин").arg(settings.longBreakMinutes));
    m_everyValue->setText(tr("%1 помидора").arg(settings.longBreakEvery));
}

void PomodoroPage::ensureStatsToday()
{
    const QDate today = QDate::currentDate();
    auto& stats = m_store.data().pomodoroStats;
    if (stats.day == today) {
        return;
    }
    stats.day = today;
    stats.completedToday = 0;
    stats.focusMinutesToday = 0;
    stats.currentStreak = 0;
    m_store.commit(Domain::Pomodoro);
}

void PomodoroPage::openSettings()
{
    PomodoroSettingsDialog dialog(m_store.data().pomodoro, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    m_store.data().pomodoro = dialog.result();
    if (dialog.resetStatsRequested()) {
        auto& stats = m_store.data().pomodoroStats;
        stats.day = QDate::currentDate();
        stats.completedToday = 0;
        stats.focusMinutesToday = 0;
        stats.currentStreak = 0;
    }
    m_store.commit(Domain::Pomodoro);
    if (!m_running) {
        m_phaseMs = phaseDurationMs(m_phase);
        m_remainingMs = m_phaseMs;
    }
    updateUi();
}

void PomodoroPage::showCompletionPopup(PomodoroPhase finished, bool nextRunning)
{
    if (m_popup != nullptr) {
        m_popup->close();
    }
    const bool wasWork = finished == PomodoroPhase::Work;
    const QString title = wasWork ? tr("Фокус завершён") : tr("Перерыв завершён");
    QString details;
    if (m_phase == PomodoroPhase::ShortBreak) {
        details = tr("Сделайте короткий перерыв");
    } else if (m_phase == PomodoroPhase::LongBreak) {
        details = tr("Время для длинного перерыва");
    } else {
        details = tr("Пора вернуться к фокусу");
    }
    auto* popup = new NotificationPopup();
    const QString primary = nextRunning
                                ? QString()
                                : (m_phase == PomodoroPhase::Work ? tr("Начать фокус")
                                                                   : tr("Начать перерыв"));
    popup->setContent(tr("ПОМОДОРО"), title, details,
                      wasWork ? QStringLiteral("🍅") : QStringLiteral("☕"),
                      phaseColor(m_phase),
                      nextRunning ? tr("Следующий этап уже идёт") : tr("Готово"),
                      primary, icons::Icon::Play, tr("Закрыть"));
    connect(popup, &NotificationPopup::primaryClicked, this, [this, popup] {
        if (!m_running) {
            startPause();
        }
        popup->close();
    });
    connect(popup, &NotificationPopup::dismissed, popup, &QWidget::close);
    m_popup = popup;
    if (const QScreen* screen = this->screen() != nullptr ? this->screen()
                                                           : QGuiApplication::primaryScreen()) {
        const QRect area = screen->availableGeometry();
        popup->move(area.right() - popup->width() - 14, area.bottom() - popup->height() - 14);
    }
    popup->show();
    popup->raise();
}

void PomodoroPage::refreshClock(const QDateTime& now)
{
    Q_UNUSED(now)
    ensureStatsToday();
}

void PomodoroPage::primaryAction()
{
    if (isActive()) {
        startPause();
    }
}

void PomodoroPage::onActivated()
{
    ensureStatsToday();
    updateUi();
    if (m_running) {
        m_timer->setInterval(250);
        if (!m_timer->isActive()) {
            m_timer->start();
        }
    }
}

void PomodoroPage::onDeactivated()
{
    if (m_running) {
        m_timer->setInterval(1000);
    }
}

void PomodoroPage::themeUpdated()
{
    updateUi();
}

void PomodoroPage::resizeEvent(QResizeEvent* event)
{
    PageWidget::resizeEvent(event);
    applyResponsiveLayout();
}

void PomodoroPage::applyResponsiveLayout()
{
    if (m_contentLayout == nullptr || m_statsCard == nullptr) {
        return;
    }
    const bool narrow = width() < 840;
    if (narrow == m_narrow) {
        return;
    }
    m_narrow = narrow;
    m_contentLayout->setDirection(narrow ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight);
    if (narrow) {
        m_statsCard->setMinimumWidth(0);
        m_statsCard->setMaximumWidth(QWIDGETSIZE_MAX);
    } else {
        m_statsCard->setFixedWidth(310);
    }
}

} // namespace pac::ui
