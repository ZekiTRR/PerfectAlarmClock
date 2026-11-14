#include "AlarmsPage.h"

#include "AlarmDelegate.h"
#include "AlarmDialog.h"
#include "AlarmTableModel.h"
#include "NonWorkingDaysDialog.h"
#include "core/Catalog.h"
#include "core/Schedule.h"
#include "core/Settings.h"
#include "core/Store.h"
#include "core/Theme.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QDate>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLineEdit>
#include <QLabel>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QFontMetrics>
#include <QTableView>
#include <QVBoxLayout>

#include <algorithm>
#include <array>

namespace pac::ui {
namespace {

[[nodiscard]] QColor nextAlarmColor(const Store& store)
{
    const auto& palette = catalog::alarmColors();
    return QColor(palette.at(store.data().alarms.size() % palette.size()));
}

} // namespace

AlarmsPage::AlarmsPage(Store& store, QWidget* parent)
    : PageWidget(parent)
    , m_store(store)
{
    buildUi();
}

void AlarmsPage::buildUi()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 10, 16, 12);
    layout->setSpacing(8);

    m_toolbar = new QWidget(this);
    m_toolbar->setFixedHeight(32);
    auto* toolbarLayout = new QHBoxLayout(m_toolbar);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarLayout->setSpacing(8);

    m_groupCombo = new QComboBox(m_toolbar);
    m_groupCombo->setMinimumWidth(120);
    m_groupCombo->setMaximumWidth(280);
    m_groupCombo->setIconSize(QSize(14, 14));
    m_groupCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_groupCombo->setToolTip(tr("Фильтр по группе"));
    toolbarLayout->addWidget(m_groupCombo);

    m_sortCombo = new QComboBox(m_toolbar);
    m_sortCombo->setMinimumWidth(120);
    m_sortCombo->setMaximumWidth(230);
    m_sortCombo->setToolTip(tr("Сортировка внутри групп"));
    m_sortCombo->addItem(tr("По ближайшему сигналу"),
                         static_cast<int>(AlarmTableModel::SortMode::NextRun));
    m_sortCombo->addItem(tr("По времени дня"),
                         static_cast<int>(AlarmTableModel::SortMode::Time));
    m_sortCombo->addItem(tr("По названию"),
                         static_cast<int>(AlarmTableModel::SortMode::Name));
    m_sortCombo->addItem(tr("Сначала включённые"),
                         static_cast<int>(AlarmTableModel::SortMode::Enabled));
    toolbarLayout->addWidget(m_sortCombo);

    auto* holidaysButton = new ThemedToolButton(icons::Icon::Calendar,
                                                 tr("Праздники и выходные"), m_toolbar);
    holidaysButton->setFixedSize(26, 26);
    holidaysButton->setIconSizePx(15);
    toolbarLayout->addWidget(holidaysButton);
    toolbarLayout->addStretch(1);

    m_countLabel = new QLabel(m_toolbar);
    m_countLabel->setObjectName(QStringLiteral("Muted"));
    toolbarLayout->addWidget(m_countLabel);

    m_resetFilterButton = new ThemedToolButton(icons::Icon::Close, tr("Сбросить фильтр"), m_toolbar);
    m_resetFilterButton->setFixedSize(26, 26);
    m_resetFilterButton->setIconSizePx(15);
    m_resetFilterButton->setVisible(false);
    toolbarLayout->addWidget(m_resetFilterButton);

    m_pauseButton = new ThemedToolButton(icons::Icon::Pause, tr("Приостановить все"), m_toolbar);
    m_pauseButton->setFixedSize(26, 26);
    m_pauseButton->setIconSizePx(14);
    m_pauseButton->setCursor(Qt::PointingHandCursor);
    toolbarLayout->addWidget(m_pauseButton);

    m_enableAllButton = new ThemedToolButton(icons::Icon::Check, tr("Включить все будильники"), m_toolbar);
    m_enableAllButton->setFixedSize(26, 26);
    m_enableAllButton->setIconSizePx(14);
    toolbarLayout->addWidget(m_enableAllButton);

    m_disableAllButton = new ThemedToolButton(icons::Icon::Close, tr("Выключить все будильники"), m_toolbar);
    m_disableAllButton->setFixedSize(26, 26);
    m_disableAllButton->setIconSizePx(14);
    toolbarLayout->addWidget(m_disableAllButton);

    m_collapseButton = new ThemedToolButton(icons::Icon::ChevronUp, tr("Свернуть все группы"), m_toolbar);
    m_collapseButton->setFixedSize(26, 26);
    m_collapseButton->setIconSizePx(15);
    toolbarLayout->addWidget(m_collapseButton);
    layout->addWidget(m_toolbar);

    auto* fluentHeader = new QWidget(this);
    fluentHeader->setObjectName(QStringLiteral("Transparent"));
    auto* fluentLayout = new QHBoxLayout(fluentHeader);
    fluentLayout->setContentsMargins(0, 0, 0, 0);
    fluentLayout->setSpacing(8);
    m_fluentSearch = new QLineEdit(fluentHeader);
    m_fluentSearch->setObjectName(QStringLiteral("FluentSearch"));
    m_fluentSearch->setPlaceholderText(tr("Поиск будильников…"));
    m_fluentSearch->setClearButtonEnabled(true);
    m_fluentSearch->setMinimumWidth(180);
    m_fluentSearch->addAction(
        icons::qicon(icons::Icon::Search, ThemeManager::instance().current().accent),
        QLineEdit::LeadingPosition);
    fluentLayout->addWidget(m_fluentSearch, 1);
    auto* fluentMenu = new ThemedToolButton(icons::Icon::More, tr("Действия со списком"), fluentHeader);
    fluentMenu->setPopupMode(QToolButton::InstantPopup);
    auto* fluentMenuMenu = new QMenu(fluentMenu);
    auto* addGroupAction = fluentMenuMenu->addAction(tr("Новая группа"));
    connect(addGroupAction, &QAction::triggered, this, &AlarmsPage::createGroup);
    auto* holidaysAction = fluentMenuMenu->addAction(tr("Праздники и выходные"));
    connect(holidaysAction, &QAction::triggered, this, &AlarmsPage::showHolidays);
    fluentMenuMenu->addSeparator();
    auto* enableAllAction = fluentMenuMenu->addAction(tr("Включить все"));
    connect(enableAllAction, &QAction::triggered, this, [this] { m_model->setAllEnabled(true); });
    auto* disableAllAction = fluentMenuMenu->addAction(tr("Выключить все"));
    connect(disableAllAction, &QAction::triggered, this, [this] { m_model->setAllEnabled(false); });
    auto* pauseAction = fluentMenuMenu->addAction(tr("Приостановить все"));
    connect(pauseAction, &QAction::triggered, this, &AlarmsPage::showPauseMenu);
    auto* collapseAction = fluentMenuMenu->addAction(tr("Свернуть группы"));
    connect(collapseAction, &QAction::triggered, this, &AlarmsPage::collapseAll);
    fluentMenu->setMenu(fluentMenuMenu);
    fluentLayout->addWidget(fluentMenu);
    auto* fluentAddButton = new ThemedButton(tr("Добавить будильник"), icons::Icon::AlarmPlus,
                                             fluentHeader, true);
    connect(fluentAddButton, &QPushButton::clicked, this, &AlarmsPage::createAlarm);
    fluentLayout->addWidget(fluentAddButton);
    fluentHeader->hide();
    layout->addWidget(fluentHeader);

    m_model = new AlarmTableModel(m_store, this);
    m_delegate = new AlarmDelegate(this);
    m_table = new QTableView(this);
    m_table->setModel(m_model);
    m_table->setItemDelegate(m_delegate);
    m_table->setFrameShape(QFrame::NoFrame);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setShowGrid(false);
    m_table->setWordWrap(false);
    m_table->setTextElideMode(Qt::ElideRight);
    m_table->setMouseTracking(true);
    m_table->setAlternatingRowColors(false);
    m_table->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_table->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_table->setFocusPolicy(Qt::StrongFocus);
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setMinimumSectionSize(30);
    m_table->horizontalHeader()->setHighlightSections(false);
    m_table->horizontalHeader()->setSectionsClickable(false);
    m_table->horizontalHeader()->setMinimumSectionSize(36);
    m_table->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_table->setCornerButtonEnabled(false);
    m_table->installEventFilter(this);
    m_table->viewport()->setContextMenuPolicy(Qt::CustomContextMenu);

    const QFontMetrics metrics(m_table->font());
    m_table->setColumnWidth(AlarmTableModel::Enabled, 42);
    m_table->setColumnWidth(AlarmTableModel::Dial, 64);
    m_table->setColumnWidth(AlarmTableModel::Countdown,
                            std::max(132, metrics.horizontalAdvance(QStringLiteral("00 дн. 00:00:00")) + 32));
    m_table->setColumnWidth(AlarmTableModel::Time,
                            std::max(82, metrics.horizontalAdvance(QStringLiteral("00:00")) + 36));
    m_table->setColumnWidth(AlarmTableModel::Emoji, 60);
    auto* header = m_table->horizontalHeader();
    for (int column = 0; column < AlarmTableModel::ColumnCount; ++column) {
        header->setSectionResizeMode(column, QHeaderView::Fixed);
    }
    header->setSectionResizeMode(AlarmTableModel::Repeat, QHeaderView::Stretch);
    header->setSectionResizeMode(AlarmTableModel::Label, QHeaderView::Stretch);
    header->setStretchLastSection(false);

    auto* empty = new QWidget(this);
    auto* emptyLayout = new QVBoxLayout(empty);
    emptyLayout->setContentsMargins(24, 24, 24, 24);
    emptyLayout->addStretch(1);
    auto* emptyCard = makeCard(empty);
    emptyCard->setMaximumWidth(520);
    auto* emptyInner = new QVBoxLayout(emptyCard);
    emptyInner->setContentsMargins(30, 26, 30, 26);
    emptyInner->setSpacing(8);
    m_emptyIcon = new QLabel(emptyCard);
    m_emptyIcon->setAlignment(Qt::AlignHCenter);
    m_emptyIcon->setPixmap(icons::pixmap(icons::Icon::Alarm, 42,
                                        ThemeManager::instance().current().accent));
    emptyInner->addWidget(m_emptyIcon);
    m_emptyTitle = makeLabel(QString(), QStringLiteral("PageTitle"), emptyCard);
    m_emptyTitle->setAlignment(Qt::AlignHCenter);
    emptyInner->addWidget(m_emptyTitle);
    m_emptyHint = makeLabel(QString(), QStringLiteral("Muted"), emptyCard);
    m_emptyHint->setAlignment(Qt::AlignHCenter);
    m_emptyHint->setWordWrap(true);
    emptyInner->addWidget(m_emptyHint);
    emptyInner->addSpacing(6);
    m_emptyAction = new ThemedButton(tr("Создать будильник"), icons::Icon::AlarmPlus, emptyCard, true);
    m_emptyAction->setDefault(true);
    emptyInner->addWidget(m_emptyAction, 0, Qt::AlignHCenter);
    emptyLayout->addWidget(emptyCard, 0, Qt::AlignHCenter);
    emptyLayout->addStretch(1);

    m_listStack = new QStackedWidget(this);
    m_listStack->setObjectName(QStringLiteral("Transparent"));
    m_listStack->addWidget(m_table);
    m_listStack->addWidget(empty);
    layout->addWidget(m_listStack, 1);

    m_cardScroll = new QScrollArea(this);
    m_cardScroll->setObjectName(QStringLiteral("Transparent"));
    m_cardScroll->setWidgetResizable(true);
    m_cardScroll->setFrameShape(QFrame::NoFrame);
    m_cardScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_cardScroll->viewport()->setAutoFillBackground(false);
    m_cardHost = new QWidget(m_cardScroll);
    m_cardHost->setObjectName(QStringLiteral("Transparent"));
    m_cardLayout = new QVBoxLayout(m_cardHost);
    m_cardLayout->setContentsMargins(0, 0, 8, 0);
    m_cardLayout->setSpacing(9);
    m_cardLayout->addStretch(1);
    m_cardScroll->setWidget(m_cardHost);
    m_cardScroll->hide();
    layout->addWidget(m_cardScroll, 1);

    connect(m_fluentSearch, &QLineEdit::textChanged, this, &AlarmsPage::setSearch);

    connect(m_groupCombo, &QComboBox::currentIndexChanged, this, [this] {
        if (m_updatingGroups) {
            return;
        }
        m_model->setGroupFilter(m_groupCombo->currentData().toString());
        updateCountLabel();
        updateEmptyState();
    });
    connect(m_resetFilterButton, &QToolButton::clicked, this, &AlarmsPage::resetFilters);
    connect(m_sortCombo, &QComboBox::currentIndexChanged, this, [this] {
        m_model->setSortMode(static_cast<AlarmTableModel::SortMode>(
            m_sortCombo->currentData().toInt()));
    });
    connect(m_pauseButton, &QToolButton::clicked, this, &AlarmsPage::showPauseMenu);
    connect(holidaysButton, &QToolButton::clicked, this, &AlarmsPage::showHolidays);
    connect(m_enableAllButton, &QToolButton::clicked, this, [this] {
        m_model->setAllEnabled(true);
    });
    connect(m_disableAllButton, &QToolButton::clicked, this, [this] {
        m_model->setAllEnabled(false);
    });
    connect(m_collapseButton, &QToolButton::clicked, this, [this] {
        m_allCollapsed = !m_allCollapsed;
        m_model->collapseAll(m_allCollapsed);
        updateCollapseButton();
    });
    connect(m_emptyAction, &QPushButton::clicked, this, [this] {
        if (m_model->hasAlarms() || m_model->isFiltered()) {
            resetFilters();
        } else {
            createAlarm();
        }
    });
    connect(m_table, &QTableView::clicked, this, [this](const QModelIndex& index) {
        if (m_model->isGroupRow(index.row())) {
            m_model->toggleGroup(index.row());
        }
    });
    connect(m_table, &QTableView::doubleClicked, this, [this](const QModelIndex& index) {
        if (!m_model->isGroupRow(index.row())) {
            const QUuid id = m_model->alarmIdAtRow(index.row());
            if (!id.isNull()) {
                editAlarm(id);
            }
        }
    });
    connect(m_table->viewport(), &QWidget::customContextMenuRequested, this,
            &AlarmsPage::showContextMenu);
    connect(m_model, &QAbstractItemModel::modelReset, this, [this] {
        updateLayoutRows();
        updateCountLabel();
        updateEmptyState();
        m_resetFilterButton->setVisible(m_model->isFiltered());
    });

    updateGroupCombo();
    updateLayoutRows();
    updateCountLabel();
    updateEmptyState();
    updateCollapseButton();
    updatePauseButton();
}

