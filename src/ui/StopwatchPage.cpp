#include "StopwatchPage.h"

#include "core/Fonts.h"
#include "core/Schedule.h"
#include "core/Theme.h"

#include <QFont>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QShortcut>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>

namespace pac::ui {
namespace {

[[nodiscard]] QString stateText(bool running, qint64 elapsed)
{
    if (running) {
        return StopwatchPage::tr("Идёт отсчёт");
    }
    return elapsed > 0 ? StopwatchPage::tr("Пауза") : StopwatchPage::tr("Готов");
}

} // namespace

StopwatchPage::StopwatchPage(QWidget* parent)
    : PageWidget(parent)
{
    buildUi();
    updateDisplay();
    updateStartButton();
    updateStatePill();
}

void StopwatchPage::buildUi()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(10);

    auto* hero = makeCard(this);
    auto* heroLayout = new QVBoxLayout(hero);
    heroLayout->setContentsMargins(24, 16, 24, 16);
    heroLayout->setSpacing(6);

    auto* titleRow = new QHBoxLayout;
    titleRow->setSpacing(9);
    auto* icon = new QLabel(hero);
    icon->setPixmap(icons::pixmap(icons::Icon::Stopwatch, 24,
                                  ThemeManager::instance().current().accent));
    titleRow->addWidget(icon);
    auto* title = new QLabel(tr("Секундомер"), hero);
    QFont titleFont = title->font();
    titleFont.setPointSizeF(12.0);
    titleFont.setWeight(QFont::DemiBold);
    title->setFont(titleFont);
    titleRow->addWidget(title);
    titleRow->addStretch(1);
    m_statePill = new QLabel(hero);
    m_statePill->setAlignment(Qt::AlignCenter);
    m_statePill->setFixedHeight(22);
    m_statePill->setMinimumWidth(92);
    titleRow->addWidget(m_statePill);
    heroLayout->addLayout(titleRow);

    m_timeLabel = new QLabel(QStringLiteral("00:00:00.00"), hero);
    m_timeLabel->setFont(fonts::digitFont(46.0, QFont::Light));
    m_timeLabel->setAlignment(Qt::AlignCenter);
    m_timeLabel->setMinimumHeight(102);
    heroLayout->addWidget(m_timeLabel);

    auto* buttonRow = new QHBoxLayout;
    buttonRow->setSpacing(10);
    buttonRow->addStretch(1);
    m_startButton = new ThemedButton(tr("Старт"), icons::Icon::Play, hero, true);
    m_startButton->setMinimumSize(124, 34);
    m_startButton->setIconSizePx(16);
    m_lapButton = new ThemedButton(tr("Круг"), icons::Icon::Flag, hero);
    m_lapButton->setMinimumSize(104, 34);
    m_lapButton->setIconSizePx(16);
    m_resetButton = new ThemedButton(tr("Сброс"), icons::Icon::Reset, hero);
    m_resetButton->setMinimumSize(104, 34);
    m_resetButton->setIconSizePx(16);
    buttonRow->addWidget(m_startButton);
    buttonRow->addWidget(m_lapButton);
    buttonRow->addWidget(m_resetButton);
    buttonRow->addStretch(1);
    heroLayout->addLayout(buttonRow);

    auto* hint = new QLabel(tr("Пробел — старт/пауза · L — круг · R — сброс"), hero);
    hint->setObjectName(QStringLiteral("Faint"));
    hint->setAlignment(Qt::AlignCenter);
    heroLayout->addWidget(hint);
    layout->addWidget(hero);

    auto* lapsCard = makeCard(this);
    auto* lapsLayout = new QVBoxLayout(lapsCard);
    lapsLayout->setContentsMargins(16, 12, 16, 12);
    lapsLayout->setSpacing(6);
    auto* lapsHeader = new QHBoxLayout;
    auto* lapsTitle = new QLabel(tr("Круги"), lapsCard);
    QFont lapsFont = lapsTitle->font();
    lapsFont.setWeight(QFont::DemiBold);
    lapsTitle->setFont(lapsFont);
    lapsHeader->addWidget(lapsTitle);
    m_lapHeader = new QLabel(lapsCard);
    m_lapHeader->setObjectName(QStringLiteral("Muted"));
    lapsHeader->addWidget(m_lapHeader);
    lapsHeader->addStretch(1);
    lapsLayout->addLayout(lapsHeader);

