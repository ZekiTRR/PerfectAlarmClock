#include "AlarmScheduleDialog.h"

#include "UiKit.h"
#include "core/Catalog.h"
#include "core/Settings.h"
#include "core/Theme.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QDateTimeEdit>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTimeEdit>
#include <QVBoxLayout>

namespace pac::ui {
namespace {

[[nodiscard]] QLabel* label(const QString& text, QWidget* parent)
{
    auto* widget = new QLabel(text, parent);
    widget->setObjectName(QStringLiteral("Muted"));
    widget->setFixedWidth(122);
    return widget;
}

[[nodiscard]] QHBoxLayout* row(const QString& text, QWidget* field, QWidget* parent)
{
    auto* layout = new QHBoxLayout;
    layout->setSpacing(8);
    layout->addWidget(label(text, parent));
    layout->addWidget(field, 1);
    return layout;
}

} // namespace

AlarmScheduleDialog::AlarmScheduleDialog(const AlarmSchedule& schedule, QWidget* parent)
    : QDialog(parent)
    , m_schedule(schedule)
{
    setWindowTitle(tr("Дополнительное расписание"));
    setMinimumWidth(540);
    buildUi();
}

AlarmSchedule AlarmScheduleDialog::result() const
{
    return m_schedule;
}

void AlarmScheduleDialog::buildUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 14);
    root->setSpacing(10);

    auto* header = makeCard(this);
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(16, 12, 16, 12);
    headerLayout->setSpacing(12);
    m_time = new QTimeEdit(header);
    m_time->setObjectName(QStringLiteral("HeroTime"));
    m_time->setDisplayFormat(Settings::instance().use24h() ? QStringLiteral("HH:mm")
                                                           : QStringLiteral("h:mm AP"));
    m_time->setTime(m_schedule.time);
    m_time->setFixedWidth(150);
    headerLayout->addWidget(m_time);
    auto* textColumn = new QVBoxLayout;
    textColumn->setSpacing(2);
    auto* title = new QLabel(tr("Расписание будильника"), header);
    QFont titleFont = title->font();
    titleFont.setPointSizeF(11.5);
    titleFont.setWeight(QFont::DemiBold);
    title->setFont(titleFont);
    textColumn->addWidget(title);
    auto* subtitle = new QLabel(tr("Часовой пояс и праздники задаются для всего будильника."),
                                header);
    subtitle->setObjectName(QStringLiteral("Faint"));
    textColumn->addWidget(subtitle);
    headerLayout->addLayout(textColumn, 1);
    m_enabled = new QCheckBox(tr("Активно"), header);
    m_enabled->setChecked(m_schedule.enabled);
    headerLayout->addWidget(m_enabled);
    root->addWidget(header);

    QVBoxLayout* formLayout = nullptr;
    auto* form = makeCard(this);
    auto* formBox = new QVBoxLayout(form);
    formBox->setContentsMargins(16, 14, 16, 14);
    formBox->setSpacing(8);
    formLayout = formBox;

    m_repeat = new QComboBox(form);
    m_repeat->addItem(tr("Один раз"), static_cast<int>(RepeatKind::Once));
    m_repeat->addItem(tr("Каждый день"), static_cast<int>(RepeatKind::Daily));
    m_repeat->addItem(tr("По будням"), static_cast<int>(RepeatKind::Weekdays));
    m_repeat->addItem(tr("По выходным"), static_cast<int>(RepeatKind::Weekends));
    m_repeat->addItem(tr("По дням недели"), static_cast<int>(RepeatKind::Weekly));
    m_repeat->addItem(tr("Ежемесячно"), static_cast<int>(RepeatKind::Monthly));
    m_repeat->addItem(tr("N-й день недели месяца"),
                      static_cast<int>(RepeatKind::MonthlyNthWeekday));
    m_repeat->addItem(tr("Последний рабочий день месяца"),
                      static_cast<int>(RepeatKind::LastWeekdayOfMonth));
    m_repeat->addItem(tr("Ежегодно"), static_cast<int>(RepeatKind::Yearly));
    m_repeat->addItem(tr("С интервалом"), static_cast<int>(RepeatKind::Interval));
    m_repeat->addItem(tr("Каждые N дней"), static_cast<int>(RepeatKind::EveryNDays));
    m_repeat->addItem(tr("Каждые N недель"), static_cast<int>(RepeatKind::EveryNWeeks));
    formLayout->addLayout(row(tr("Повтор"), m_repeat, form));

    m_anchorRow = new QWidget(form);
    auto* anchorLayout = new QHBoxLayout(m_anchorRow);
    anchorLayout->setContentsMargins(0, 0, 0, 0);
    anchorLayout->setSpacing(8);
    m_anchorLabel = label(tr("Начало"), m_anchorRow);
    anchorLayout->addWidget(m_anchorLabel);
    m_anchor = new QDateEdit(m_anchorRow);
    m_anchor->setCalendarPopup(true);
    m_anchor->setDisplayFormat(QStringLiteral("dd.MM.yyyy"));
    m_anchor->setDate(m_schedule.anchorDate.isValid() ? m_schedule.anchorDate
                                                       : QDate::currentDate());
    anchorLayout->addWidget(m_anchor, 0);
    anchorLayout->addStretch(1);
    formLayout->addWidget(m_anchorRow);

    m_weeklyPanel = new QWidget(form);
    auto* weeklyLayout = new QHBoxLayout(m_weeklyPanel);
    weeklyLayout->setContentsMargins(0, 0, 0, 0);
    weeklyLayout->setSpacing(3);
    weeklyLayout->addWidget(label(tr("Дни"), m_weeklyPanel));
    for (int day = Qt::Monday; day <= Qt::Sunday; ++day) {
        auto* button = new QPushButton(catalog::dayShort(static_cast<Qt::DayOfWeek>(day)),
                                       m_weeklyPanel);
        button->setCheckable(true);
        button->setFixedSize(36, 26);
        button->setStyleSheet(QStringLiteral("padding: 2px 3px;"));
        if (m_schedule.days.contains(static_cast<Qt::DayOfWeek>(day))) {
            button->setChecked(true);
        }
        m_dayButtons.append(button);
        weeklyLayout->addWidget(button);
    }
    weeklyLayout->addStretch(1);
    formLayout->addWidget(m_weeklyPanel);

    m_everyDaysPanel = new QWidget(form);
    auto* everyDaysLayout = new QHBoxLayout(m_everyDaysPanel);
    everyDaysLayout->setContentsMargins(0, 0, 0, 0);
    everyDaysLayout->setSpacing(8);
    everyDaysLayout->addWidget(label(tr("Интервал"), m_everyDaysPanel));
    m_everyDays = new QSpinBox(m_everyDaysPanel);
    m_everyDays->setRange(1, 365);
    m_everyDays->setValue(std::max(1, m_schedule.intervalDays));
    m_everyDays->setSuffix(tr(" дн."));
    everyDaysLayout->addWidget(m_everyDays);
    everyDaysLayout->addStretch(1);
    formLayout->addWidget(m_everyDaysPanel);

    m_everyWeeksPanel = new QWidget(form);
    auto* everyWeeksLayout = new QHBoxLayout(m_everyWeeksPanel);
    everyWeeksLayout->setContentsMargins(0, 0, 0, 0);
    everyWeeksLayout->setSpacing(8);
    everyWeeksLayout->addWidget(label(tr("Интервал"), m_everyWeeksPanel));
    m_everyWeeks = new QSpinBox(m_everyWeeksPanel);
    m_everyWeeks->setRange(1, 52);
    m_everyWeeks->setValue(std::max(1, m_schedule.intervalWeeks));
    m_everyWeeks->setSuffix(tr(" нед."));
    everyWeeksLayout->addWidget(m_everyWeeks);
    everyWeeksLayout->addStretch(1);
    formLayout->addWidget(m_everyWeeksPanel);

    m_nthPanel = new QWidget(form);
    auto* nthLayout = new QHBoxLayout(m_nthPanel);
    nthLayout->setContentsMargins(0, 0, 0, 0);
    nthLayout->setSpacing(8);
    nthLayout->addWidget(label(tr("Когда"), m_nthPanel));
    m_nthWeek = new QComboBox(m_nthPanel);
    m_nthWeek->addItem(tr("Первый"), 1);
    m_nthWeek->addItem(tr("Второй"), 2);
    m_nthWeek->addItem(tr("Третий"), 3);
    m_nthWeek->addItem(tr("Четвёртый"), 4);
    m_nthWeek->addItem(tr("Последний"), -1);
    int weekIndex = m_nthWeek->findData(m_schedule.monthWeek == 0 ? 1 : m_schedule.monthWeek);
    m_nthWeek->setCurrentIndex(weekIndex >= 0 ? weekIndex : 0);
    nthLayout->addWidget(m_nthWeek);
    m_nthWeekday = new QComboBox(m_nthPanel);
    for (int day = Qt::Monday; day <= Qt::Sunday; ++day) {
        m_nthWeekday->addItem(catalog::dayLong(static_cast<Qt::DayOfWeek>(day)), day);
    }
    if (!m_schedule.days.isEmpty()) {
        const int index = m_nthWeekday->findData(static_cast<int>(m_schedule.days.first()));
        if (index >= 0) {
            m_nthWeekday->setCurrentIndex(index);
        }
    }
    nthLayout->addWidget(m_nthWeekday, 1);
    formLayout->addWidget(m_nthPanel);

    m_monthlyPanel = new QWidget(form);
    auto* monthlyLayout = new QHBoxLayout(m_monthlyPanel);
    monthlyLayout->setContentsMargins(0, 0, 0, 0);
    monthlyLayout->setSpacing(8);
    monthlyLayout->addWidget(label(tr("День"), m_monthlyPanel));
    m_monthDay = new QSpinBox(m_monthlyPanel);
    m_monthDay->setRange(1, 31);
    m_monthDay->setValue(std::clamp(m_schedule.monthDay, 1, 31));
    m_monthDay->setSuffix(tr(" число"));
    monthlyLayout->addWidget(m_monthDay);
    auto* monthlyHint = new QLabel(tr("31 = последний день месяца"), m_monthlyPanel);
    monthlyHint->setObjectName(QStringLiteral("Faint"));
    monthlyLayout->addWidget(monthlyHint);
    monthlyLayout->addStretch(1);
    formLayout->addWidget(m_monthlyPanel);

    m_intervalPanel = new QWidget(form);
    auto* intervalLayout = new QHBoxLayout(m_intervalPanel);
    intervalLayout->setContentsMargins(0, 0, 0, 0);
    intervalLayout->setSpacing(8);
    intervalLayout->addWidget(label(tr("Интервал"), m_intervalPanel));
    m_intervalMinutes = new QSpinBox(m_intervalPanel);
    m_intervalMinutes->setRange(1, 1440);
    m_intervalMinutes->setValue(std::clamp(m_schedule.intervalMinutes, 1, 1440));
    m_intervalMinutes->setSuffix(tr(" мин"));
    intervalLayout->addWidget(m_intervalMinutes);
    auto* fromCaption = new QLabel(tr("с"), m_intervalPanel);
    fromCaption->setObjectName(QStringLiteral("Faint"));
    intervalLayout->addWidget(fromCaption);
    m_intervalFrom = new QTimeEdit(m_intervalPanel);
    m_intervalFrom->setDisplayFormat(Settings::instance().use24h() ? QStringLiteral("HH:mm")
                                                                   : QStringLiteral("h:mm AP"));
    m_intervalFrom->setTime(m_schedule.activeFrom);
    intervalLayout->addWidget(m_intervalFrom);
    auto* toCaption = new QLabel(tr("до"), m_intervalPanel);
    toCaption->setObjectName(QStringLiteral("Faint"));
    intervalLayout->addWidget(toCaption);
    m_intervalTo = new QTimeEdit(m_intervalPanel);
    m_intervalTo->setDisplayFormat(Settings::instance().use24h() ? QStringLiteral("HH:mm")
                                                                 : QStringLiteral("h:mm AP"));
    m_intervalTo->setTime(m_schedule.activeTo);
    intervalLayout->addWidget(m_intervalTo);
    intervalLayout->addStretch(1);
    formLayout->addWidget(m_intervalPanel);

    m_repeatHint = new QLabel(form);
    m_repeatHint->setObjectName(QStringLiteral("Faint"));
    m_repeatHint->setWordWrap(true);
    formLayout->addWidget(m_repeatHint);
    root->addWidget(form);

    auto* footer = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    footer->button(QDialogButtonBox::Save)->setText(tr("Сохранить"));
    footer->button(QDialogButtonBox::Save)->setProperty("accent", true);
    footer->button(QDialogButtonBox::Cancel)->setText(tr("Отмена"));
    root->addWidget(footer);
    connect(footer, &QDialogButtonBox::accepted, this, &AlarmScheduleDialog::accept);
    connect(footer, &QDialogButtonBox::rejected, this, &AlarmScheduleDialog::reject);

    const int repeatIndex = m_repeat->findData(static_cast<int>(m_schedule.repeat));
    m_repeat->setCurrentIndex(repeatIndex >= 0 ? repeatIndex : 0);
    connect(m_repeat, &QComboBox::currentIndexChanged, this,
            [this] { updatePanels(); });
    updatePanels();
    m_time->setSelectedSection(QDateTimeEdit::NoSection);
    m_repeat->setFocus(Qt::OtherFocusReason);
}