void AlarmsPage::refreshClock(const QDateTime& now)
{
    m_model->tick(now);
}

void AlarmsPage::onActivated()
{
    m_model->tick(QDateTime::currentDateTime());
}

void AlarmsPage::themeUpdated()
{
    updateGroupIcons();
    updateEmptyState();
    m_emptyIcon->setPixmap(icons::pixmap(icons::Icon::Alarm, 42,
                                        ThemeManager::instance().current().accent));
    m_table->viewport()->update();
}

void AlarmsPage::applyThemeMode()
{
    const bool fluent = ThemeManager::instance().current().flavor == ThemeFlavor::WindowsClock;
    if (m_fluent == fluent) {
        if (fluent) {
            rebuildCards();
        }
        return;
    }
    m_fluent = fluent;
    if (m_toolbar != nullptr) {
        m_toolbar->setVisible(!fluent);
    }
    if (m_listStack != nullptr) {
        m_listStack->setVisible(!fluent);
    }
    QWidget* fluentHeader = m_fluentSearch != nullptr ? m_fluentSearch->parentWidget() : nullptr;
    if (fluentHeader != nullptr) {
        fluentHeader->setVisible(fluent);
    }
    if (m_cardScroll != nullptr) {
        m_cardScroll->setVisible(fluent);
    }
    rebuildCards();
}