    m_lapsTable = new QTableWidget(0, 3, lapsCard);
    m_lapsTable->setHorizontalHeaderLabels({tr("КРУГ"), tr("ВРЕМЯ КРУГА"), tr("ОБЩЕЕ ВРЕМЯ")});
    m_lapsTable->verticalHeader()->setVisible(false);
    m_lapsTable->setShowGrid(false);
    m_lapsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_lapsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_lapsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_lapsTable->setAlternatingRowColors(true);
    m_lapsTable->setFocusPolicy(Qt::NoFocus);
    m_lapsTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_lapsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    m_lapsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_lapsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_lapsTable->setColumnWidth(0, 86);
    m_lapsTable->setFrameShape(QFrame::NoFrame);

    auto* empty = new QWidget(lapsCard);
    auto* emptyLayout = new QVBoxLayout(empty);
    emptyLayout->setContentsMargins(0, 22, 0, 22);
    auto* emptyLabel = new QLabel(tr("Круги появятся после первого нажатия «Круг»"), empty);
    emptyLabel->setObjectName(QStringLiteral("Muted"));
    emptyLabel->setAlignment(Qt::AlignCenter);
    emptyLayout->addWidget(emptyLabel);
    emptyLayout->addStretch(1);

    m_lapsStack = new QStackedWidget(lapsCard);
    m_lapsStack->setObjectName(QStringLiteral("Transparent"));
    m_lapsStack->addWidget(m_lapsTable);
    m_lapsStack->addWidget(empty);
    m_lapsStack->setCurrentIndex(1);
    lapsLayout->addWidget(m_lapsStack, 1);
    layout->addWidget(lapsCard, 1);

    connect(m_startButton, &QPushButton::clicked, this, &StopwatchPage::toggleRunning);
    connect(m_lapButton, &QPushButton::clicked, this, &StopwatchPage::recordLap);
    connect(m_resetButton, &QPushButton::clicked, this, &StopwatchPage::resetStopwatch);

    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(33);
    m_refreshTimer->setTimerType(Qt::PreciseTimer);
    connect(m_refreshTimer, &QTimer::timeout, this, &StopwatchPage::updateDisplay);

    auto* lapShortcut = new QShortcut(QKeySequence(Qt::Key_L), this);
    lapShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(lapShortcut, &QShortcut::activated, this, &StopwatchPage::recordLap);
    auto* resetShortcut = new QShortcut(QKeySequence(Qt::Key_R), this);
    resetShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(resetShortcut, &QShortcut::activated, this, &StopwatchPage::resetStopwatch);
}

qint64 StopwatchPage::elapsedMs() const
{
    return m_accumulatedMs + (m_running ? m_elapsed.elapsed() : 0);
}

void StopwatchPage::toggleRunning()
{
    if (m_running) {
        m_accumulatedMs += m_elapsed.elapsed();
        m_running = false;
        m_refreshTimer->stop();
    } else {
        m_running = true;
        m_elapsed.start();
        if (isActive()) {
            m_refreshTimer->start();
        }
    }
    updateDisplay();
    updateStartButton();
    updateStatePill();
}

void StopwatchPage::recordLap()
{
    if (!m_running) {
        return;
    }
    const qint64 total = elapsedMs();
    const qint64 lap = total - m_lastLapTotal;
    m_lastLapTotal = total;
    m_lapTimes.append(lap);
    m_lapTotals.append(total);
    updateDisplay();
    updateLapsTable();
}

void StopwatchPage::resetStopwatch()
{
    m_running = false;
    m_refreshTimer->stop();
    m_accumulatedMs = 0;
    m_elapsed.invalidate();
    m_lastLapTotal = 0;
    m_lapTimes.clear();
    m_lapTotals.clear();
    updateDisplay();
    updateStartButton();
    updateStatePill();
    updateLapsTable();
}

