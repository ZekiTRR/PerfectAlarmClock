#include "TimersPage.h"

#include "TimerDialog.h"
#include "core/Catalog.h"
#include "core/Schedule.h"
#include "core/Fonts.h"
#include "core/Settings.h"
#include "core/Store.h"
#include "core/TimerPlan.h"
#include "core/Theme.h"

#include <QAbstractAnimation>
#include <QDateTime>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QStackedWidget>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>
#include <functional>

namespace pac::ui {
namespace {

class TimerProgress final : public QWidget {
public:
    explicit TimerProgress(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setFixedHeight(4);
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
        const QRectF track(0.0, 0.0, width(), height());
        painter.setPen(Qt::NoPen);
        painter.setBrush(tinted(colors.border, 0.7));
        painter.drawRoundedRect(track, 2.0, 2.0);
        if (m_progress > 0.0) {
            const QRectF fill(0.0, 0.0, std::max(4.0, width() * m_progress), height());
            painter.setBrush(m_color.isValid() ? m_color : colors.accent);
            painter.drawRoundedRect(fill, 2.0, 2.0);
        }
    }

private:
    qreal m_progress{0.0};
    QColor m_color;
};

} // namespace

class TimerCard final : public QFrame {
public:
    using Callback = std::function<void()>;

    TimerCard(const CountdownTimer& timer, QWidget* parent = nullptr)
        : QFrame(parent)
        , m_id(timer.id)
        , m_color(timer.color)
    {
        setObjectName(QStringLiteral("Card"));
        setFrameShape(QFrame::NoFrame);
        auto* outer = new QVBoxLayout(this);
        m_outer = outer;

        auto* row = new QHBoxLayout;
        row->setSpacing(10);

        auto* accentBar = new QFrame(this);
        accentBar->setFixedWidth(3);
        accentBar->setStyleSheet(QStringLiteral("background:%1; border:none; border-radius:1px;")
                                     .arg(timer.color));
        row->addWidget(accentBar);

        auto* icon = new QLabel(this);
        icon->setFixedSize(38, 38);
        icon->setAlignment(Qt::AlignCenter);
        icon->setPixmap(emojiPixmap(QStringLiteral("⏳"), 22));
        const QColor color(timer.color);
        icon->setStyleSheet(QStringLiteral("background: rgba(%1,%2,%3,36); border-radius: 8px;")
                                .arg(color.red())
                                .arg(color.green())
                                .arg(color.blue()));
        row->addWidget(icon);

        auto* textColumn = new QVBoxLayout;
        textColumn->setSpacing(0);
        m_title = new QLabel(this);
        QFont titleFont = m_title->font();
        titleFont.setPointSizeF(10.0);
        titleFont.setWeight(QFont::DemiBold);
        m_title->setFont(titleFont);
        m_title->setMaximumWidth(300);
        textColumn->addWidget(m_title);
        m_status = new QLabel(this);
        m_status->setObjectName(QStringLiteral("Muted"));
        textColumn->addWidget(m_status);
        row->addLayout(textColumn);
        row->addStretch(1);

        m_time = new QLabel(this);
        m_time->setObjectName(QStringLiteral("Mono"));
        m_time->setFont(fonts::digitFont(19.0, QFont::Light));
        m_time->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_time->setMinimumWidth(150);
        row->addWidget(m_time);

        auto* buttons = new QHBoxLayout;
        buttons->setSpacing(1);
        m_startPause = new ThemedToolButton(icons::Icon::Play, tr("Запустить"), this);
        m_reset = new ThemedToolButton(icons::Icon::Reset, tr("Сбросить"), this);
        m_more = new ThemedToolButton(icons::Icon::More, tr("Действия этапа"), this);
        m_more->setVisible(false);
        m_edit = new ThemedToolButton(icons::Icon::Edit, tr("Изменить"), this);
        m_delete = new ThemedToolButton(icons::Icon::Delete, tr("Удалить"), this);
        m_delete->setGlyphColor(ThemeManager::instance().current().danger);
        for (auto* button : {m_startPause, m_reset, m_more, m_edit, m_delete}) {
            button->setFixedSize(26, 26);
            button->setIconSizePx(15);
            buttons->addWidget(button);
        }
        row->addLayout(buttons);
        outer->addLayout(row);

        m_progress = new TimerProgress(this);
        outer->addWidget(m_progress);

        connect(m_startPause, &QToolButton::clicked, this, [this] { if (m_onStartPause) m_onStartPause(); });
        connect(m_reset, &QToolButton::clicked, this, [this] { if (m_onReset) m_onReset(); });
        connect(m_edit, &QToolButton::clicked, this, [this] { if (m_onEdit) m_onEdit(); });
        connect(m_delete, &QToolButton::clicked, this, [this] { if (m_onDelete) m_onDelete(); });
        connect(&Settings::instance(), &Settings::changed, this, [this] { applyDensity(); });
        applyDensity();
        connect(m_more, &QToolButton::clicked, this, [this] {
            QMenu menu(this);
            auto* previous = menu.addAction(tr("Предыдущий этап"));
            auto* skip = menu.addAction(tr("Пропустить этап"));
            auto* addMinute = menu.addAction(tr("Добавить минуту"));
            QAction* chosen = menu.exec(m_more->mapToGlobal(QPoint(0, m_more->height())));
            if (chosen == previous && m_onPrevious) {
                m_onPrevious();
            } else if (chosen == skip && m_onSkip) {
                m_onSkip();
            } else if (chosen == addMinute && m_onAddMinute) {
                m_onAddMinute();
            }
        });
    }