void AlarmScheduleDialog::updatePanels()
{
    const auto repeat = static_cast<RepeatKind>(m_repeat->currentData().toInt());
    const bool weekly = repeat == RepeatKind::Weekly || repeat == RepeatKind::EveryNWeeks;
    m_weeklyPanel->setVisible(weekly);
    m_everyDaysPanel->setVisible(repeat == RepeatKind::EveryNDays);
    m_everyWeeksPanel->setVisible(repeat == RepeatKind::EveryNWeeks);
    m_nthPanel->setVisible(repeat == RepeatKind::MonthlyNthWeekday);
    m_monthlyPanel->setVisible(repeat == RepeatKind::Monthly);
    m_intervalPanel->setVisible(repeat == RepeatKind::Interval);
    const bool anchor = repeat == RepeatKind::Once || repeat == RepeatKind::Yearly
                        || repeat == RepeatKind::EveryNDays
                        || repeat == RepeatKind::EveryNWeeks;
    m_anchorRow->setVisible(anchor);

    switch (repeat) {
    case RepeatKind::Once:
        m_anchorLabel->setText(tr("Дата"));
        break;
    case RepeatKind::Yearly:
        m_anchorLabel->setText(tr("Дата в году"));
        break;
    case RepeatKind::EveryNDays:
    case RepeatKind::EveryNWeeks:
        m_anchorLabel->setText(tr("Начало серии"));
        break;
    default:
        break;
    }

    QString hint;
    switch (repeat) {
    case RepeatKind::Weekends:
        hint = tr("Срабатывает в субботу и воскресенье.");
        break;
    case RepeatKind::MonthlyNthWeekday:
        hint = tr("Например: каждый второй вторник месяца.");
        break;
    case RepeatKind::LastWeekdayOfMonth:
        hint = tr("Последний понедельник–пятница месяца, кроме праздников.");
        break;
    case RepeatKind::EveryNDays:
        hint = tr("Серия начинается с указанной даты.");
        break;
    case RepeatKind::EveryNWeeks:
        hint = tr("Дни недели выбираются ниже; без выбора используется день начала.");
        break;
    case RepeatKind::Interval:
        hint = tr("Сигналы повторяются внутри указанного окна времени.");
        break;
    case RepeatKind::Weekly:
        hint = tr("Можно выбрать несколько дней недели.");
        break;
    default:
        break;
    }
    m_repeatHint->setText(hint);
    m_repeatHint->setVisible(!hint.isEmpty());
}

