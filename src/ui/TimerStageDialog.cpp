#include "TimerStageDialog.h"

#include "UiKit.h"
#include "core/Catalog.h"
#include "core/SoundPlayer.h"
#include "core/Theme.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QSpinBox>
#include <QVBoxLayout>

#include <algorithm>

namespace pac::ui {
namespace {

[[nodiscard]] QLabel* caption(const QString& text, QWidget* parent)
{
    auto* label = new QLabel(text, parent);
    label->setObjectName(QStringLiteral("Muted"));
    label->setFixedWidth(110);
    return label;
}

[[nodiscard]] QIcon colorIcon(const QString& color)
{
    QPixmap pixmap(16, 16);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(QColor(0, 0, 0, 60), 1.0));
    painter.setBrush(QColor(color));
    painter.drawEllipse(QRectF(1.5, 1.5, 13.0, 13.0));
    return QIcon(pixmap);
}

} // namespace

TimerStageDialog::TimerStageDialog(const TimerStage& stage, QWidget* parent)
    : QDialog(parent)
    , m_stage(stage)
{
    setWindowTitle(tr("Этап таймера"));
    setMinimumWidth(520);
    buildUi();
}

TimerStageDialog::~TimerStageDialog()
{
    if (m_previewing) {
        SoundPlayer::instance().stop();
    }
}

TimerStage TimerStageDialog::result() const
{
    return m_stage;
}

void TimerStageDialog::buildUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 14);
    root->setSpacing(10);

    auto* title = new QLabel(tr("Параметры этапа"), this);
    QFont titleFont = title->font();
    titleFont.setPointSizeF(14.0);
    titleFont.setWeight(QFont::DemiBold);
    title->setFont(titleFont);
    root->addWidget(title);

    auto* form = new QGridLayout;
    form->setHorizontalSpacing(10);
    form->setVerticalSpacing(8);

    m_name = new QLineEdit(this);
    m_name->setPlaceholderText(tr("Например: Работа, Отдых, Разминка"));
    m_name->setText(m_stage.name);
    form->addWidget(caption(tr("Название"), this), 0, 0);
    form->addWidget(m_name, 0, 1);

    auto* duration = new QHBoxLayout;
    duration->setSpacing(5);
    m_hours = new QSpinBox(this);
    m_hours->setRange(0, 99);
    m_hours->setSuffix(tr(" ч"));
    m_minutes = new QSpinBox(this);
    m_minutes->setRange(0, 59);
    m_minutes->setSuffix(tr(" мин"));
    m_seconds = new QSpinBox(this);
    m_seconds->setRange(0, 59);
    m_seconds->setSuffix(tr(" с"));
    duration->addWidget(m_hours);
    duration->addWidget(m_minutes);
    duration->addWidget(m_seconds);
    duration->addStretch(1);
    form->addWidget(caption(tr("Длительность"), this), 1, 0);
    form->addLayout(duration, 1, 1);
    setDuration(m_stage.durationSec);

    m_kind = new QComboBox(this);
    m_kind->addItem(tr("Подготовка"), static_cast<int>(TimerStageKind::Prepare));
    m_kind->addItem(tr("Работа"), static_cast<int>(TimerStageKind::Work));
    m_kind->addItem(tr("Отдых"), static_cast<int>(TimerStageKind::Rest));
    m_kind->addItem(tr("Восстановление"), static_cast<int>(TimerStageKind::Recovery));
    m_kind->addItem(tr("Другое"), static_cast<int>(TimerStageKind::Custom));
    const int kindIndex = m_kind->findData(static_cast<int>(m_stage.kind));
    m_kind->setCurrentIndex(kindIndex >= 0 ? kindIndex : 0);
    form->addWidget(caption(tr("Тип"), this), 2, 0);
    form->addWidget(m_kind, 2, 1);

    m_color = new QComboBox(this);
    for (const QString& color : catalog::alarmColors()) {
        m_color->addItem(colorIcon(color), color, color);
    }
    int colorIndex = m_color->findData(m_stage.color);
    if (colorIndex < 0) {
        colorIndex = 0;
    }
    m_color->setCurrentIndex(colorIndex);
    form->addWidget(caption(tr("Цвет"), this), 3, 0);
    form->addWidget(m_color, 3, 1);

    m_sound = new QComboBox(this);
    m_sound->addItem(tr("Без отдельного звука"), QString());
    for (const auto& sound : catalog::availableSounds()) {
        m_sound->addItem(sound.name, sound.id);
    }
    if (!m_stage.soundId.isEmpty() && m_sound->findData(m_stage.soundId) < 0) {
        m_sound->addItem(catalog::soundName(m_stage.soundId), m_stage.soundId);
    }
    const int soundIndex = m_sound->findData(m_stage.soundId);
    m_sound->setCurrentIndex(soundIndex >= 0 ? soundIndex : 0);
    auto* soundRow = new QHBoxLayout;
    soundRow->setSpacing(8);
    soundRow->addWidget(m_sound, 1);
    m_preview = new ThemedButton(tr("Прослушать"), icons::Icon::Play, this);
    m_preview->setFixedHeight(28);
    soundRow->addWidget(m_preview);
    form->addWidget(caption(tr("Сигнал"), this), 4, 0);
    form->addLayout(soundRow, 4, 1);

    m_message = new QLineEdit(this);
    m_message->setPlaceholderText(tr("Сообщение при переходе к следующему этапу"));
    m_message->setText(m_stage.message);
    form->addWidget(caption(tr("Сообщение"), this), 5, 0);
    form->addWidget(m_message, 5, 1);

    form->setColumnStretch(1, 1);
    root->addLayout(form);

    m_previewHint = new QLabel(this);
    m_previewHint->setObjectName(QStringLiteral("Faint"));
    m_previewHint->setWordWrap(true);
    root->addWidget(m_previewHint);
    root->addStretch(1);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Save)->setText(tr("Сохранить"));
    buttons->button(QDialogButtonBox::Save)->setProperty("accent", true);
    buttons->button(QDialogButtonBox::Cancel)->setText(tr("Отмена"));
    root->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &TimerStageDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_preview, &QPushButton::clicked, this, &TimerStageDialog::togglePreview);
}