    QUuid timerId() const
    {
        return m_id;
    }

    void setCallbacks(Callback startPause, Callback reset, Callback edit, Callback remove)
    {
        m_onStartPause = std::move(startPause);
        m_onReset = std::move(reset);
        m_onEdit = std::move(edit);
        m_onDelete = std::move(remove);
    }

    void setSequenceCallbacks(Callback previous, Callback skip, Callback addMinute)
    {
        m_onPrevious = std::move(previous);
        m_onSkip = std::move(skip);
        m_onAddMinute = std::move(addMinute);
    }

    void applyDensity()
    {
        if (m_outer == nullptr) {
            return;
        }
        const int density = Settings::instance().rowDensity();
        m_outer->setContentsMargins(density == 0 ? 10 : (density == 2 ? 18 : 14),
                                    density == 0 ? 7 : (density == 2 ? 13 : 10),
                                    density == 0 ? 10 : (density == 2 ? 18 : 14),
                                    density == 0 ? 7 : (density == 2 ? 13 : 10));
        m_outer->setSpacing(density == 0 ? 4 : (density == 2 ? 10 : 7));
        if (m_title != nullptr) {
            QFont titleFont = m_title->font();
            titleFont.setPointSizeF(density == 0 ? 9.5 : (density == 2 ? 10.5 : 10.0));
            m_title->setFont(titleFont);
        }
        if (m_time != nullptr) {
            m_time->setFont(fonts::digitFont(density == 0 ? 17.0 : (density == 2 ? 22.0 : 19.0),
                                             QFont::Light));
            m_time->setMinimumWidth(density == 0 ? 130 : (density == 2 ? 175 : 150));
        }
        updateGeometry();
        update();
    }