void StopwatchPage::updateDisplay()
{
    if (m_timeLabel != nullptr) {
        m_timeLabel->setText(schedule::formatStopwatch(elapsedMs()));
    }
    if (m_lapButton != nullptr) {
        m_lapButton->setEnabled(m_running);
    }
    if (m_resetButton != nullptr) {
        m_resetButton->setEnabled(m_running || elapsedMs() > 0);
    }
}

void StopwatchPage::updateStartButton()
{
    if (m_startButton == nullptr) {
        return;
    }
    m_startButton->setText(m_running ? tr("Пауза") : tr("Старт"));
    m_startButton->setGlyph(m_running ? icons::Icon::Pause : icons::Icon::Play);
}

void StopwatchPage::updateStatePill()
{
    if (m_statePill == nullptr) {
        return;
    }
    const ThemeColors& colors = ThemeManager::instance().current();
    const qint64 elapsed = elapsedMs();
    m_statePill->setText(stateText(m_running, elapsed));
    QColor color = colors.dim;
    if (m_running) {
        color = colors.success;
    } else if (elapsed > 0) {
        color = colors.warning;
    }
    m_statePill->setStyleSheet(
        QStringLiteral("background: rgba(%1,%2,%3,30); border: 1px solid rgba(%1,%2,%3,90);"
                       " border-radius: 10px; padding: 1px 10px; color: %4;")
            .arg(color.red())
            .arg(color.green())
            .arg(color.blue())
            .arg(color.name()));
}

void StopwatchPage::updateLapsTable()
{
    if (m_lapsTable == nullptr) {
        return;
    }
    m_lapsStack->setCurrentIndex(m_lapTimes.isEmpty() ? 1 : 0);
    m_lapHeader->setText(m_lapTimes.isEmpty() ? tr("нет кругов")
                                              : tr("всего: %1").arg(m_lapTimes.size()));
    m_lapsTable->clearContents();
    m_lapsTable->setRowCount(m_lapTimes.size());
    if (m_lapTimes.isEmpty()) {
        return;
    }
    const qint64 best = *std::min_element(m_lapTimes.cbegin(), m_lapTimes.cend());
    const qint64 worst = *std::max_element(m_lapTimes.cbegin(), m_lapTimes.cend());
    const ThemeColors& colors = ThemeManager::instance().current();
    const QFont mono = fonts::digitFont(9.0);
    for (int row = 0; row < m_lapTimes.size(); ++row) {
        auto* number = new QTableWidgetItem(QString::number(row + 1));
        number->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        number->setForeground(colors.dim);
        auto* lapItem = new QTableWidgetItem(schedule::formatStopwatch(m_lapTimes.at(row)));
        lapItem->setFont(mono);
        lapItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        auto* totalItem = new QTableWidgetItem(schedule::formatStopwatch(m_lapTotals.at(row)));
        totalItem->setFont(mono);
        totalItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        totalItem->setForeground(colors.dim);
        if (m_lapTimes.size() > 1 && m_lapTimes.at(row) == best) {
            lapItem->setForeground(colors.success);
        } else if (m_lapTimes.size() > 1 && m_lapTimes.at(row) == worst) {
            lapItem->setForeground(colors.danger);
        } else {
            lapItem->setForeground(colors.text);
        }
        m_lapsTable->setItem(row, 0, number);
        m_lapsTable->setItem(row, 1, lapItem);
        m_lapsTable->setItem(row, 2, totalItem);
        m_lapsTable->setRowHeight(row, 28);
    }
    m_lapsTable->scrollToBottom();
}

void StopwatchPage::updateLapsPalette()
{
    updateLapsTable();
}

void StopwatchPage::refreshClock(const QDateTime& now)
{
    Q_UNUSED(now)
    if (m_running) {
        updateDisplay();
    }
}

void StopwatchPage::primaryAction()
{
    if (isActive()) {
        toggleRunning();
    }
}

void StopwatchPage::onActivated()
{
    updateDisplay();
    if (m_running) {
        m_refreshTimer->start();
    }
}

void StopwatchPage::onDeactivated()
{
    if (m_refreshTimer != nullptr) {
        m_refreshTimer->stop();
    }
}

void StopwatchPage::themeUpdated()
{
    if (m_statePill != nullptr) {
        updateStatePill();
    }
    updateLapsPalette();
}

} // namespace pac::ui