void AlarmsPage::focusSearch()
{
    if (m_fluentSearch != nullptr) {
        m_fluentSearch->setFocus(Qt::OtherFocusReason);
        m_fluentSearch->selectAll();
    }
}

void AlarmsPage::resizeEvent(QResizeEvent* event)
{
    PageWidget::resizeEvent(event);
    if (m_countLabel != nullptr) {
        m_countLabel->setVisible(width() >= 820);
    }
}

void AlarmsPage::createAlarm()
{
    createAlarmImpl(QDate());
}

void AlarmsPage::createAlarmOnDate(const QDate& date)
{
    createAlarmImpl(date);
}

void AlarmsPage::editAlarmById(const QUuid& id)
{
    editAlarm(id);
}

void AlarmsPage::createAlarmImpl(const QDate& presetDate)
{
    Alarm alarm;
    QTime base = QTime::currentTime().addSecs(3600);
    alarm.time = QTime(base.hour(), base.minute() - base.minute() % 5);
    alarm.onceDate = QDate::currentDate().addDays(1);
    alarm.group = defaultGroup();
    alarm.enabled = true;
    alarm.repeat = RepeatKind::Daily;
    if (presetDate.isValid()) {
        alarm.repeat = RepeatKind::Once;
        alarm.onceDate = presetDate;
        if (presetDate == QDate::currentDate() && QTime::currentTime() >= QTime(9, 0)) {
            alarm.time = QTime(base.hour(), base.minute() - base.minute() % 5);
        } else {
            alarm.time = QTime(9, 0);
        }
    }
    alarm.color = nextAlarmColor(m_store).name();
    alarm.soundId = Settings::instance().defaultAlarmSound();
    alarm.soundEnabled = true;
    alarm.volume = Settings::instance().defaultAlarmVolume();
    alarm.loopSound = Settings::instance().defaultLoopSound();
    alarm.maxSoundMinutes = Settings::instance().defaultMaxSoundMinutes();
    alarm.snoozeEnabled = true;
    alarm.snoozeMinutes = Settings::instance().defaultSnoozeMinutes();
    alarm.maxSnoozes = Settings::instance().defaultMaxSnoozes();
    alarm.popupEnabled = Settings::instance().defaultAlarmPopup();
    alarm.preAlertMinutes = Settings::instance().defaultPreAlertMinutes();
    alarm.icon = QStringLiteral("⏰");

    AlarmDialog dialog(alarm, true, groupNames(), this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    const Alarm created = dialog.result();
    m_store.ensureGroup(created.group);
    m_store.data().alarms.push_back(created);
    m_store.commit(Domain::Alarms);
    updateGroupCombo();
    if (!m_model->groupFilter().isEmpty() && m_model->groupFilter() != created.group) {
        m_model->setGroupFilter(QString());
        updateGroupCombo();
    }
    reloadSelecting(created.id);
}

void AlarmsPage::createGroup()
{
    bool ok = false;
    const QString name = QInputDialog::getText(this, tr("Новая группа"),
                                               tr("Название группы:"), QLineEdit::Normal,
                                               QString(), &ok)
                             .trimmed();
    if (!ok || name.isEmpty()) {
        return;
    }
    for (const auto& group : m_store.data().groups) {
        if (group.name.compare(name, Qt::CaseInsensitive) == 0) {
            showMessage(tr("Группы"), tr("Группа с таким названием уже существует."));
            return;
        }
    }
    m_store.ensureGroup(name);
    m_store.commit(Domain::Alarms);
    updateGroupCombo();
    const int index = m_groupCombo->findData(name);
    if (index >= 0) {
        m_groupCombo->setCurrentIndex(index);
    }
}

void AlarmsPage::editSelected()
{
    const int row = currentRow();
    if (row < 0 || m_model->isGroupRow(row)) {
        return;
    }
    const QUuid id = m_model->alarmIdAtRow(row);
    if (!id.isNull()) {
        editAlarm(id);
    }
}

void AlarmsPage::duplicateSelected()
{
    const int row = currentRow();
    if (row < 0 || m_model->isGroupRow(row)) {
        return;
    }
    const QUuid id = m_model->alarmIdAtRow(row);
    if (!id.isNull()) {
        duplicateAlarm(id);
    }
}

void AlarmsPage::deleteSelected()
{
    const int row = currentRow();
    if (row < 0 || m_model->isGroupRow(row)) {
        return;
    }
    const QUuid id = m_model->alarmIdAtRow(row);
    if (!id.isNull()) {
        deleteAlarm(id);
    }
}

void AlarmsPage::toggleSelected()
{
    const int row = currentRow();
    if (row < 0) {
        return;
    }
    if (m_model->isGroupRow(row)) {
        m_model->toggleGroup(row);
        return;
    }
    m_model->toggleAlarm(row);
}

void AlarmsPage::collapseAll()
{
    m_allCollapsed = true;
    m_model->collapseAll(true);
    updateCollapseButton();
}

void AlarmsPage::setSearch(const QString& text)
{
    m_model->setSearch(text);
    updateCountLabel();
    updateEmptyState();
    m_resetFilterButton->setVisible(m_model->isFiltered());
}

void AlarmsPage::showHolidays()
{
    NonWorkingDaysDialog dialog(m_store, this);
    dialog.exec();
    reload();
}

void AlarmsPage::applyDensity()
{
    updateLayoutRows();
    m_table->viewport()->update();
    if (m_fluent) {
        rebuildCards();
    }
}

void AlarmsPage::reload()
{
    updateGroupCombo();
    m_model->reload();
    updateLayoutRows();
    updateCountLabel();
    updateEmptyState();
    updatePauseButton();
}

void AlarmsPage::editAlarm(const QUuid& id)
{
    Alarm* existing = m_store.findAlarm(id);
    if (existing == nullptr) {
        return;
    }
    Alarm copy = *existing;
    AlarmDialog dialog(copy, false, groupNames(), this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    const Alarm updated = dialog.result();
    Alarm* target = m_store.findAlarm(id);
    if (target == nullptr) {
        return;
    }
    m_store.ensureGroup(updated.group);
    *target = updated;
    m_store.commit(Domain::Alarms);
    updateGroupCombo();
    if (!m_model->groupFilter().isEmpty() && m_model->groupFilter() != updated.group) {
        const int index = m_groupCombo->findData(updated.group);
        if (index >= 0) {
            m_groupCombo->setCurrentIndex(index);
        }
    }
    reloadSelecting(updated.id);
}

void AlarmsPage::duplicateAlarm(const QUuid& id)
{
    const Alarm* source = m_store.findAlarm(id);
    if (source == nullptr) {
        return;
    }
    Alarm copy = *source;
    copy.id = QUuid::createUuid();
    copy.label = copy.label.isEmpty() ? tr("Будильник (копия)") : tr("%1 (копия)").arg(copy.label);
    copy.enabled = false;
    copy.snoozeUntil = QDateTime();
    copy.snoozeCount = 0;
    copy.lastTriggered = QDateTime();
    if (copy.repeat == RepeatKind::Interval) {
        copy.intervalAnchor = QDateTime::currentDateTime();
    }
    m_store.ensureGroup(copy.group);
    m_store.data().alarms.push_back(copy);
    m_store.commit(Domain::Alarms);
    updateGroupCombo();
    reloadSelecting(copy.id);
}

void AlarmsPage::deleteAlarm(const QUuid& id)
{
    Alarm* alarm = m_store.findAlarm(id);
    if (alarm == nullptr) {
        return;
    }
    const QString label = alarm->label.isEmpty() ? tr("Без названия") : alarm->label;
    QMessageBox box(QMessageBox::Question, tr("Удалить будильник"),
                    tr("Удалить будильник «%1»?").arg(label),
                    QMessageBox::Yes | QMessageBox::No, this);
    box.setDefaultButton(QMessageBox::No);
    if (box.exec() != QMessageBox::Yes) {
        return;
    }
    const auto it = std::find_if(m_store.data().alarms.begin(), m_store.data().alarms.end(),
                                 [&id](const Alarm& entry) { return entry.id == id; });
    if (it == m_store.data().alarms.end()) {
        return;
    }
    m_store.data().alarms.erase(it);
    m_store.commit(Domain::Alarms);
    updateGroupCombo();
    reload();
}

void AlarmsPage::showContextMenu(const QPoint& viewportPosition)
{
    const QModelIndex index = m_table->indexAt(viewportPosition);
    const QColor accent = ThemeManager::instance().current().accent;
    QMenu menu(this);
    if (index.isValid() && m_model->isGroupRow(index.row())) {
        const QString group = index.data(Qt::DisplayRole).toString();
        QAction* addAlarmAction = menu.addAction(icons::qicon(icons::Icon::AlarmPlus, accent),
                                                 tr("Новый будильник в группе"));
        connect(addAlarmAction, &QAction::triggered, this, [this, group] {
            const int comboIndex = m_groupCombo->findData(group);
            if (comboIndex >= 0) {
                m_groupCombo->setCurrentIndex(comboIndex);
            }
            createAlarm();
        });
        menu.addSeparator();
        QAction* enableGroupAction = menu.addAction(icons::qicon(icons::Icon::Check, accent),
                                                    tr("Включить группу"));
        connect(enableGroupAction, &QAction::triggered, this,
                [this, row = index.row()] { m_model->setGroupEnabled(row, true); });
        QAction* disableGroupAction = menu.addAction(icons::qicon(icons::Icon::Close, accent),
                                                     tr("Выключить группу"));
        connect(disableGroupAction, &QAction::triggered, this,
                [this, row = index.row()] { m_model->setGroupEnabled(row, false); });
        menu.addSeparator();
        QAction* renameAction = menu.addAction(icons::qicon(icons::Icon::Edit, accent),
                                               tr("Переименовать группу…"));
        connect(renameAction, &QAction::triggered, this, [this, group] { renameGroup(group); });
        QAction* deleteAction = menu.addAction(icons::qicon(icons::Icon::Delete, accent),
                                               tr("Удалить группу…"));
        connect(deleteAction, &QAction::triggered, this, [this, group] { deleteGroup(group); });
        menu.addSeparator();
        const bool collapsed = index.data(AlarmTableModel::CollapsedRole).toBool();
        QAction* toggleAction = menu.addAction(
            icons::qicon(collapsed ? icons::Icon::ChevronDown : icons::Icon::ChevronUp, accent),
            collapsed ? tr("Развернуть группу") : tr("Свернуть группу"));
        connect(toggleAction, &QAction::triggered, this,
                [this, row = index.row()] { m_model->toggleGroup(row); });
    } else if (index.isValid()) {
        const QUuid id = index.data(AlarmTableModel::AlarmIdRole).toUuid();
        const bool enabled = index.data(AlarmTableModel::EnabledRole).toBool();
        QAction* editAction = menu.addAction(icons::qicon(icons::Icon::Edit, accent),
                                             tr("Изменить"));
        connect(editAction, &QAction::triggered, this, [this, id] { editAlarm(id); });
        QAction* duplicateAction = menu.addAction(icons::qicon(icons::Icon::Copy, accent),
                                                  tr("Создать копию"));
        connect(duplicateAction, &QAction::triggered, this, [this, id] { duplicateAlarm(id); });
        menu.addSeparator();
        QAction* toggleAction = menu.addAction(
            icons::qicon(enabled ? icons::Icon::Close : icons::Icon::Check, accent),
            enabled ? tr("Выключить") : tr("Включить"));
        connect(toggleAction, &QAction::triggered, this, [this, row = index.row()] {
            m_model->toggleAlarm(row);
        });
        QAction* deleteAction = menu.addAction(icons::qicon(icons::Icon::Delete, accent),
                                               tr("Удалить"));
        connect(deleteAction, &QAction::triggered, this, [this, id] { deleteAlarm(id); });
    } else {
        QAction* newAlarmAction = menu.addAction(icons::qicon(icons::Icon::AlarmPlus, accent),
                                                 tr("Новый будильник"));
        connect(newAlarmAction, &QAction::triggered, this, &AlarmsPage::createAlarm);
        QAction* newGroupAction = menu.addAction(icons::qicon(icons::Icon::FolderPlus, accent),
                                                 tr("Новая группа"));
        connect(newGroupAction, &QAction::triggered, this, &AlarmsPage::createGroup);
        menu.addSeparator();
        QAction* resetAction = menu.addAction(icons::qicon(icons::Icon::Reset, accent),
                                              tr("Сбросить фильтры"));
        connect(resetAction, &QAction::triggered, this, &AlarmsPage::resetFilters);
    }
    menu.exec(m_table->viewport()->mapToGlobal(viewportPosition));
}

void AlarmsPage::renameGroup(const QString& oldName)
{
    bool ok = false;
    const QString newName = QInputDialog::getText(this, tr("Переименовать группу"),
                                                  tr("Новое название:"), QLineEdit::Normal,
                                                  oldName, &ok)
                                .trimmed();
    if (!ok || newName.isEmpty() || newName == oldName) {
        return;
    }
    for (const auto& group : m_store.data().groups) {
        if (group.name.compare(newName, Qt::CaseInsensitive) == 0) {
            showMessage(tr("Группы"), tr("Группа с таким названием уже существует."));
            return;
        }
    }
    for (auto& group : m_store.data().groups) {
        if (group.name == oldName) {
            group.name = newName;
            break;
        }
    }
    for (auto& alarm : m_store.data().alarms) {
        if (alarm.group == oldName) {
            alarm.group = newName;
        }
    }
    const bool wasFiltering = m_model->groupFilter() == oldName;
    m_store.commit(Domain::Alarms);
    updateGroupCombo();
    if (wasFiltering) {
        const int index = m_groupCombo->findData(newName);
        if (index >= 0) {
            m_groupCombo->setCurrentIndex(index);
        }
    }
    reload();
}

void AlarmsPage::deleteGroup(const QString& name)
{
    if (name == QStringLiteral("Общие")) {
        showMessage(tr("Группы"), tr("Системную группу «Общие» нельзя удалить."));
        return;
    }
    const int count = static_cast<int>(std::count_if(
        m_store.data().alarms.cbegin(), m_store.data().alarms.cend(),
        [&name](const Alarm& alarm) { return alarm.group == name; }));
    QMessageBox box(QMessageBox::Question, tr("Удалить группу"),
                    tr("Удалить группу «%1»?%2")
                        .arg(name, count > 0
                                      ? tr("\n%1 будильник(ов) будет перемещён(о) в группу «Общие».")
                                            .arg(count)
                                      : QString()),
                    QMessageBox::Yes | QMessageBox::No, this);
    box.setDefaultButton(QMessageBox::No);
    if (box.exec() != QMessageBox::Yes) {
        return;
    }
    m_store.ensureGroup(QStringLiteral("Общие"));
    for (auto& alarm : m_store.data().alarms) {
        if (alarm.group == name) {
            alarm.group = QStringLiteral("Общие");
        }
    }
    m_store.data().groups.erase(
        std::remove_if(m_store.data().groups.begin(), m_store.data().groups.end(),
                       [&name](const AlarmGroup& group) { return group.name == name; }),
        m_store.data().groups.end());
    m_store.commit(Domain::Alarms);
    if (m_model->groupFilter() == name) {
        m_model->setGroupFilter(QString());
    }
    updateGroupCombo();
    reload();
}

void AlarmsPage::resetFilters()
{
    {
        const QSignalBlocker blocker(m_groupCombo);
        m_groupCombo->setCurrentIndex(m_groupCombo->count() > 0 ? 0 : -1);
    }
    m_model->setGroupFilter(QString());
    m_model->setSearch(QString());
    emit searchResetRequested();
    updateCountLabel();
    updateEmptyState();
    m_resetFilterButton->setVisible(false);
}

void AlarmsPage::updateGroupCombo()
{
    m_updatingGroups = true;
    const QString current = m_model->groupFilter();
    QSignalBlocker blocker(m_groupCombo);
    m_groupCombo->clear();
    m_groupCombo->addItem(icons::qicon(icons::Icon::List, ThemeManager::instance().current().accent),
                          tr("Все группы"), QString());
    for (const auto& group : m_store.data().groups) {
        const int count = static_cast<int>(std::count_if(
            m_store.data().alarms.cbegin(), m_store.data().alarms.cend(),
            [&group](const Alarm& alarm) { return alarm.group == group.name; }));
        const QString label = count > 0 ? QStringLiteral("%1  (%2)").arg(group.name).arg(count)
                                        : group.name;
        m_groupCombo->addItem(
            icons::qicon(icons::Icon::Folder, ThemeManager::instance().current().accent), label,
            group.name);
    }
    int index = m_groupCombo->findData(current);
    if (index < 0) {
        index = 0;
    }
    m_groupCombo->setCurrentIndex(index);
    m_updatingGroups = false;
    const QString applied = m_groupCombo->currentData().toString();
    if (applied != m_model->groupFilter()) {
        m_model->setGroupFilter(applied);
    }
}

void AlarmsPage::updateGroupIcons()
{
    const QColor accent = ThemeManager::instance().current().accent;
    for (int i = 0; i < m_groupCombo->count(); ++i) {
        const icons::Icon icon = i == 0 ? icons::Icon::List : icons::Icon::Folder;
        m_groupCombo->setItemIcon(i, icons::qicon(icon, accent));
    }
}

void AlarmsPage::updateLayoutRows()
{
    const int density = Settings::instance().rowDensity();
    const int groupHeight = density == 0 ? 26 : (density == 2 ? 34 : 30);
    const int rows = m_model->rowCount();
    for (int row = 0; row < rows; ++row) {
        if (m_model->isGroupRow(row)) {
            m_table->setSpan(row, 0, 1, AlarmTableModel::ColumnCount);
            m_table->setRowHeight(row, groupHeight);
        } else {
            m_table->setSpan(row, 0, 1, 1);
            m_table->setRowHeight(row, AlarmDelegate::alarmRowHeight());
        }
    }
}

void AlarmsPage::updateCountLabel()
{
    int visible = 0;
    for (int row = 0; row < m_model->rowCount(); ++row) {
        if (!m_model->isGroupRow(row)) {
            ++visible;
        }
    }
    const int total = static_cast<int>(m_store.data().alarms.size());
    int enabled = 0;
    for (const auto& alarm : m_store.data().alarms) {
        if (alarm.enabled) {
            ++enabled;
        }
    }
    QString text = m_model->isFiltered()
                       ? tr("Показано: %1 из %2").arg(visible).arg(total)
                       : tr("Включено: %1 из %2").arg(enabled).arg(total);
    const QDateTime suspendedUntil = Settings::instance().alarmsSuspendedUntil();
    if (suspendedUntil > QDateTime::currentDateTime()) {
        text += tr(" · пауза до %1").arg(suspendedUntil.toString(QStringLiteral("HH:mm")));
    }
    m_countLabel->setText(text);
    if (m_enableAllButton != nullptr) {
        m_enableAllButton->setEnabled(total > 0);
    }
    if (m_disableAllButton != nullptr) {
        m_disableAllButton->setEnabled(enabled > 0);
    }
}

void AlarmsPage::updateEmptyState()
{
    const bool empty = m_model->rowCount() == 0;
    if (m_fluent) {
        rebuildCards();
        return;
    }
    if (!empty) {
        m_listStack->setCurrentIndex(0);
        return;
    }
    m_listStack->setCurrentIndex(1);
    const bool noAlarms = !m_model->hasAlarms() && !m_model->isFiltered();
    if (noAlarms) {
        m_emptyTitle->setText(tr("Пока нет будильников"));
        m_emptyHint->setText(tr("Создайте первый будильник — он появится здесь вместе "
                                "с обратным отсчётом и расписанием."));
        m_emptyAction->setText(tr("Создать будильник"));
        m_emptyAction->setGlyph(icons::Icon::AlarmPlus);
    } else {
        m_emptyTitle->setText(tr("Ничего не найдено"));
        m_emptyHint->setText(tr("Измените поисковый запрос или сбросьте фильтр по группе."));
        m_emptyAction->setText(tr("Сбросить фильтры"));
        m_emptyAction->setGlyph(icons::Icon::Reset);
    }
    m_emptyIcon->setPixmap(icons::pixmap(noAlarms ? icons::Icon::Alarm : icons::Icon::Search, 42,
                                        ThemeManager::instance().current().accent));
}

void AlarmsPage::updateCollapseButton()
{
    m_collapseButton->setGlyph(m_allCollapsed ? icons::Icon::ChevronDown
                                               : icons::Icon::ChevronUp);
    m_collapseButton->setToolTip(m_allCollapsed ? tr("Развернуть все группы")
                                                 : tr("Свернуть все группы"));
}

void AlarmsPage::reloadSelecting(const QUuid& id)
{
    m_model->reload();
    updateLayoutRows();
    updateCountLabel();
    updateEmptyState();
    selectAlarm(id);
}

void AlarmsPage::selectAlarm(const QUuid& id)
{
    const int row = m_model->rowForAlarm(id);
    if (row < 0) {
        return;
    }
    m_table->selectRow(row);
    m_table->scrollTo(m_model->index(row, 0), QAbstractItemView::PositionAtCenter);
}

int AlarmsPage::currentRow() const
{
    const QModelIndex index = m_table->currentIndex();
    return index.isValid() ? index.row() : -1;
}

QUuid AlarmsPage::selectedAlarmId() const
{
    const int row = currentRow();
    if (row < 0 || m_model->isGroupRow(row)) {
        return {};
    }
    return m_model->alarmIdAtRow(row);
}

QString AlarmsPage::defaultGroup() const
{
    if (!m_model->groupFilter().isEmpty()) {
        return m_model->groupFilter();
    }
    const QUuid id = selectedAlarmId();
    if (!id.isNull()) {
        if (const Alarm* alarm = m_store.findAlarm(id); alarm != nullptr) {
            return alarm->group;
        }
    }
    if (!m_store.data().groups.empty()) {
        return m_store.data().groups.front().name;
    }
    return QStringLiteral("Общие");
}

QStringList AlarmsPage::groupNames() const
{
    QStringList names;
    for (const auto& group : m_store.data().groups) {
        names.append(group.name);
    }
    if (names.isEmpty()) {
        names.append(QStringLiteral("Общие"));
    }
    return names;
}

void AlarmsPage::updatePauseButton()
{
    if (m_pauseButton == nullptr) {
        return;
    }
    const QDateTime until = Settings::instance().alarmsSuspendedUntil();
    const bool active = until > QDateTime::currentDateTime();
    m_pauseButton->setGlyph(active ? icons::Icon::Play : icons::Icon::Pause);
    m_pauseButton->setToolTip(
        active ? tr("Будильники на паузе до %1. Нажмите, чтобы снять или изменить.")
                     .arg(until.toString(QStringLiteral("HH:mm")))
               : tr("Временно приостановить все будильники"));
}

void AlarmsPage::showPauseMenu()
{
    const QDateTime now = QDateTime::currentDateTime();
    const QDateTime until = Settings::instance().alarmsSuspendedUntil();
    const bool active = until > now;

    QMenu menu(this);
    if (active) {
        auto* resume = menu.addAction(icons::qicon(icons::Icon::Play,
                                                    ThemeManager::instance().current().accent),
                                       tr("Снять паузу"));
        connect(resume, &QAction::triggered, this, [] {
            Settings::instance().setAlarmsSuspendedUntil(QDateTime());
        });
        menu.addSeparator();
    }
    const auto addPause = [&](const QString& text, const QDateTime& target) {
        auto* action = menu.addAction(text);
        connect(action, &QAction::triggered, this, [target] {
            Settings::instance().setAlarmsSuspendedUntil(target);
        });
    };
    addPause(tr("На 15 минут"), now.addSecs(15 * 60));
    addPause(tr("На 30 минут"), now.addSecs(30 * 60));
    addPause(tr("На 1 час"), now.addSecs(60 * 60));
    addPause(tr("На 2 часа"), now.addSecs(2 * 60 * 60));
    QDateTime nextMorning(now.date().addDays(1), QTime(8, 0));
    if (now.time() < QTime(8, 0)) {
        nextMorning = QDateTime(now.date(), QTime(8, 0));
    }
    addPause(tr("До 08:00"), nextMorning);
    menu.exec(m_pauseButton->mapToGlobal(QPoint(0, m_pauseButton->height())));
    updatePauseButton();
    updateCountLabel();
}

void AlarmsPage::showMessage(const QString& title, const QString& text)
{
    QMessageBox::information(this, title, text);
}

bool AlarmsPage::eventFilter(QObject* watched, QEvent* event)
{
    const QUuid cardId = watched->property("alarmCardId").toUuid();
    if (!cardId.isNull() && event->type() == QEvent::MouseButtonDblClick) {
        editAlarm(cardId);
        return true;
    }
    if (watched == m_table && event->type() == QEvent::KeyPress) {
        auto* key = static_cast<QKeyEvent*>(event);
        if (key->matches(QKeySequence::Delete)) {
            deleteSelected();
            return true;
        }
        if (key->key() == Qt::Key_Space) {
            toggleSelected();
            return true;
        }
        if (key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter
            || key->key() == Qt::Key_F2) {
            editSelected();
            return true;
        }
        if (key->modifiers() == Qt::ControlModifier && key->key() == Qt::Key_D) {
            duplicateSelected();
            return true;
        }
        if (key->key() == Qt::Key_Escape && m_model->isFiltered()) {
            resetFilters();
            return true;
        }
    }
    return PageWidget::eventFilter(watched, event);
}

void AlarmsPage::rebuildCards()
{
    if (m_cardLayout == nullptr) {
        return;
    }
    while (QLayoutItem* item = m_cardLayout->takeAt(0)) {
        if (item->widget() != nullptr) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    const int rows = m_model->rowCount();
    if (rows == 0) {
        auto* emptyCard = new QFrame(m_cardHost);
        emptyCard->setObjectName(QStringLiteral("FluentCard"));
        auto* layout = new QVBoxLayout(emptyCard);
        layout->setContentsMargins(28, 26, 28, 26);
        layout->setSpacing(8);
        auto* icon = new QLabel(emptyCard);
        icon->setAlignment(Qt::AlignHCenter);
        icon->setPixmap(icons::pixmap(icons::Icon::Alarm, 42,
                                      ThemeManager::instance().current().accent));
        layout->addWidget(icon);
        auto* title = new QLabel(m_model->isFiltered() ? tr("Ничего не найдено")
                                                        : tr("Пока нет будильников"),
                                 emptyCard);
        title->setObjectName(QStringLiteral("PageTitle"));
        title->setAlignment(Qt::AlignHCenter);
        layout->addWidget(title);
        auto* hint = new QLabel(m_model->isFiltered()
                                    ? tr("Измените поисковый запрос или сбросьте фильтр по группе.")
                                    : tr("Создайте первый будильник — он появится здесь."),
                                emptyCard);
        hint->setObjectName(QStringLiteral("Muted"));
        hint->setAlignment(Qt::AlignHCenter);
        hint->setWordWrap(true);
        layout->addWidget(hint);
        auto* action = new ThemedButton(m_model->isFiltered() ? tr("Сбросить фильтры")
                                                               : tr("Создать будильник"),
                                        m_model->isFiltered() ? icons::Icon::Reset
                                                              : icons::Icon::AlarmPlus,
                                        emptyCard, true);
        connect(action, &QPushButton::clicked, this, [this] {
            if (m_model->isFiltered()) {
                resetFilters();
            } else {
                createAlarm();
            }
        });
        layout->addWidget(action, 0, Qt::AlignHCenter);
        emptyCard->setMaximumWidth(520);
        m_cardLayout->addWidget(emptyCard, 0, Qt::AlignHCenter);
        m_cardLayout->addStretch(1);
        return;
    }

    for (int row = 0; row < rows; ++row) {
        const QModelIndex index = m_model->index(row, AlarmTableModel::Enabled);
        if (m_model->isGroupRow(row)) {
            const QString name = index.data(Qt::DisplayRole).toString();
            const int count = index.data(AlarmTableModel::CountRole).toInt();
            auto* header = new QLabel(QStringLiteral("%1 · %2").arg(name).arg(count), m_cardHost);
            header->setObjectName(QStringLiteral("SectionLabel"));
            header->setContentsMargins(4, 8, 0, 2);
            m_cardLayout->addWidget(header);
            continue;
        }
        const Alarm* alarm = m_model->alarmAtRow(row);
        if (alarm != nullptr) {
            m_cardLayout->addWidget(makeAlarmCard(*alarm));
        }
    }
    m_cardLayout->addStretch(1);
}

QWidget* AlarmsPage::makeAlarmCard(const Alarm& alarm)
{
    auto* card = new QFrame(m_cardHost);
    card->setObjectName(QStringLiteral("FluentCard"));
    card->setMinimumHeight(108);
    card->setProperty("alarmCardId", alarm.id);
    card->setContextMenuPolicy(Qt::CustomContextMenu);
    card->installEventFilter(this);

    auto* layout = new QHBoxLayout(card);
    layout->setContentsMargins(18, 14, 18, 14);
    layout->setSpacing(18);

    auto* text = new QVBoxLayout;
    text->setSpacing(5);
    auto* time = new QLabel(schedule::timeText(alarm.time, Settings::instance().use24h()), card);
    time->setObjectName(QStringLiteral("FluentTime"));
    time->setAttribute(Qt::WA_TransparentForMouseEvents);
    auto* label = new QLabel(alarm.label.isEmpty() ? tr("Будильник") : alarm.label, card);
    label->setObjectName(QStringLiteral("FluentLabel"));
    label->setAttribute(Qt::WA_TransparentForMouseEvents);
    text->addWidget(time);
    text->addWidget(label);

    auto* days = new QHBoxLayout;
    days->setSpacing(5);
    std::array<bool, 7> mask{};
    switch (alarm.repeat) {
    case RepeatKind::Daily:
        mask.fill(true);
        break;
    case RepeatKind::Weekdays:
        mask = {true, true, true, true, true, false, false};
        break;
    case RepeatKind::Weekends:
        mask = {false, false, false, false, false, true, true};
        break;
    case RepeatKind::Weekly:
        for (const Qt::DayOfWeek day : alarm.days) {
            const int index = static_cast<int>(day) - 1;
            if (index >= 0 && index < 7) {
                mask[static_cast<std::size_t>(index)] = true;
            }
        }
        break;
    default:
        break;
    }
    const QStringList names{tr("Пн"), tr("Вт"), tr("Ср"), tr("Чт"), tr("Пт"), tr("Сб"), tr("Вс")};
    for (int index = 0; index < names.size(); ++index) {
        auto* day = new QLabel(names.at(index), card);
        day->setProperty("fluentChip", true);
        day->setProperty("selected", mask[static_cast<std::size_t>(index)]);
        day->setFixedSize(28, 28);
        day->setAlignment(Qt::AlignCenter);
        day->setAttribute(Qt::WA_TransparentForMouseEvents);
        days->addWidget(day);
    }
    days->addStretch(1);
    text->addLayout(days);
    layout->addLayout(text, 1);

    auto* side = new QVBoxLayout;
    side->setSpacing(12);
    const QDateTime now = QDateTime::currentDateTime();
    const QDateTime next = schedule::nextOccurrence(alarm, now);
    QString meta = schedule::repeatSummary(alarm);
    if (alarm.snoozeUntil.isValid() && alarm.snoozeUntil > now) {
        meta = tr("Отложен до %1")
                   .arg(schedule::timeText(alarm.snoozeUntil.time(), Settings::instance().use24h()));
    } else if (next.isValid()) {
        meta += QStringLiteral(" · ") + schedule::dateTimeText(next, now, Settings::instance().use24h());
    }
    auto* metaLabel = new QLabel(meta, card);
    metaLabel->setObjectName(QStringLiteral("FluentMeta"));
    metaLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    metaLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    auto* toggle = new SwitchButton(card);
    toggle->setChecked(alarm.enabled);
    toggle->setToolTip(tr("Включить или выключить будильник"));
    side->addWidget(metaLabel);
    side->addWidget(toggle, 0, Qt::AlignRight);
    layout->addLayout(side);

    const QUuid id = alarm.id;
    connect(toggle, &QAbstractButton::toggled, this, [this, id](bool checked) {
        Alarm* current = m_store.findAlarm(id);
        if (current == nullptr || current->enabled == checked) {
            return;
        }
        current->enabled = checked;
        current->snoozeUntil = QDateTime();
        current->snoozeCount = 0;
        m_store.commit(Domain::Alarms);
        reload();
    });
    connect(card, &QWidget::customContextMenuRequested, this, [this, id, card](const QPoint& pos) {
        showCardContextMenu(id, card->mapToGlobal(pos));
    });
    return card;
}

void AlarmsPage::showCardContextMenu(const QUuid& id, const QPoint& globalPosition)
{
    const Alarm* alarm = m_store.findAlarm(id);
    if (alarm == nullptr) {
        return;
    }
    const QColor accent = ThemeManager::instance().current().accent;
    QMenu menu(this);
    auto* editAction = menu.addAction(icons::qicon(icons::Icon::Edit, accent), tr("Изменить"));
    connect(editAction, &QAction::triggered, this, [this, id] { editAlarm(id); });
    auto* duplicateAction = menu.addAction(icons::qicon(icons::Icon::Copy, accent), tr("Дублировать"));
    connect(duplicateAction, &QAction::triggered, this, [this, id] { duplicateAlarm(id); });
    auto* toggleAction = menu.addAction(
        icons::qicon(alarm->enabled ? icons::Icon::Pause : icons::Icon::Check, accent),
        alarm->enabled ? tr("Выключить") : tr("Включить"));
    connect(toggleAction, &QAction::triggered, this, [this, id] {
        Alarm* current = m_store.findAlarm(id);
        if (current == nullptr) {
            return;
        }
        current->enabled = !current->enabled;
        current->snoozeUntil = QDateTime();
        current->snoozeCount = 0;
        m_store.commit(Domain::Alarms);
        reload();
    });
    menu.addSeparator();
    auto* deleteAction = menu.addAction(icons::qicon(icons::Icon::Delete, accent), tr("Удалить"));
    connect(deleteAction, &QAction::triggered, this, [this, id] { deleteAlarm(id); });
    menu.exec(globalPosition);
}

} // namespace pac::ui
