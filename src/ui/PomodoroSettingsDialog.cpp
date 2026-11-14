#include "PomodoroSettingsDialog.h"

#include "core/Catalog.h"
#include "core/Theme.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QStyle>
#include <QVBoxLayout>

#include <algorithm>

namespace pac::ui {
namespace {

[[nodiscard]] QLabel* caption(const QString& text, QWidget* parent)
{
    auto* label = new QLabel(text, parent);
    label->setObjectName(QStringLiteral("Muted"));
    return label;
}

[[nodiscard]] QGroupBox* makeGroup(const QString& title, QWidget* parent, QVBoxLayout** layout)
{
    auto* box = new QGroupBox(title, parent);
    auto* boxLayout = new QVBoxLayout(box);
    boxLayout->setContentsMargins(12, 10, 12, 12);
    boxLayout->setSpacing(8);
    *layout = boxLayout;
    return box;
}

} // namespace

PomodoroSettingsDialog::PomodoroSettingsDialog(const PomodoroSettings& settings, QWidget* parent)
    : QDialog(parent)
    , m_settings(settings)
{
    setWindowTitle(tr("Настройки помодоро"));
    setMinimumWidth(540);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(14, 14, 14, 12);
    root->setSpacing(10);

    QVBoxLayout* durationLayout = nullptr;
    auto* durationBox = makeGroup(tr("Длительности"), this, &durationLayout);
    {
        auto* grid = new QGridLayout;
        grid->setHorizontalSpacing(10);
        grid->setVerticalSpacing(7);
        int row = 0;
        const auto addSpin = [&](const QString& title, int value, int min, int max, int suffix) {
            auto* label = caption(title, durationBox);
            auto* spin = new QSpinBox(durationBox);
            spin->setRange(min, max);
            spin->setValue(value);
            spin->setSuffix(tr(" мин"));
            grid->addWidget(label, row, 0);
            grid->addWidget(spin, row, 1);
            ++row;
            Q_UNUSED(suffix)
            return spin;
        };
        m_work = addSpin(tr("Фокус"), settings.workMinutes, 1, 180, 0);
        m_shortBreak = addSpin(tr("Короткий перерыв"), settings.shortBreakMinutes, 1, 60, 0);
        m_longBreak = addSpin(tr("Длинный перерыв"), settings.longBreakMinutes, 1, 120, 0);
        m_longEvery = new QSpinBox(durationBox);
        m_longEvery->setRange(2, 12);
        m_longEvery->setValue(settings.longBreakEvery);
        m_longEvery->setSuffix(tr(" помидоров"));
        grid->addWidget(caption(tr("Длинный перерыв каждые"), durationBox), row, 0);
        grid->addWidget(m_longEvery, row, 1);
        grid->setColumnStretch(2, 1);
        durationLayout->addLayout(grid);
    }
    root->addWidget(durationBox);

    QVBoxLayout* behaviorLayout = nullptr;
    auto* behaviorBox = makeGroup(tr("Автоматизация"), this, &behaviorLayout);
    m_autoBreaks = new QCheckBox(tr("Автоматически начинать перерывы"), behaviorBox);
    m_autoBreaks->setChecked(settings.autoStartBreaks);
    m_autoWork = new QCheckBox(tr("Автоматически возвращаться к фокусу"), behaviorBox);
    m_autoWork->setChecked(settings.autoStartWork);
    behaviorLayout->addWidget(m_autoBreaks);
    behaviorLayout->addWidget(m_autoWork);
    root->addWidget(behaviorBox);

    QVBoxLayout* soundLayout = nullptr;
    auto* soundBox = makeGroup(tr("Звук и уведомления"), this, &soundLayout);
    m_soundEnabled = new QCheckBox(tr("Звук при завершении этапа"), soundBox);
    m_soundEnabled->setChecked(settings.soundEnabled);
    soundLayout->addWidget(m_soundEnabled);
    {
        auto* row = new QHBoxLayout;
        row->setSpacing(8);
        row->addWidget(caption(tr("Сигнал"), soundBox));
        m_soundCombo = new QComboBox(soundBox);
        for (const auto& sound : catalog::availableSounds()) {
            m_soundCombo->addItem(sound.name, sound.id);
        }
        const int index = m_soundCombo->findData(settings.soundId);
        m_soundCombo->setCurrentIndex(index >= 0 ? index : 0);
        row->addWidget(m_soundCombo, 1);
        row->addWidget(caption(tr("Громкость"), soundBox));
        m_volume = new QSpinBox(soundBox);
        m_volume->setRange(0, 100);
        m_volume->setValue(settings.volume);
        m_volume->setSuffix(QStringLiteral("%"));
        row->addWidget(m_volume);
        soundLayout->addLayout(row);
    }
    m_notifications = new QCheckBox(tr("Показывать окно завершения этапа"), soundBox);
    m_notifications->setChecked(settings.notifications);
    soundLayout->addWidget(m_notifications);
    connect(m_soundEnabled, &QCheckBox::toggled, soundBox, [this, soundBox](bool enabled) {
        m_soundCombo->setEnabled(enabled);
        m_volume->setEnabled(enabled);
        Q_UNUSED(soundBox)
    });
    m_soundCombo->setEnabled(settings.soundEnabled);
    m_volume->setEnabled(settings.soundEnabled);
    root->addWidget(soundBox);

    QVBoxLayout* goalLayout = nullptr;
    auto* goalBox = makeGroup(tr("Цель"), this, &goalLayout);
    {
        auto* row = new QHBoxLayout;
        row->setSpacing(8);
        row->addWidget(caption(tr("Помидоров в день"), goalBox));
        m_dailyGoal = new QSpinBox(goalBox);
        m_dailyGoal->setRange(1, 30);
        m_dailyGoal->setValue(settings.dailyGoal);
        row->addWidget(m_dailyGoal);
        row->addStretch(1);
        goalLayout->addLayout(row);
    }
    m_resetStats = new QCheckBox(tr("Сбросить статистику сегодняшнего дня"), goalBox);
    m_resetStats->setToolTip(tr("Количество помидоров и минуты фокуса будут обнулены"));
    goalLayout->addWidget(m_resetStats);
    root->addWidget(goalBox);

    root->addStretch(1);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    auto* saveButton = buttons->button(QDialogButtonBox::Save);
    saveButton->setText(tr("Сохранить"));
    saveButton->setProperty("accent", true);
    saveButton->setDefault(true);
    saveButton->style()->unpolish(saveButton);
    saveButton->style()->polish(saveButton);
    buttons->button(QDialogButtonBox::Cancel)->setText(tr("Отмена"));
    connect(buttons, &QDialogButtonBox::accepted, this, &PomodoroSettingsDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(buttons);
}

PomodoroSettings PomodoroSettingsDialog::result() const
{
    return m_settings;
}

bool PomodoroSettingsDialog::resetStatsRequested() const
{
    return m_resetStats->isChecked();
}

void PomodoroSettingsDialog::accept()
{
    m_settings.workMinutes = m_work->value();
    m_settings.shortBreakMinutes = m_shortBreak->value();
    m_settings.longBreakMinutes = m_longBreak->value();
    m_settings.longBreakEvery = m_longEvery->value();
    m_settings.autoStartBreaks = m_autoBreaks->isChecked();
    m_settings.autoStartWork = m_autoWork->isChecked();
    m_settings.soundEnabled = m_soundEnabled->isChecked();
    m_settings.soundId = m_soundCombo->currentData().toString();
    if (m_settings.soundId.isEmpty()) {
        m_settings.soundId = QStringLiteral("builtin:gentle");
    }
    m_settings.volume = m_volume->value();
    m_settings.notifications = m_notifications->isChecked();
    m_settings.dailyGoal = m_dailyGoal->value();
    QDialog::accept();
}

} // namespace pac::ui