    void updateDisplay(const CountdownTimer& timer, const QDateTime& now)
    {
        m_color = timer.color;
        const int currentRemaining = timerplan::currentRemainingMs(timer, now);
        const int totalRemaining = timerplan::remainingTotalMs(timer, now);
        const int total = std::max(1, timerplan::totalSeconds(timer));
        int shown = currentRemaining;
        if (timer.mode == TimerMode::CountUp) {
            shown = std::max(0, total * 1000 - totalRemaining);
        }
        m_time->setText(schedule::formatTimer(shown));
        m_title->setText(timer.label.isEmpty() ? tr("Таймер") : timer.label);

        QString state;
        const TimerStage* stage = timerplan::currentStage(timer);
        const bool sequence = timerplan::hasSequence(timer);
        switch (timer.state) {
        case TimerState::Running:
            state = tr("Идёт отсчёт");
            if (!sequence && timer.endAt.isValid()) {
                state += tr(" · до %1").arg(
                    schedule::timeText(timer.endAt.time(), Settings::instance().use24h()));
            }
            break;
        case TimerState::Paused:
            state = tr("Пауза · осталось %1").arg(schedule::formatTimer(totalRemaining));
            break;
        case TimerState::Finished:
            state = tr("Завершён");
            break;
        case TimerState::Stopped:
            state = tr("Готов к запуску");
            break;
        }
        if (sequence) {
            state = timerplan::modeName(timer.mode) + QStringLiteral(" · ")
                    + (stage != nullptr && !stage->name.isEmpty() ? stage->name
                                                                  : tr("Этап %1")
                                                                        .arg(timer.currentStage + 1));
            state += tr(" · %1/%2").arg(timer.currentStage + 1)
                         .arg(static_cast<int>(timer.stages.size()));
            state += tr(" · круг %1/%2")
                         .arg(std::min(timer.completedCycles + 1, timer.repeatCycles))
                         .arg(timer.repeatCycles);
            if (timer.state == TimerState::Running) {
                state += tr(" · всего %1").arg(schedule::formatTimer(totalRemaining));
            }
        } else if (timer.mode == TimerMode::CountUp) {
            state = tr("Прошло %1 из %2")
                        .arg(schedule::formatTimer(shown),
                             schedule::formatTimer(static_cast<qint64>(total) * 1000));
        } else if ((timer.mode == TimerMode::Deadline || timer.mode == TimerMode::Target)
                   && timer.targetAt.isValid()) {
            state += tr(" · срок %1")
                         .arg(schedule::dateTimeText(timer.targetAt, now,
                                                     Settings::instance().use24h()));
        } else if (timer.mode == TimerMode::RandomInterval
                   && timer.state != TimerState::Stopped) {
            state += tr(" · интервал %1").arg(
                schedule::formatTimer(static_cast<qint64>(timer.durationSec) * 1000));
        }
        if (timer.autoRepeat && !sequence) {
            state += QStringLiteral(" · ") + tr("повтор");
        }
        m_status->setText(state);
        const QString tooltip = QStringLiteral("%1\n%2\n%3")
                                    .arg(m_title->text(), state,
                                         tr("Длительность: %1").arg(
                                             schedule::formatTimer(total * 1000)));
        setToolTip(tooltip);
        const qreal progress = 1.0 - static_cast<qreal>(totalRemaining)
                                          / (static_cast<qreal>(total) * 1000.0);
        m_progress->setProgress(progress, timer.state == TimerState::Finished
                                               ? ThemeManager::instance().current().success
                                               : QColor(timer.color));
        m_startPause->setGlyph(timer.state == TimerState::Running ? icons::Icon::Pause
                                                                   : icons::Icon::Play);
        m_startPause->setToolTip(timer.state == TimerState::Running ? tr("Пауза") : tr("Запустить"));
        m_reset->setEnabled(timer.state != TimerState::Stopped
                            || totalRemaining < total * 1000);
        m_more->setVisible(sequence && timer.state != TimerState::Finished
                          && timer.state != TimerState::Stopped);
    }

private:
    QUuid m_id;
    QString m_color;
    QVBoxLayout* m_outer{nullptr};
    QLabel* m_title{nullptr};
    QLabel* m_status{nullptr};
    QLabel* m_time{nullptr};
    TimerProgress* m_progress{nullptr};
    ThemedToolButton* m_startPause{nullptr};
    ThemedToolButton* m_reset{nullptr};
    ThemedToolButton* m_more{nullptr};
    ThemedToolButton* m_edit{nullptr};
    ThemedToolButton* m_delete{nullptr};
    Callback m_onStartPause;
    Callback m_onReset;
    Callback m_onEdit;
    Callback m_onDelete;
    Callback m_onPrevious;
    Callback m_onSkip;
    Callback m_onAddMinute;
};

TimersPage::TimersPage(Store& store, QWidget* parent)
    : PageWidget(parent)
    , m_store(store)
{
    buildUi();
    connect(&m_store, &Store::timersChanged, this, [this] {
        QTimer::singleShot(0, this, [this] {
            rebuildCards();
            updateEmptyState();
        });
    });
    const auto applyListDensity = [this] {
        if (m_cardsLayout != nullptr) {
            const int density = Settings::instance().rowDensity();
            m_cardsLayout->setSpacing(density == 0 ? 4 : (density == 2 ? 12 : 8));
        }
    };
    connect(&Settings::instance(), &Settings::changed, this, applyListDensity);
    applyListDensity();
    rebuildCards();
    updateEmptyState();
}

void TimersPage::buildUi()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(10);

