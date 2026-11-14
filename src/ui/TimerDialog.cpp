#include "TimerDialog.h"

#include "TimerStageDialog.h"
#include "UiKit.h"
#include "core/Catalog.h"
#include "core/Settings.h"
#include "core/Schedule.h"
#include "core/SoundPlayer.h"
#include "core/TimerPlan.h"
#include "core/Theme.h"

#include <QAbstractItemModel>
#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QDateTimeEdit>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QFont>
#include <QIcon>
#include <QPainter>
#include <QPixmap>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QStyle>
#include <QVBoxLayout>

#include <algorithm>

namespace pac::ui {
namespace {

QGroupBox* groupBox(const QString& title, QWidget* parent, QVBoxLayout** layout)
{
    auto* box = new QGroupBox(title, parent);
    auto* boxLayout = new QVBoxLayout(box);
    boxLayout->setContentsMargins(12, 10, 12, 12);
    boxLayout->setSpacing(8);
    *layout = boxLayout;
    return box;
}

[[nodiscard]] QLabel* mutedLabel(const QString& text, QWidget* parent, int width = 106)
{
    auto* label = new QLabel(text, parent);
    label->setObjectName(QStringLiteral("Muted"));
    label->setFixedWidth(width);
    return label;
}

[[nodiscard]] QString durationText(int seconds)
{
    const int hours = seconds / 3600;
    const int minutes = (seconds % 3600) / 60;
    const int rest = seconds % 60;
    QStringList parts;
    if (hours > 0) {
        parts.append(QObject::tr("%1 ч").arg(hours));
    }
    if (minutes > 0) {
        parts.append(QObject::tr("%1 мин").arg(minutes));
    }
    if (rest > 0 || parts.isEmpty()) {
        parts.append(QObject::tr("%1 с").arg(rest));
    }
    return parts.join(QLatin1Char(' '));
}

[[nodiscard]] QIcon stageColorIcon(const QString& color)
{
    QPixmap pixmap(16, 16);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(QColor(0, 0, 0, 55), 1.0));
    painter.setBrush(QColor(color));
    painter.drawEllipse(QRectF(2.0, 2.0, 12.0, 12.0));
    return QIcon(pixmap);
}

[[nodiscard]] QString stageColorAt(int index)
{
    const auto& colors = catalog::alarmColors();
    return colors.at(static_cast<std::size_t>(index) % colors.size());
}

void appendStage(std::vector<TimerStage>& stages, const QString& name, int seconds,
                 TimerStageKind kind, int colorIndex = 0)
{
    TimerStage stage;
    stage.name = name;
    stage.durationSec = std::max(1, seconds);
    stage.kind = kind;
    stage.color = stageColorAt(colorIndex);
    stages.push_back(stage);
}

void fillModeTemplate(TimerMode mode, std::vector<TimerStage>& stages, int* cycles)
{
    stages.clear();
    *cycles = 1;
    switch (mode) {
    case TimerMode::Interval:
        appendStage(stages, QObject::tr("Работа"), 30, TimerStageKind::Work, 0);
        appendStage(stages, QObject::tr("Отдых"), 30, TimerStageKind::Rest, 3);
        *cycles = 5;
        break;
    case TimerMode::MultiStage:
        appendStage(stages, QObject::tr("Этап 1"), 300, TimerStageKind::Work, 0);
        appendStage(stages, QObject::tr("Этап 2"), 300, TimerStageKind::Rest, 4);
        break;
    case TimerMode::Hiit:
        appendStage(stages, QObject::tr("Интенсив"), 40, TimerStageKind::Work, 1);
        appendStage(stages, QObject::tr("Восстановление"), 20, TimerStageKind::Recovery, 3);
        *cycles = 8;
        break;
    case TimerMode::Tabata:
        appendStage(stages, QObject::tr("Интенсив"), 20, TimerStageKind::Work, 1);
        appendStage(stages, QObject::tr("Отдых"), 10, TimerStageKind::Rest, 3);
        *cycles = 8;
        break;
    case TimerMode::Kitchen:
        appendStage(stages, QObject::tr("Готовка"), 600, TimerStageKind::Custom, 5);
        break;
    case TimerMode::Workout:
        appendStage(stages, QObject::tr("Подход"), 45, TimerStageKind::Work, 2);
        appendStage(stages, QObject::tr("Отдых"), 15, TimerStageKind::Rest, 3);
        *cycles = 8;
        break;
    case TimerMode::Presentation:
        appendStage(stages, QObject::tr("Выступление"), 900, TimerStageKind::Work, 0);
        appendStage(stages, QObject::tr("Вопросы"), 300, TimerStageKind::Custom, 4);
        break;
    case TimerMode::Meeting:
        appendStage(stages, QObject::tr("Вступление"), 300, TimerStageKind::Prepare, 6);
        appendStage(stages, QObject::tr("Обсуждение"), 1200, TimerStageKind::Work, 0);
        appendStage(stages, QObject::tr("Итоги"), 300, TimerStageKind::Custom, 4);
        break;
    case TimerMode::Study:
        appendStage(stages, QObject::tr("Занятие"), 3000, TimerStageKind::Work, 0);
        appendStage(stages, QObject::tr("Перерыв"), 600, TimerStageKind::Rest, 3);
        *cycles = 4;
        break;
    default:
        break;
    }
}

[[nodiscard]] bool isSequenceMode(TimerMode mode)
{
    switch (mode) {
    case TimerMode::Interval:
    case TimerMode::MultiStage:
    case TimerMode::Hiit:
    case TimerMode::Tabata:
    case TimerMode::Kitchen:
    case TimerMode::Workout:
    case TimerMode::Presentation:
    case TimerMode::Meeting:
    case TimerMode::Study:
        return true;
    default:
        return false;
    }
}

} // namespace