void TimerStageDialog::setDuration(int seconds)
{
    m_hours->setValue(std::clamp(seconds / 3600, 0, 99));
    m_minutes->setValue(std::clamp((seconds % 3600) / 60, 0, 59));
    m_seconds->setValue(std::clamp(seconds % 60, 0, 59));
}

int TimerStageDialog::durationSeconds() const
{
    return m_hours->value() * 3600 + m_minutes->value() * 60 + m_seconds->value();
}

void TimerStageDialog::accept()
{
    if (durationSeconds() <= 0) {
        QMessageBox::warning(this, tr("Этап"), tr("Длительность этапа должна быть больше нуля."));
        return;
    }
    m_stage.name = m_name->text().trimmed();
    if (m_stage.name.isEmpty()) {
        m_stage.name = tr("Этап");
    }
    m_stage.durationSec = durationSeconds();
    m_stage.kind = static_cast<TimerStageKind>(m_kind->currentData().toInt());
    m_stage.color = m_color->currentData().toString();
    m_stage.soundId = m_sound->currentData().toString();
    m_stage.message = m_message->text().trimmed();
    if (m_previewing) {
        SoundPlayer::instance().stop();
    }
    QDialog::accept();
}

void TimerStageDialog::togglePreview()
{
    if (m_previewing) {
        SoundPlayer::instance().stop();
        m_previewing = false;
        m_preview->setGlyph(icons::Icon::Play);
        return;
    }
    const QString id = m_sound->currentData().toString();
    if (id.isEmpty()) {
        m_previewHint->setText(tr("У этапа нет отдельного сигнала — будет использован звук таймера."));
        return;
    }
    if (SoundPlayer::resolveSoundPath(id).isEmpty()) {
        QMessageBox::warning(this, tr("Звук"), tr("Не удалось найти выбранный звук."));
        return;
    }
    SoundPlayer::instance().play(id, 70, false, 0, false);
    m_previewing = true;
    m_preview->setGlyph(icons::Icon::Stop);
    m_previewHint->setText(tr("Предпрослушивание сигнала этапа"));
}

} // namespace pac::ui