    m_pageHeader = new QWidget(this);
    m_pageHeader->setObjectName(QStringLiteral("Transparent"));
    auto* headerLayout = new QHBoxLayout(m_pageHeader);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    m_pageTitle = new QLabel(tr("Таймеры"), m_pageHeader);
    m_pageTitle->setObjectName(QStringLiteral("PageTitle"));
    headerLayout->addWidget(m_pageTitle);
    headerLayout->addStretch(1);
    m_pageHeader->hide();
    layout->addWidget(m_pageHeader);

    auto* toolbar = new QWidget(this);
    m_toolbarLayout = new QHBoxLayout(toolbar);
    m_toolbarLayout->setContentsMargins(0, 0, 0, 0);
    m_toolbarLayout->setSpacing(6);
    m_quickLabel = new QLabel(tr("Быстрый запуск"), toolbar);
    m_quickLabel->setObjectName(QStringLiteral("Muted"));
    m_toolbarLayout->addWidget(m_quickLabel);
    for (const int minutes : {5, 10, 15, 25, 45, 60}) {
        auto* button = new QPushButton(QStringLiteral("%1 мин").arg(minutes), toolbar);
        button->setFixedHeight(27);
        button->setCursor(Qt::PointingHandCursor);
        button->setProperty("quickMinutes", minutes);
        connect(button, &QPushButton::clicked, this, [this, minutes] { quickStart(minutes); });
        m_toolbarLayout->addWidget(button);
        m_quickButtons.append(button);
    }
    m_toolbarLayout->addStretch(1);
    auto* bulkButton = new ThemedToolButton(icons::Icon::More, tr("Все таймеры"), toolbar);
    bulkButton->setFixedSize(28, 28);
    bulkButton->setIconSizePx(16);
    connect(bulkButton, &QToolButton::clicked, this, [this, bulkButton] {
        QMenu menu(this);
        auto* startAll = menu.addAction(icons::qicon(icons::Icon::Play,
                                                     ThemeManager::instance().current().accent),
                                        tr("Запустить все"));
        auto* pauseAll = menu.addAction(icons::qicon(icons::Icon::Pause,
                                                     ThemeManager::instance().current().accent),
                                        tr("Пауза для всех"));
        auto* resetAll = menu.addAction(icons::qicon(icons::Icon::Reset,
                                                     ThemeManager::instance().current().accent),
                                        tr("Сбросить все"));
        auto* stopAll = menu.addAction(icons::qicon(icons::Icon::Stop,
                                                    ThemeManager::instance().current().danger),
                                       tr("Остановить все"));
        QAction* chosen = menu.exec(bulkButton->mapToGlobal(QPoint(0, bulkButton->height())));
        if (chosen == nullptr) {
            return;
        }
        const QDateTime now = QDateTime::currentDateTime();
        if (chosen == startAll) {
            for (auto& timer : m_store.data().timers) {
                if (timer.state == TimerState::Running) {
                    continue;
                }
                if (timer.state == TimerState::Paused) {
                    timer.endAt = now.addMSecs(std::max<qint64>(1, timer.remainingMs));
                    timer.state = TimerState::Running;
                } else {
                    timerplan::prepareStart(timer, now);
                }
            }
        } else if (chosen == pauseAll) {
            for (auto& timer : m_store.data().timers) {
                if (timer.state != TimerState::Running) {
                    continue;
                }
                timer.remainingMs = timerplan::currentRemainingMs(timer, now);
                timer.endAt = QDateTime();
                timer.state = TimerState::Paused;
            }
        } else if (chosen == resetAll || chosen == stopAll) {
            for (auto& timer : m_store.data().timers) {
                timerplan::reset(timer);
            }
        }
        m_store.commit(Domain::Timers);
    });
    m_toolbarLayout->addWidget(bulkButton);
    auto* addButton = new ThemedButton(tr("Новый таймер"), icons::Icon::Timer, toolbar, true);
    addButton->setMinimumHeight(28);
    connect(addButton, &QPushButton::clicked, this, &TimersPage::createTimer);
    m_toolbarLayout->addWidget(addButton);
    layout->addWidget(toolbar);