TimerDialog::TimerDialog(const CountdownTimer& timer, bool isNew, QWidget* parent)
    : QDialog(parent)
    , m_timer(timer)
    , m_isNew(isNew)
{
    setWindowTitle(isNew ? tr("Новый таймер") : tr("Изменить таймер"));
    setMinimumWidth(560);
    buildUi();
}

void TimerDialog::buildUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(14, 14, 14, 12);
    root->setSpacing(10);

    m_summary = new QLabel(this);
    m_summary->setObjectName(QStringLiteral("Muted"));

    auto* header = makeCard(this);
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(18, 16, 18, 16);
    headerLayout->setSpacing(14);

    auto* icon = new QLabel(header);
    icon->setFixedSize(54, 54);
    icon->setAlignment(Qt::AlignCenter);
    icon->setPixmap(emojiPixmap(QStringLiteral("⏳"), 32));
    icon->setStyleSheet(QStringLiteral("background: rgba(63,127,214,38); border: 1px solid rgba(63,127,214,110); border-radius: 10px;"));
    headerLayout->addWidget(icon);

    auto* mainColumn = new QVBoxLayout;
    mainColumn->setSpacing(8);
    m_label = new QLineEdit(header);
    m_label->setPlaceholderText(tr("Название таймера"));
    QFont labelFont = m_label->font();
    labelFont.setPointSizeF(11.5);
    labelFont.setWeight(QFont::DemiBold);
    m_label->setFont(labelFont);
    m_label->setText(m_timer.label);
    mainColumn->addWidget(m_label);

    m_durationRow = new QWidget(header);
    auto* durationRow = new QHBoxLayout(m_durationRow);
    durationRow->setContentsMargins(0, 0, 0, 0);
    durationRow->setSpacing(5);
    m_hours = new QSpinBox(m_durationRow);
    m_hours->setRange(0, 99);
    m_hours->setSuffix(tr(" ч"));
    m_minutes = new QSpinBox(m_durationRow);
    m_minutes->setRange(0, 59);
    m_minutes->setSuffix(tr(" мин"));
    m_seconds = new QSpinBox(m_durationRow);
    m_seconds->setRange(0, 59);
    m_seconds->setSuffix(tr(" с"));
    for (QSpinBox* spin : {m_hours, m_minutes, m_seconds}) {
        spin->setMinimumWidth(95);
        QFont font = spin->font();
        font.setPointSizeF(12.0);
        spin->setFont(font);
        durationRow->addWidget(spin);
    }
    durationRow->addStretch(1);
    mainColumn->addWidget(m_durationRow);
    headerLayout->addLayout(mainColumn, 1);
    root->addWidget(header);

    const int totalSeconds = std::max(1, m_timer.durationSec);
    setDuration(totalSeconds / 3600, (totalSeconds % 3600) / 60, totalSeconds % 60);

    m_quickPresets = new QWidget(this);
    auto* presets = new QHBoxLayout(m_quickPresets);
    presets->setContentsMargins(4, 0, 4, 0);
    presets->setSpacing(5);
    presets->addWidget(mutedLabel(tr("Быстрый выбор"), m_quickPresets, 96));
    for (const int minutes : {1, 5, 10, 15, 25, 45, 60}) {
        auto* button = new QPushButton(QStringLiteral("%1 мин").arg(minutes), m_quickPresets);
        button->setFixedHeight(26);
        presets->addWidget(button);
        connect(button, &QPushButton::clicked, this, [this, minutes] {
            setDuration(minutes / 60, minutes % 60, 0);
        });
    }
    presets->addStretch(1);
    root->addWidget(m_quickPresets);

    QVBoxLayout* modeLayout = nullptr;
    auto* modeBox = groupBox(tr("Режим"), this, &modeLayout);
    {
        auto* row = new QHBoxLayout;
        row->setSpacing(8);
        row->addWidget(mutedLabel(tr("Режим"), modeBox, 106));
        m_mode = new QComboBox(modeBox);
        m_mode->addItem(tr("Обратный отсчёт"), static_cast<int>(TimerMode::Countdown));
        m_mode->addItem(tr("Прямой отсчёт до предела"), static_cast<int>(TimerMode::CountUp));
        m_mode->addItem(tr("Повторяющийся"), static_cast<int>(TimerMode::Repeating));
        m_mode->addItem(tr("Интервальный"), static_cast<int>(TimerMode::Interval));
        m_mode->addItem(tr("Многоэтапный"), static_cast<int>(TimerMode::MultiStage));
        m_mode->addItem(QStringLiteral("HIIT"), static_cast<int>(TimerMode::Hiit));
        m_mode->addItem(QStringLiteral("Tabata"), static_cast<int>(TimerMode::Tabata));
        m_mode->addItem(tr("Кухонный"), static_cast<int>(TimerMode::Kitchen));
        m_mode->addItem(tr("Тренировка"), static_cast<int>(TimerMode::Workout));
        m_mode->addItem(tr("Выступление"), static_cast<int>(TimerMode::Presentation));
        m_mode->addItem(tr("Встреча"), static_cast<int>(TimerMode::Meeting));
        m_mode->addItem(tr("Учёба"), static_cast<int>(TimerMode::Study));
        m_mode->addItem(tr("Сон"), static_cast<int>(TimerMode::Sleep));
        m_mode->addItem(tr("Дедлайн"), static_cast<int>(TimerMode::Deadline));
        m_mode->addItem(tr("До заданного времени"), static_cast<int>(TimerMode::Target));
        m_mode->addItem(tr("Случайный интервал"),
                        static_cast<int>(TimerMode::RandomInterval));
        row->addWidget(m_mode, 1);
        modeLayout->addLayout(row);
    }
    m_targetRow = new QWidget(modeBox);
    {
        auto* row = new QHBoxLayout(m_targetRow);
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(8);
        row->addWidget(mutedLabel(tr("Срок"), m_targetRow, 106));
        m_targetAt = new QDateTimeEdit(m_targetRow);
        m_targetAt->setCalendarPopup(true);
        m_targetAt->setDisplayFormat(QStringLiteral("dd.MM.yyyy HH:mm"));
        m_targetAt->setDateTime(QDateTime::currentDateTime().addSecs(3600));
        row->addWidget(m_targetAt, 1);
        modeLayout->addWidget(m_targetRow);
    }
    m_randomRow = new QWidget(modeBox);
    {
        auto* row = new QHBoxLayout(m_randomRow);
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(8);
        row->addWidget(mutedLabel(tr("Случайно от"), m_randomRow, 106));
        m_randomMin = new QSpinBox(m_randomRow);
        m_randomMin->setRange(1, 7 * 86'400);
        m_randomMin->setValue(std::max(1, m_timer.randomMinSec));
        m_randomMin->setSuffix(tr(" с"));
        m_randomMax = new QSpinBox(m_randomRow);
        m_randomMax->setRange(1, 7 * 86'400);
        m_randomMax->setValue(std::max(1, m_timer.randomMaxSec));
        m_randomMax->setSuffix(tr(" с"));
        row->addWidget(m_randomMin);
        row->addWidget(new QLabel(tr("до"), m_randomRow));
        row->addWidget(m_randomMax);
        row->addStretch(1);
        modeLayout->addWidget(m_randomRow);
    }
    root->addWidget(modeBox);

    QVBoxLayout* stageLayout = nullptr;
    m_stageBox = groupBox(tr("Сценарий"), this, &stageLayout);
    {
        auto* top = new QHBoxLayout;
        top->setSpacing(8);
        top->addWidget(mutedLabel(tr("Шаблон"), m_stageBox, 66));
        m_presetCombo = new QComboBox(m_stageBox);
        m_presetCombo->addItem(tr("Свой набор"), 0);
        m_presetCombo->addItem(tr("HIIT · 40/20 × 8"), 1);
        m_presetCombo->addItem(tr("Tabata · 20/10 × 8"), 2);
        m_presetCombo->addItem(tr("Интервалы · 30/30 × 10"), 3);
        m_presetCombo->addItem(tr("Учёба · 50/10 × 4"), 4);
        m_presetCombo->addItem(tr("Кухня · 10/2"), 5);
        m_presetCombo->addItem(tr("Тренировка · 45/15 × 8"), 6);
        m_presetCombo->addItem(tr("Презентация · 15/5"), 7);
        m_presetCombo->addItem(tr("Встреча · 5/20/5"), 8);
        top->addWidget(m_presetCombo, 1);
        top->addWidget(mutedLabel(tr("Кругов"), m_stageBox, 52));
        m_cycles = new QSpinBox(m_stageBox);
        m_cycles->setRange(1, 99);
        m_cycles->setValue(std::clamp(m_timer.repeatCycles, 1, 99));
        top->addWidget(m_cycles);
        stageLayout->addLayout(top);

        m_stageList = new QListWidget(m_stageBox);
        m_stageList->setFixedHeight(146);
        m_stageList->setSelectionMode(QAbstractItemView::SingleSelection);
        m_stageList->setDragDropMode(QAbstractItemView::InternalMove);
        m_stageList->setDefaultDropAction(Qt::MoveAction);
        stageLayout->addWidget(m_stageList);

        auto* stageButtons = new QHBoxLayout;
        stageButtons->setSpacing(6);
        auto* addButton = new QPushButton(tr("Добавить…"), m_stageBox);
        auto* editButton = new QPushButton(tr("Изменить"), m_stageBox);
        auto* removeButton = new QPushButton(tr("Удалить"), m_stageBox);
        auto* upButton = new QPushButton(tr("Выше"), m_stageBox);
        auto* downButton = new QPushButton(tr("Ниже"), m_stageBox);
        stageButtons->addWidget(addButton);
        stageButtons->addWidget(editButton);
        stageButtons->addWidget(removeButton);
        stageButtons->addStretch(1);
        stageButtons->addWidget(upButton);
        stageButtons->addWidget(downButton);
        stageLayout->addLayout(stageButtons);
        m_planSummary = new QLabel(m_stageBox);
        m_planSummary->setObjectName(QStringLiteral("Muted"));
        m_planSummary->setWordWrap(true);
        stageLayout->addWidget(m_planSummary);

        connect(addButton, &QPushButton::clicked, this, &TimerDialog::addStage);
        connect(editButton, &QPushButton::clicked, this, &TimerDialog::editStage);
        connect(removeButton, &QPushButton::clicked, this, &TimerDialog::removeStage);
        connect(upButton, &QPushButton::clicked, this, [this] { moveStage(-1); });
        connect(downButton, &QPushButton::clicked, this, [this] { moveStage(1); });
        connect(m_stageList, &QListWidget::itemDoubleClicked, this,
                [this](QListWidgetItem*) { editStage(); });
        connect(m_stageList->model(), &QAbstractItemModel::rowsMoved, this,
                [this] { syncStagesFromList(); });
        connect(m_presetCombo, &QComboBox::currentIndexChanged, this,
                [this](int index) { applyPreset(m_presetCombo->itemData(index).toInt()); });
        connect(m_cycles, &QSpinBox::valueChanged, this, [this] { updatePlanSummary(); });
    }
    stageLayout->addStretch(0);
    root->addWidget(m_stageBox);

    QVBoxLayout* soundLayout = nullptr;
    auto* soundBox = groupBox(tr("Звук"), this, &soundLayout);
    m_soundEnabled = new QCheckBox(tr("Включить звук по завершении"), soundBox);
    m_soundEnabled->setChecked(m_timer.soundEnabled);
    soundLayout->addWidget(m_soundEnabled);
    {
        auto* grid = new QGridLayout;
        grid->setHorizontalSpacing(8);
        grid->setVerticalSpacing(8);
        grid->addWidget(mutedLabel(tr("Сигнал"), soundBox), 0, 0);
        m_soundCombo = new QComboBox(soundBox);
        grid->addWidget(m_soundCombo, 0, 1);
        m_browseSound = new QPushButton(tr("Обзор…"), soundBox);
        grid->addWidget(m_browseSound, 0, 2);
        m_previewButton = new ThemedButton(QString(), icons::Icon::Play, soundBox);
        m_previewButton->setToolTip(tr("Прослушать"));
        m_previewButton->setFixedSize(28, 28);
        m_previewButton->setIconSizePx(15);
        grid->addWidget(m_previewButton, 0, 3);

        grid->addWidget(mutedLabel(tr("Громкость"), soundBox), 1, 0);
        m_volume = new QSlider(Qt::Horizontal, soundBox);
        m_volume->setRange(0, 100);
        m_volume->setValue(std::clamp(m_timer.volume, 0, 100));
        grid->addWidget(m_volume, 1, 1, 1, 2);
        m_volumeValue = new QLabel(QStringLiteral("%1%").arg(m_volume->value()), soundBox);
        m_volumeValue->setFixedWidth(38);
        m_volumeValue->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        grid->addWidget(m_volumeValue, 1, 3);

        grid->addWidget(mutedLabel(tr("Параметры"), soundBox), 2, 0);
        m_loop = new QCheckBox(tr("Повторять до закрытия"), soundBox);
        m_loop->setChecked(m_timer.loopSound);
        grid->addWidget(m_loop, 2, 1, 1, 3);
        grid->setColumnStretch(1, 1);
        soundLayout->addLayout(grid);
    }
    for (const auto& sound : catalog::availableSounds()) {
        m_soundCombo->addItem(sound.name, sound.id);
    }
    if (m_timer.soundId.startsWith(QStringLiteral("file:"))
        && m_soundCombo->findData(m_timer.soundId) < 0) {
        m_soundCombo->addItem(tr("Файл: %1").arg(QFileInfo(m_timer.soundId.mid(5)).fileName()),
                              m_timer.soundId);
    }
    const int soundIndex = m_soundCombo->findData(m_timer.soundId);
    m_soundCombo->setCurrentIndex(soundIndex >= 0 ? soundIndex : 0);
    root->addWidget(soundBox);

    QVBoxLayout* behaviorLayout = nullptr;
    auto* behaviorBox = groupBox(tr("Поведение"), this, &behaviorLayout);
    m_startNow = new QCheckBox(tr("Запустить сразу после сохранения"), behaviorBox);
    m_startNow->setChecked(m_isNew);
    behaviorLayout->addWidget(m_startNow);
    m_autoRepeat = new QCheckBox(tr("Повторять автоматически после завершения"), behaviorBox);
    m_autoRepeat->setChecked(m_timer.autoRepeat);
    behaviorLayout->addWidget(m_autoRepeat);
    m_popup = new QCheckBox(tr("Показывать окно завершения"), behaviorBox);
    m_popup->setChecked(m_timer.popupEnabled);
    behaviorLayout->addWidget(m_popup);
    root->addWidget(behaviorBox);

    root->addWidget(m_summary);
    root->addStretch(1);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    auto* saveButton = buttons->button(QDialogButtonBox::Save);
    saveButton->setText(tr("Сохранить"));
    saveButton->setProperty("accent", true);
    saveButton->setDefault(true);
    saveButton->style()->unpolish(saveButton);
    saveButton->style()->polish(saveButton);
    buttons->button(QDialogButtonBox::Cancel)->setText(tr("Отмена"));
    connect(buttons, &QDialogButtonBox::accepted, this, &TimerDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &TimerDialog::reject);
    root->addWidget(buttons);

    connect(m_hours, &QSpinBox::valueChanged, this, [this] { updateSummary(); });
    connect(m_minutes, &QSpinBox::valueChanged, this, [this] { updateSummary(); });
    connect(m_seconds, &QSpinBox::valueChanged, this, [this] { updateSummary(); });
    connect(m_soundEnabled, &QCheckBox::toggled, this, [this] { updateSoundControls(); });
    connect(m_volume, &QSlider::valueChanged, this, [this](int value) {
        m_volumeValue->setText(QStringLiteral("%1%").arg(value));
    });
    connect(m_previewButton, &QPushButton::clicked, this, &TimerDialog::togglePreview);
    connect(m_browseSound, &QPushButton::clicked, this, &TimerDialog::chooseSound);
    connect(m_targetAt, &QDateTimeEdit::dateTimeChanged, this,
            [this] { updateSummary(); });
    connect(m_randomMin, &QSpinBox::valueChanged, this, [this] { updateSummary(); });
    connect(m_randomMax, &QSpinBox::valueChanged, this, [this] { updateSummary(); });

    const int modeIndex = m_mode->findData(static_cast<int>(m_timer.mode));
    m_mode->setCurrentIndex(modeIndex >= 0 ? modeIndex : 0);
    if (m_timer.targetAt.isValid()) {
        m_targetAt->setDateTime(m_timer.targetAt);
    }
    m_randomMin->setValue(std::max(1, m_timer.randomMinSec));
    m_randomMax->setValue(std::max(1, m_timer.randomMaxSec));
    refreshStageList();
    m_loading = true;
    updateModeUi();
    m_loading = false;
    updatePlanSummary();
    updateSoundControls();
    updateSummary();
    connect(m_mode, &QComboBox::currentIndexChanged, this, [this] {
        updateModeUi();
        updateSummary();
    });
    m_label->setFocus(Qt::OtherFocusReason);
}

void TimerDialog::updateModeUi()
{
    if (m_mode == nullptr) {
        return;
    }
    const auto mode = static_cast<TimerMode>(m_mode->currentData().toInt());
    const bool sequence = isSequenceMode(mode);
    const bool target = mode == TimerMode::Deadline || mode == TimerMode::Target;
    const bool random = mode == TimerMode::RandomInterval;
    if (!m_loading && !sequence && !m_timer.stages.empty()) {
        m_timer.stages.clear();
        refreshStageList();
    }
    m_stageBox->setVisible(sequence);
    m_durationRow->setVisible(!sequence && !target && !random);
    m_quickPresets->setVisible(!sequence && !target && !random);
    m_targetRow->setVisible(target);
    m_randomRow->setVisible(random);
    if (sequence && m_timer.stages.empty()) {
        int cycles = 1;
        fillModeTemplate(mode, m_timer.stages, &cycles);
        m_cycles->setValue(cycles);
        refreshStageList();
    }
    if (mode == TimerMode::Repeating && m_autoRepeat != nullptr) {
        m_autoRepeat->setChecked(true);
    }
    updatePlanSummary();
}

void TimerDialog::refreshStageList()
{
    if (m_stageList == nullptr) {
        return;
    }
    QString selectedId;
    if (auto* item = m_stageList->currentItem()) {
        selectedId = item->data(Qt::UserRole).toString();
    }
    m_stageList->clear();
    for (const auto& stage : m_timer.stages) {
        auto* item = new QListWidgetItem(stageColorIcon(stage.color), stage.name);
        item->setText(QStringLiteral("%1 · %2 · %3")
                          .arg(stage.name, durationText(stage.durationSec),
                               timerplan::stageKindName(stage.kind)));
        item->setData(Qt::UserRole, stage.id.toString());
        item->setToolTip(stage.message);
        m_stageList->addItem(item);
        if (stage.id.toString() == selectedId) {
            m_stageList->setCurrentItem(item);
        }
    }
    updatePlanSummary();
}

void TimerDialog::updatePlanSummary()
{
    if (m_planSummary == nullptr) {
        return;
    }
    if (m_timer.stages.empty()) {
        m_planSummary->setText(tr("Добавьте хотя бы один этап."));
        if (m_summary != nullptr) {
            m_summary->setText(tr("Сценарий пока пуст."));
        }
        return;
    }
    int total = 0;
    for (const auto& stage : m_timer.stages) {
        total += std::max(1, stage.durationSec);
    }
    const int cycles = m_cycles != nullptr ? std::max(1, m_cycles->value()) : 1;
    total *= cycles;
    const QDateTime end = QDateTime::currentDateTime().addSecs(total);
    const QString text =
        tr("Всего: %1 · окончание около %2 · этапов: %3 · кругов: %4")
            .arg(durationText(total), end.toString(QStringLiteral("HH:mm")),
                 QString::number(m_timer.stages.size()), QString::number(cycles));
    m_planSummary->setText(text);
    if (m_summary != nullptr) {
        m_summary->setText(text);
    }
}

int TimerDialog::selectedStageIndex() const
{
    if (m_stageList == nullptr || m_stageList->currentItem() == nullptr) {
        return -1;
    }
    const QString id = m_stageList->currentItem()->data(Qt::UserRole).toString();
    for (int index = 0; index < static_cast<int>(m_timer.stages.size()); ++index) {
        if (m_timer.stages.at(static_cast<std::size_t>(index)).id.toString() == id) {
            return index;
        }
    }
    return -1;
}

void TimerDialog::addStage()
{
    TimerStage stage;
    stage.name = tr("Этап %1").arg(m_timer.stages.size() + 1);
    stage.durationSec = 300;
    stage.color = stageColorAt(static_cast<int>(m_timer.stages.size()));
    TimerStageDialog dialog(stage, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    m_timer.stages.push_back(dialog.result());
    refreshStageList();
    if (m_stageList->count() > 0) {
        m_stageList->setCurrentRow(m_stageList->count() - 1);
    }
    updatePlanSummary();
}

void TimerDialog::editStage()
{
    const int index = selectedStageIndex();
    if (index < 0) {
        return;
    }
    TimerStage stage = m_timer.stages.at(static_cast<std::size_t>(index));
    TimerStageDialog dialog(stage, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    m_timer.stages.at(static_cast<std::size_t>(index)) = dialog.result();
    refreshStageList();
    updatePlanSummary();
}

void TimerDialog::removeStage()
{
    const int index = selectedStageIndex();
    if (index < 0) {
        return;
    }
    m_timer.stages.erase(m_timer.stages.begin() + index);
    refreshStageList();
    updatePlanSummary();
}

void TimerDialog::moveStage(int delta)
{
    const int index = selectedStageIndex();
    const int target = index + delta;
    if (index < 0 || target < 0 || target >= static_cast<int>(m_timer.stages.size())) {
        return;
    }
    std::swap(m_timer.stages.at(static_cast<std::size_t>(index)),
              m_timer.stages.at(static_cast<std::size_t>(target)));
    refreshStageList();
    m_stageList->setCurrentRow(target);
    updatePlanSummary();
}

void TimerDialog::syncStagesFromList()
{
    if (m_stageList == nullptr) {
        return;
    }
    std::vector<TimerStage> reordered;
    reordered.reserve(m_timer.stages.size());
    for (int row = 0; row < m_stageList->count(); ++row) {
        const QString id = m_stageList->item(row)->data(Qt::UserRole).toString();
        for (const auto& stage : m_timer.stages) {
            if (stage.id.toString() == id) {
                reordered.push_back(stage);
                break;
            }
        }
    }
    if (reordered.size() == m_timer.stages.size()) {
        m_timer.stages = std::move(reordered);
    }
    updatePlanSummary();
}

void TimerDialog::applyPreset(int index)
{
    if (index <= 0) {
        return;
    }
    m_timer.stages.clear();
    int cycles = 1;
    switch (index) {
    case 1:
        fillModeTemplate(TimerMode::Hiit, m_timer.stages, &cycles);
        break;
    case 2:
        fillModeTemplate(TimerMode::Tabata, m_timer.stages, &cycles);
        break;
    case 3:
        fillModeTemplate(TimerMode::Interval, m_timer.stages, &cycles);
        break;
    case 4:
        fillModeTemplate(TimerMode::Study, m_timer.stages, &cycles);
        break;
    case 5:
        fillModeTemplate(TimerMode::Kitchen, m_timer.stages, &cycles);
        break;
    case 6:
        fillModeTemplate(TimerMode::Workout, m_timer.stages, &cycles);
        break;
    case 7:
        fillModeTemplate(TimerMode::Presentation, m_timer.stages, &cycles);
        break;
    case 8:
        fillModeTemplate(TimerMode::Meeting, m_timer.stages, &cycles);
        break;
    default:
        break;
    }
    m_cycles->setValue(cycles);
    refreshStageList();
    updatePlanSummary();
}

void TimerDialog::setDuration(int hours, int minutes, int seconds)
{
    m_hours->setValue(std::clamp(hours, 0, 99));
    m_minutes->setValue(std::clamp(minutes, 0, 59));
    m_seconds->setValue(std::clamp(seconds, 0, 59));
    updateSummary();
}

int TimerDialog::durationSeconds() const
{
    return m_hours->value() * 3600 + m_minutes->value() * 60 + m_seconds->value();
}

void TimerDialog::updateSummary()
{
    if (m_mode == nullptr) {
        return;
    }
    const auto mode = static_cast<TimerMode>(m_mode->currentData().toInt());
    if (isSequenceMode(mode)) {
        updatePlanSummary();
        return;
    }
    if (mode == TimerMode::Deadline || mode == TimerMode::Target) {
        m_summary->setText(tr("Сигнал сработает в заданное время."));
        return;
    }
    if (mode == TimerMode::RandomInterval) {
        m_summary->setText(tr("Случайная длительность от %1 до %2 с.")
                               .arg(m_randomMin->value())
                               .arg(m_randomMax->value()));
        return;
    }
    const int total = durationSeconds();
    const int hours = total / 3600;
    const int minutes = (total % 3600) / 60;
    const int seconds = total % 60;
    QStringList parts;
    if (hours > 0) {
        parts.append(tr("%1 ч").arg(hours));
    }
    if (minutes > 0) {
        parts.append(tr("%1 мин").arg(minutes));
    }
    if (seconds > 0) {
        parts.append(tr("%1 с").arg(seconds));
    }
    m_summary->setText(parts.isEmpty() ? tr("Длительность не задана")
                                       : tr("Длительность: %1").arg(parts.join(QStringLiteral(" "))) );
}

void TimerDialog::updateSoundControls()
{
    const bool enabled = m_soundEnabled->isChecked();
    m_soundCombo->setEnabled(enabled);
    m_browseSound->setEnabled(enabled);
    m_previewButton->setEnabled(enabled);
    m_volume->setEnabled(enabled);
    m_volumeValue->setEnabled(enabled);
    m_loop->setEnabled(enabled);
    if (!enabled && m_previewing) {
        SoundPlayer::instance().stop();
        m_previewing = false;
        m_previewButton->setGlyph(icons::Icon::Play);
    }
}

void TimerDialog::chooseSound()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Звук таймера"), QString(),
        tr("Аудиофайлы (*.wav *.mp3 *.wma *.ogg *.flac);;Все файлы (*.*)"));
    if (path.isEmpty()) {
        return;
    }
    for (int i = m_soundCombo->count() - 1; i >= 0; --i) {
        if (m_soundCombo->itemData(i).toString().startsWith(QStringLiteral("file:"))) {
            m_soundCombo->removeItem(i);
        }
    }
    const QString id = QStringLiteral("file:") + QDir::toNativeSeparators(path);
    m_soundCombo->addItem(tr("Файл: %1").arg(QFileInfo(path).fileName()), id);
    m_soundCombo->setCurrentIndex(m_soundCombo->count() - 1);
    m_soundEnabled->setChecked(true);
}

void TimerDialog::togglePreview()
{
    if (m_previewing) {
        SoundPlayer::instance().stop();
        m_previewing = false;
        m_previewButton->setGlyph(icons::Icon::Play);
        return;
    }
    const QString id = soundId();
    if (SoundPlayer::resolveSoundPath(id).isEmpty()) {
        QMessageBox::warning(this, tr("Звук"), tr("Не удалось открыть выбранный звук."));
        return;
    }
    SoundPlayer::instance().play(id, m_volume->value(), false, 0, false);
    m_previewing = true;
    m_previewButton->setGlyph(icons::Icon::Stop);
}

QString TimerDialog::soundId() const
{
    const QString id = m_soundCombo->currentData().toString();
    return id.isEmpty() ? Settings::instance().defaultTimerSound() : id;
}

CountdownTimer TimerDialog::result() const
{
    CountdownTimer timer = m_timer;
    timer.label = m_label->text().trimmed();
    timer.mode = static_cast<TimerMode>(m_mode->currentData().toInt());
    timer.repeatCycles = std::max(1, m_cycles->value());
    timer.targetAt = (timer.mode == TimerMode::Deadline || timer.mode == TimerMode::Target)
                         ? m_targetAt->dateTime()
                         : QDateTime();
    timer.randomMinSec = m_randomMin->value();
    timer.randomMaxSec = m_randomMax->value();
    if (!isSequenceMode(timer.mode)) {
        timer.stages.clear();
    }
    timer.endAt = QDateTime();
    timer.state = TimerState::Stopped;
    timer.currentStage = 0;
    timer.completedCycles = 0;
    if (!timer.stages.empty()) {
        int total = 0;
        for (const auto& stage : timer.stages) {
            total += std::max(1, stage.durationSec);
        }
        timer.durationSec = std::max(1, total * timer.repeatCycles);
        timer.remainingMs = static_cast<qint64>(
            std::max(1, timer.stages.front().durationSec)) * 1000;
    } else if (timer.mode == TimerMode::Deadline || timer.mode == TimerMode::Target) {
        timer.durationSec = static_cast<int>(
            std::max<qint64>(1, QDateTime::currentDateTime().secsTo(timer.targetAt)));
        timer.remainingMs = static_cast<qint64>(timer.durationSec) * 1000;
    } else if (timer.mode == TimerMode::RandomInterval) {
        timer.durationSec = std::max(1, std::min(timer.randomMinSec, timer.randomMaxSec));
        timer.remainingMs = static_cast<qint64>(timer.durationSec) * 1000;
    } else {
        timer.durationSec = std::max(1, durationSeconds());
        timer.remainingMs = static_cast<qint64>(timer.durationSec) * 1000;
    }
    timer.soundEnabled = m_soundEnabled->isChecked();
    timer.soundId = soundId();
    timer.volume = m_volume->value();
    timer.loopSound = m_loop->isChecked();
    timer.popupEnabled = m_popup->isChecked();
    timer.autoRepeat = m_autoRepeat->isChecked();
    if (timer.label.isEmpty()) {
        timer.label = tr("Таймер на %1 мин").arg(std::max(1, timer.durationSec / 60));
    }
    return timer;
}

bool TimerDialog::startImmediately() const
{
    return m_startNow->isChecked();
}

void TimerDialog::accept()
{
    const auto mode = static_cast<TimerMode>(m_mode->currentData().toInt());
    if (isSequenceMode(mode)) {
        if (m_timer.stages.empty()) {
            QMessageBox::warning(this, tr("Сценарий"),
                                 tr("Добавьте хотя бы один этап."));
            return;
        }
        if (m_cycles->value() <= 0) {
            QMessageBox::warning(this, tr("Сценарий"),
                                 tr("Количество кругов должно быть больше нуля."));
            return;
        }
    } else if (mode == TimerMode::Deadline || mode == TimerMode::Target) {
        if (!m_targetAt->dateTime().isValid()
            || m_targetAt->dateTime() <= QDateTime::currentDateTime()) {
            QMessageBox::warning(this, tr("Срок"),
                                 tr("Укажите будущую дату и время."));
            return;
        }
    } else if (mode == TimerMode::RandomInterval) {
        if (m_randomMax->value() < m_randomMin->value()) {
            QMessageBox::warning(this, tr("Случайный интервал"),
                                 tr("Максимальная длительность меньше минимальной."));
            return;
        }
    } else if (durationSeconds() <= 0) {
        QMessageBox::warning(this, tr("Длительность"),
                             tr("Установите длительность больше нуля."));
        return;
    }
    if (m_previewing) {
        SoundPlayer::instance().stop();
        m_previewing = false;
    }
    QDialog::accept();
}

void TimerDialog::reject()
{
    if (m_previewing) {
        SoundPlayer::instance().stop();
        m_previewing = false;
    }
    QDialog::reject();
}

} // namespace pac::ui