AlarmSchedule AlarmScheduleDialog::collect() const
{
    AlarmSchedule schedule = m_schedule;
    schedule.time = m_time->time();
    schedule.enabled = m_enabled->isChecked();
    schedule.repeat = static_cast<RepeatKind>(m_repeat->currentData().toInt());
    schedule.anchorDate = m_anchor->date();
    schedule.intervalDays = m_everyDays->value();
    schedule.intervalWeeks = m_everyWeeks->value();
    schedule.intervalMinutes = m_intervalMinutes->value();
    schedule.activeFrom = m_intervalFrom->time();
    schedule.activeTo = m_intervalTo->time();
    schedule.monthDay = m_monthDay->value();
    schedule.monthWeek = m_nthWeek->currentData().toInt();
    schedule.days.clear();
    if (schedule.repeat == RepeatKind::Weekly || schedule.repeat == RepeatKind::EveryNWeeks) {
        for (int index = 0; index < m_dayButtons.size(); ++index) {
            if (m_dayButtons.at(index)->isChecked()) {
                schedule.days.append(static_cast<Qt::DayOfWeek>(Qt::Monday + index));
            }
        }
    } else if (schedule.repeat == RepeatKind::MonthlyNthWeekday) {
        schedule.days.append(
            static_cast<Qt::DayOfWeek>(m_nthWeekday->currentData().toInt()));
    }
    return schedule;
}

void AlarmScheduleDialog::accept()
{
    const AlarmSchedule schedule = collect();
    if (schedule.repeat == RepeatKind::Weekly && schedule.days.isEmpty()) {
        QMessageBox::warning(this, tr("Расписание"),
                             tr("Выберите хотя бы один день недели."));
        return;
    }
    if (schedule.repeat == RepeatKind::Once) {
        const QDateTime candidate(schedule.anchorDate, schedule.time);
        if (schedule.enabled && candidate <= QDateTime::currentDateTime()) {
            QMessageBox::warning(this, tr("Расписание"),
                                 tr("Однократное расписание должно быть в будущем."));
            return;
        }
    }
    m_schedule = schedule;
    QDialog::accept();
}

} // namespace pac::ui