    m_scroll = new QScrollArea(this);
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto* content = new QWidget(m_scroll);
    content->setObjectName(QStringLiteral("Transparent"));
    m_cardsLayout = new QVBoxLayout(content);
    m_cardsLayout->setContentsMargins(0, 0, 6, 0);
    m_cardsLayout->setSpacing(8);
    m_cardsLayout->addStretch(1);
    m_scroll->setWidget(content);

    m_empty = new QWidget(this);
    auto* emptyLayout = new QVBoxLayout(m_empty);
    emptyLayout->setContentsMargins(24, 24, 24, 24);
    emptyLayout->addStretch(1);
    auto* card = makeCard(m_empty);
    card->setMaximumWidth(500);
    auto* inner = new QVBoxLayout(card);
    inner->setContentsMargins(30, 28, 30, 28);
    inner->setSpacing(8);
    auto* icon = new QLabel(card);
    icon->setAlignment(Qt::AlignHCenter);
    icon->setPixmap(icons::pixmap(icons::Icon::Timer, 44,
                                  ThemeManager::instance().current().accent));
    inner->addWidget(icon);
    m_emptyTitle = makeLabel(tr("Таймеров пока нет"), QStringLiteral("PageTitle"), card);
    m_emptyTitle->setAlignment(Qt::AlignHCenter);
    inner->addWidget(m_emptyTitle);
    m_emptyHint = makeLabel(tr("Запустите быстрый отсчёт кнопкой выше или создайте "
                              "таймер с собственным звуком и повтором."),
                            QStringLiteral("Muted"), card);
    m_emptyHint->setAlignment(Qt::AlignHCenter);
    m_emptyHint->setWordWrap(true);
    inner->addWidget(m_emptyHint);
    inner->addSpacing(4);
    auto* createButton = new ThemedButton(tr("Создать таймер"), icons::Icon::Timer, card, true);
    connect(createButton, &QPushButton::clicked, this, &TimersPage::createTimer);
    inner->addWidget(createButton, 0, Qt::AlignHCenter);
    emptyLayout->addWidget(card, 0, Qt::AlignHCenter);
    emptyLayout->addStretch(1);

    m_stack = new QStackedWidget(this);
    m_stack->setObjectName(QStringLiteral("Transparent"));
    m_stack->addWidget(m_scroll);
    m_stack->addWidget(m_empty);
    layout->addWidget(m_stack, 1);
}

void TimersPage::rebuildCards()
{
    for (auto* card : m_cards) {
        if (card != nullptr) {
            m_cardsLayout->removeWidget(card);
            delete card;
        }
    }
    m_cards.clear();

    if (m_cardsLayout->count() == 0) {
        m_cardsLayout->addStretch(1);
    }
    QLayoutItem* stretch = m_cardsLayout->itemAt(m_cardsLayout->count() - 1);
    if (stretch != nullptr && stretch->spacerItem() == nullptr) {
        m_cardsLayout->addStretch(1);
    }

    const QDateTime now = QDateTime::currentDateTime();
    for (const auto& timer : m_store.data().timers) {
        auto* card = new TimerCard(timer, m_cardsLayout->parentWidget());
        const QUuid id = timer.id;
        card->setCallbacks([this, id] { startPause(id); }, [this, id] { resetTimer(id); },
                           [this, id] { editTimer(id); }, [this, id] { removeTimer(id); });
        card->setSequenceCallbacks([this, id] { previousStage(id); },
                                   [this, id] { skipStage(id); },
                                   [this, id] { addMinute(id); });
        card->updateDisplay(timer, now);
        m_cardsLayout->insertWidget(m_cardsLayout->count() - 1, card);
        m_cards.append(card);
    }
}

void TimersPage::updateEmptyState()
{
    m_stack->setCurrentIndex(m_store.data().timers.empty() ? 1 : 0);
}

void TimersPage::refreshCards(const QDateTime& now)
{
    for (auto* card : m_cards) {
        const CountdownTimer* timer = m_store.findTimer(card->timerId());
        if (timer != nullptr) {
            card->updateDisplay(*timer, now);
        }
    }
}

void TimersPage::refreshClock(const QDateTime& now)
{
    refreshCards(now);
}

void TimersPage::onActivated()
{
    refreshClock(QDateTime::currentDateTime());
}

void TimersPage::themeUpdated()
{
    rebuildCards();
    updateEmptyState();
}

void TimersPage::applyThemeMode()
{
    m_fluent = ThemeManager::instance().current().flavor == ThemeFlavor::WindowsClock;
    if (m_pageHeader != nullptr) {
        m_pageHeader->setVisible(m_fluent);
    }
}

void TimersPage::resizeEvent(QResizeEvent* event)
{
    PageWidget::resizeEvent(event);
    const bool narrow = width() < 760;
    if (m_quickLabel != nullptr) {
        m_quickLabel->setVisible(!narrow);
    }
    for (auto* button : m_quickButtons) {
        if (button != nullptr) {
            button->setVisible(!narrow || button->property("quickMinutes").toInt() <= 30);
        }
    }
    if (m_toolbarLayout != nullptr) {
        m_toolbarLayout->setSpacing(narrow ? 4 : 6);
    }
}

void TimersPage::createTimer()
{
    CountdownTimer timer;
    timer.durationSec = Settings::instance().defaultTimerMinutes() * 60;
    timer.remainingMs = static_cast<qint64>(timer.durationSec) * 1000;
    timer.soundId = Settings::instance().defaultTimerSound();
    timer.volume = Settings::instance().defaultTimerVolume();
    timer.loopSound = Settings::instance().defaultTimerLoop();
    timer.popupEnabled = Settings::instance().defaultTimerPopup();
    timer.color = catalog::alarmColors().at(m_store.data().timers.size()
                                            % catalog::alarmColors().size());

    TimerDialog dialog(timer, true, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    CountdownTimer created = dialog.result();
    created.id = QUuid::createUuid();
    if (dialog.startImmediately()) {
        timerplan::prepareStart(created, QDateTime::currentDateTime());
    }
    m_store.data().timers.push_back(created);
    m_store.commit(Domain::Timers);
}

void TimersPage::quickStart(int minutes)
{
    CountdownTimer timer;
    timer.label = tr("Таймер на %1 мин").arg(minutes);
    timer.durationSec = minutes * 60;
    timer.remainingMs = static_cast<qint64>(timer.durationSec) * 1000;
    timer.state = TimerState::Running;
    timer.endAt = QDateTime::currentDateTime().addSecs(timer.durationSec);
    timer.soundId = Settings::instance().defaultTimerSound();
    timer.volume = Settings::instance().defaultTimerVolume();
    timer.loopSound = Settings::instance().defaultTimerLoop();
    timer.popupEnabled = Settings::instance().defaultTimerPopup();
    timer.color = catalog::alarmColors().at(m_store.data().timers.size()
                                            % catalog::alarmColors().size());
    m_store.data().timers.push_back(timer);
    m_store.commit(Domain::Timers);
}

void TimersPage::startPause(const QUuid& id)
{
    CountdownTimer* timer = m_store.findTimer(id);
    if (timer == nullptr) {
        return;
    }
    const QDateTime now = QDateTime::currentDateTime();
    if (timer->state == TimerState::Running) {
        timer->remainingMs = timerplan::currentRemainingMs(*timer, now);
        timer->endAt = QDateTime();
        timer->state = TimerState::Paused;
    } else if (timer->state == TimerState::Paused) {
        timer->endAt = now.addMSecs(std::max<qint64>(1, timer->remainingMs));
        timer->state = TimerState::Running;
    } else {
        timerplan::prepareStart(*timer, now);
    }
    m_store.commit(Domain::Timers);
}

void TimersPage::resetTimer(const QUuid& id)
{
    CountdownTimer* timer = m_store.findTimer(id);
    if (timer == nullptr) {
        return;
    }
    timerplan::reset(*timer);
    m_store.commit(Domain::Timers);
}

void TimersPage::editTimer(const QUuid& id)
{
    CountdownTimer* timer = m_store.findTimer(id);
    if (timer == nullptr) {
        return;
    }
    TimerDialog dialog(*timer, false, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    CountdownTimer updated = dialog.result();
    updated.id = id;
    if (dialog.startImmediately()) {
        timerplan::prepareStart(updated, QDateTime::currentDateTime());
    } else {
        updated.state = TimerState::Stopped;
        updated.endAt = QDateTime();
        timerplan::reset(updated);
    }
    CountdownTimer* target = m_store.findTimer(id);
    if (target != nullptr) {
        *target = updated;
        m_store.commit(Domain::Timers);
    }
}

void TimersPage::skipStage(const QUuid& id)
{
    CountdownTimer* timer = m_store.findTimer(id);
    if (timer == nullptr || timer->stages.empty()
        || timer->state == TimerState::Finished || timer->state == TimerState::Stopped) {
        return;
    }
    timerplan::skipStage(*timer, QDateTime::currentDateTime());
    m_store.commit(Domain::Timers);
}

void TimersPage::previousStage(const QUuid& id)
{
    CountdownTimer* timer = m_store.findTimer(id);
    if (timer == nullptr || timer->stages.empty()
        || timer->state == TimerState::Finished || timer->state == TimerState::Stopped) {
        return;
    }
    timerplan::previousStage(*timer, QDateTime::currentDateTime());
    m_store.commit(Domain::Timers);
}

void TimersPage::addMinute(const QUuid& id)
{
    CountdownTimer* timer = m_store.findTimer(id);
    if (timer == nullptr) {
        return;
    }
    timerplan::addTime(*timer, QDateTime::currentDateTime(), 60);
    m_store.commit(Domain::Timers);
}

void TimersPage::removeTimer(const QUuid& id)
{
    CountdownTimer* timer = m_store.findTimer(id);
    if (timer == nullptr) {
        return;
    }
    const QString label = timer->label.isEmpty() ? tr("Таймер") : timer->label;
    QMessageBox box(QMessageBox::Question, tr("Удалить таймер"),
                    tr("Удалить таймер «%1»?").arg(label),
                    QMessageBox::Yes | QMessageBox::No, this);
    box.setDefaultButton(QMessageBox::No);
    if (box.exec() != QMessageBox::Yes) {
        return;
    }
    const auto it = std::find_if(m_store.data().timers.begin(), m_store.data().timers.end(),
                                 [&id](const CountdownTimer& entry) { return entry.id == id; });
    if (it == m_store.data().timers.end()) {
        return;
    }
    m_store.data().timers.erase(it);
    m_store.commit(Domain::Timers);
}

} // namespace pac::ui
