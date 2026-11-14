#include "AlarmDialog.h"

#include "AlarmScheduleDialog.h"
#include "UiKit.h"
#include "core/Catalog.h"
#include "core/Schedule.h"
#include "core/Settings.h"
#include "core/SoundPlayer.h"
#include "core/Theme.h"

#include <QAbstractItemView>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QDir>
#include <QDialogButtonBox>
#include <QEnterEvent>
#include <QFileDialog>
#include <QGuiApplication>
#include <QScreen>
#include <QFileInfo>
#include <QFont>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QRegularExpression>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QSlider>
#include <QSpinBox>
#include <QStyle>
#include <QTimer>
#include <QTimeEdit>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>

namespace pac::ui {
namespace {

[[nodiscard]] QLabel* caption(const QString& text, QWidget* parent, int width = 104)
{
    auto* label = new QLabel(text, parent);
    label->setObjectName(QStringLiteral("Muted"));
    label->setFixedWidth(width);
    label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    return label;
}

[[nodiscard]] QGroupBox* makeGroup(const QString& title, QWidget* parent, QVBoxLayout** layout)
{
    auto* group = new QGroupBox(title, parent);
    group->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    auto* groupLayout = new QVBoxLayout(group);
    groupLayout->setContentsMargins(12, 10, 12, 12);
    groupLayout->setSpacing(8);
    *layout = groupLayout;
    return group;
}

class EmojiChoiceButton final : public QPushButton {
public:
    EmojiChoiceButton(const QString& emoji, QWidget* parent = nullptr)
        : QPushButton(parent)
        , m_emoji(emoji)
    {
        setCheckable(true);
        setFixedSize(26, 26);
        setCursor(Qt::PointingHandCursor);
        setProperty("emoji", emoji);
        setToolTip(emoji);
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event)
        const ThemeColors& colors = ThemeManager::instance().current();
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const QRectF rect = QRectF(this->rect()).adjusted(1.0, 1.0, -1.0, -1.0);
        if (isChecked()) {
            painter.setPen(QPen(colors.accent, 1.4));
            painter.setBrush(tinted(colors.accent, 0.16));
            painter.drawRoundedRect(rect, 5.0, 5.0);
        } else if (underMouse()) {
            painter.setPen(QPen(colors.border, 1.0));
            painter.setBrush(colors.hoverRow);
            painter.drawRoundedRect(rect, 5.0, 5.0);
        }
        painter.drawPixmap(QRectF(4.0, 4.0, width() - 8.0, height() - 8.0).toRect(),
                           emojiPixmap(m_emoji, 18));
    }

    void enterEvent(QEnterEvent* event) override
    {
        update();
        QPushButton::enterEvent(event);
    }

    void leaveEvent(QEvent* event) override
    {
        update();
        QPushButton::leaveEvent(event);
    }

private:
    QString m_emoji;
};

class ColorChoiceButton final : public QPushButton {
public:
    ColorChoiceButton(const QString& color, QWidget* parent = nullptr)
        : QPushButton(parent)
        , m_color(color)
    {
        setCheckable(true);
        setFixedSize(24, 24);
        setCursor(Qt::PointingHandCursor);
        setProperty("swatch", color);
        setToolTip(color);
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event)
        const ThemeColors& colors = ThemeManager::instance().current();
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const QRectF rect = QRectF(this->rect()).adjusted(1.5, 1.5, -1.5, -1.5);
        if (isChecked()) {
            painter.setPen(QPen(colors.text, 1.6));
            painter.setBrush(Qt::NoBrush);
            painter.drawRoundedRect(rect, 5.0, 5.0);
        } else if (underMouse()) {
            painter.setPen(QPen(colors.border, 1.0));
            painter.setBrush(colors.hoverRow);
            painter.drawRoundedRect(rect, 5.0, 5.0);
        }
        painter.setPen(QPen(QColor(0, 0, 0, 60), 1.0));
        painter.setBrush(QColor(m_color));
        painter.drawEllipse(rect.center(), 8.0, 8.0);
    }

    void enterEvent(QEnterEvent* event) override
    {
        update();
        QPushButton::enterEvent(event);
    }

    void leaveEvent(QEvent* event) override
    {
        update();
        QPushButton::leaveEvent(event);
    }

private:
    QString m_color;
};

void populateTimeZones(QComboBox* combo)
{
    combo->addItem(QObject::tr("Местное время"), QString());
    combo->addItem(QStringLiteral("UTC"), QStringLiteral("UTC"));
    for (const auto& city : catalog::cities()) {
        combo->addItem(QStringLiteral("%1 · %2").arg(city.city, city.country), city.timeZoneId);
    }
    combo->setMaxVisibleItems(18);
    combo->setMinimumWidth(260);
}

[[nodiscard]] QString exceptionsToString(const QList<QDate>& dates)
{
    QStringList parts;
    for (const QDate& date : dates) {
        if (date.isValid()) {
            parts.append(date.toString(QStringLiteral("dd.MM.yyyy")));
        }
    }
    return parts.join(QStringLiteral(", "));
}

[[nodiscard]] QList<QDate> exceptionsFromString(const QString& text, QStringList* invalid)
{
    QList<QDate> dates;
    const QStringList parts = text.split(QRegularExpression(QStringLiteral("[,;\\s]+")),
                                         Qt::SkipEmptyParts);
    for (const QString& part : parts) {
        QDate date = QDate::fromString(part, QStringLiteral("dd.MM.yyyy"));
        if (!date.isValid()) {
            date = QDate::fromString(part, Qt::ISODate);
        }
        if (!date.isValid()) {
            if (invalid != nullptr) {
                invalid->append(part);
            }
            continue;
        }
        if (!dates.contains(date)) {
            dates.append(date);
        }
    }
    return dates;
}

[[nodiscard]] QString scheduleSummary(const Alarm& alarm, const AlarmSchedule& schedule)
{
    Alarm temp = alarm;
    temp.time = schedule.time;
    temp.repeat = schedule.repeat;
    temp.days = schedule.days;
    temp.monthDay = schedule.monthDay;
    temp.monthWeek = schedule.monthWeek;
    temp.intervalDays = schedule.intervalDays;
    temp.intervalWeeks = schedule.intervalWeeks;
    temp.intervalMinutes = schedule.intervalMinutes;
    temp.activeFrom = schedule.activeFrom;
    temp.activeTo = schedule.activeTo;
    temp.onceDate = schedule.anchorDate;
    temp.seriesStart = schedule.anchorDate;
    temp.extraSchedules.clear();
    temp.exceptions.clear();
    temp.timeZoneId.clear();
    temp.holidayPolicy = HolidayPolicy::Ignore;
    temp.activeFromDate = QDate();
    temp.activeToDate = QDate();
    return schedule::repeatSummary(temp);
}

} // namespace

AlarmDialog::AlarmDialog(const Alarm& alarm, bool isNew, const QStringList& groups,
                         QWidget* parent)
    : QDialog(parent)
    , m_alarm(alarm)
    , m_original(alarm)
    , m_isNew(isNew)
    , m_originalRepeat(alarm.repeat)
    , m_selectedIcon(alarm.icon)
    , m_selectedColor(alarm.color)
{
    setWindowTitle(isNew ? tr("Новый будильник") : tr("Изменить будильник"));
    setMinimumSize(680, 600);
    const QScreen* targetScreen = parent != nullptr ? parent->screen() : QGuiApplication::primaryScreen();
    const int availableHeight = targetScreen != nullptr
                                    ? targetScreen->availableGeometry().height()
                                    : 900;
    resize(720, std::clamp(availableHeight - 70, 600, 800));
    buildUi();
    populateGroups(groups);
    populateSounds();
    loadAlarm();
}

AlarmDialog::~AlarmDialog()
{
    if (m_previewing) {
        SoundPlayer::instance().stop();
    }
}

Alarm AlarmDialog::result() const
{
    return m_alarm;
}

void AlarmDialog::buildUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(14, 14, 14, 12);
    root->setSpacing(10);

    auto* header = makeCard(this);
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(16, 12, 16, 12);
    headerLayout->setSpacing(14);

    m_iconPreview = new QLabel(header);
    m_iconPreview->setFixedSize(56, 56);
    m_iconPreview->setAlignment(Qt::AlignCenter);
    headerLayout->addWidget(m_iconPreview);

    auto* titleColumn = new QVBoxLayout;
    titleColumn->setSpacing(0);
    m_timeEdit = new QTimeEdit(header);
    m_timeEdit->setObjectName(QStringLiteral("HeroTime"));
    m_timeEdit->setDisplayFormat(Settings::instance().use24h() ? QStringLiteral("HH:mm")
                                                               : QStringLiteral("h:mm AP"));
    m_timeEdit->setFixedWidth(162);
    m_timeEdit->setTime(m_alarm.time);
    m_timeEdit->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    titleColumn->addWidget(m_timeEdit);

    m_labelEdit = new QLineEdit(header);
    m_labelEdit->setPlaceholderText(tr("Название будильника"));
    QFont labelFont = m_labelEdit->font();
    labelFont.setPointSizeF(11.5);
    labelFont.setWeight(QFont::DemiBold);
    m_labelEdit->setFont(labelFont);
    m_labelEdit->setText(m_alarm.label);
    m_labelEdit->setMinimumWidth(280);
    titleColumn->addWidget(m_labelEdit);
    headerLayout->addLayout(titleColumn, 1);

    auto* metaColumn = new QVBoxLayout;
    metaColumn->setSpacing(5);
    metaColumn->addWidget(caption(tr("ГРУППА"), header, 200));
    m_groupCombo = new QComboBox(header);
    m_groupCombo->setEditable(true);
    m_groupCombo->setMinimumWidth(180);
    m_groupCombo->setInsertPolicy(QComboBox::NoInsert);
    metaColumn->addWidget(m_groupCombo);
    m_enabledCheck = new QCheckBox(tr("Будильник включён"), header);
    m_enabledCheck->setChecked(m_alarm.enabled);
    metaColumn->addWidget(m_enabledCheck);
    metaColumn->addStretch(1);
    headerLayout->addLayout(metaColumn);

    root->addWidget(header);

    m_scroll = new QScrollArea(this);
    m_scroll->setFocusPolicy(Qt::NoFocus);
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto* content = new QWidget(m_scroll);
    content->setObjectName(QStringLiteral("Transparent"));
    auto* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(0, 0, 4, 0);
    contentLayout->setSpacing(10);
    m_scroll->setWidget(content);
    root->addWidget(m_scroll, 1);

    QVBoxLayout* repeatLayout = nullptr;
    auto* repeatBox = makeGroup(tr("Повторение"), content, &repeatLayout);
    {
        auto* row = new QHBoxLayout;
        row->setSpacing(8);
        row->addWidget(caption(tr("Повтор"), repeatBox));
        m_repeatCombo = new QComboBox(repeatBox);
        m_repeatCombo->addItem(tr("Один раз"), static_cast<int>(RepeatKind::Once));
        m_repeatCombo->addItem(tr("Каждый день"), static_cast<int>(RepeatKind::Daily));
        m_repeatCombo->addItem(tr("По будням"), static_cast<int>(RepeatKind::Weekdays));
        m_repeatCombo->addItem(tr("По выходным"), static_cast<int>(RepeatKind::Weekends));
        m_repeatCombo->addItem(tr("По дням недели"), static_cast<int>(RepeatKind::Weekly));
        m_repeatCombo->addItem(tr("Ежемесячно"), static_cast<int>(RepeatKind::Monthly));
        m_repeatCombo->addItem(tr("N-й день недели месяца"),
                               static_cast<int>(RepeatKind::MonthlyNthWeekday));
        m_repeatCombo->addItem(tr("Последний рабочий день месяца"),
                               static_cast<int>(RepeatKind::LastWeekdayOfMonth));
        m_repeatCombo->addItem(tr("Ежегодно"), static_cast<int>(RepeatKind::Yearly));
        m_repeatCombo->addItem(tr("С интервалом"), static_cast<int>(RepeatKind::Interval));
        m_repeatCombo->addItem(tr("Каждые N дней"), static_cast<int>(RepeatKind::EveryNDays));
        m_repeatCombo->addItem(tr("Каждые N недель"), static_cast<int>(RepeatKind::EveryNWeeks));
        row->addWidget(m_repeatCombo, 1);
        repeatLayout->addLayout(row);
    }

    m_oncePanel = new QWidget(repeatBox);
    {
        auto* panel = new QHBoxLayout(m_oncePanel);
        panel->setContentsMargins(0, 0, 0, 0);
        panel->setSpacing(8);
        panel->addWidget(caption(tr("Дата"), m_oncePanel));
        m_onceDate = new QDateEdit(m_oncePanel);
        m_onceDate->setCalendarPopup(true);
        m_onceDate->setDisplayFormat(QStringLiteral("dd.MM.yyyy"));
        m_onceDate->setDate(QDate::currentDate().addDays(1));
        panel->addWidget(m_onceDate);
        auto* hint = new QLabel(tr("Сработает один раз"), m_oncePanel);
        hint->setObjectName(QStringLiteral("Faint"));
        panel->addWidget(hint);
        panel->addStretch(1);
        repeatLayout->addWidget(m_oncePanel);
    }

    m_weeklyPanel = new QWidget(repeatBox);
    {
        auto* column = new QVBoxLayout(m_weeklyPanel);
        column->setContentsMargins(0, 0, 0, 0);
        column->setSpacing(5);
        auto* row = new QHBoxLayout;
        row->setSpacing(8);
        row->addWidget(caption(tr("Дни"), m_weeklyPanel));
        for (int day = Qt::Monday; day <= Qt::Sunday; ++day) {
            auto* button = new QPushButton(
                catalog::dayShort(static_cast<Qt::DayOfWeek>(day)), m_weeklyPanel);
            button->setCheckable(true);
            button->setFixedSize(36, 26);
            button->setStyleSheet(QStringLiteral("padding: 2px 3px;"));
            m_dayButtons.append(button);
            row->addWidget(button);
        }
        row->addStretch(1);
        column->addLayout(row);
        auto* hint = new QLabel(tr("Можно выбрать несколько дней"), m_weeklyPanel);
        hint->setObjectName(QStringLiteral("Faint"));
        column->addWidget(hint);
        repeatLayout->addWidget(m_weeklyPanel);
    }

    m_monthlyPanel = new QWidget(repeatBox);
    {
        auto* row = new QHBoxLayout(m_monthlyPanel);
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(8);
        row->addWidget(caption(tr("День"), m_monthlyPanel));
        m_monthDay = new QSpinBox(m_monthlyPanel);
        m_monthDay->setRange(1, 31);
        m_monthDay->setValue(1);
        m_monthDay->setSuffix(tr(" число"));
        row->addWidget(m_monthDay);
        auto* hint = new QLabel(tr("Если месяца короче, используется последний день"), m_monthlyPanel);
        hint->setObjectName(QStringLiteral("Faint"));
        row->addWidget(hint);
        row->addStretch(1);
        repeatLayout->addWidget(m_monthlyPanel);
    }

    m_yearlyPanel = new QWidget(repeatBox);
    {
        auto* row = new QHBoxLayout(m_yearlyPanel);
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(8);
        row->addWidget(caption(tr("Дата"), m_yearlyPanel));
        m_yearMonth = new QComboBox(m_yearlyPanel);
        for (int month = 1; month <= 12; ++month) {
            m_yearMonth->addItem(catalog::monthName(month), month);
        }
        m_yearMonth->setMinimumWidth(130);
        row->addWidget(m_yearMonth);
        m_yearDay = new QSpinBox(m_yearlyPanel);
        m_yearDay->setRange(1, 31);
        m_yearDay->setSuffix(tr(" число"));
        row->addWidget(m_yearDay);
        auto* hint = new QLabel(tr("Каждый год"), m_yearlyPanel);
        hint->setObjectName(QStringLiteral("Faint"));
        row->addWidget(hint);
        row->addStretch(1);
        repeatLayout->addWidget(m_yearlyPanel);
    }

    m_intervalPanel = new QWidget(repeatBox);
    {
        auto* column = new QVBoxLayout(m_intervalPanel);
        column->setContentsMargins(0, 0, 0, 0);
        column->setSpacing(6);
        auto* first = new QHBoxLayout;
        first->setSpacing(8);
        first->addWidget(caption(tr("Каждые"), m_intervalPanel));
        m_intervalMinutes = new QSpinBox(m_intervalPanel);
        m_intervalMinutes->setRange(1, 1440);
        m_intervalMinutes->setValue(30);
        m_intervalMinutes->setSuffix(tr(" мин"));
        first->addWidget(m_intervalMinutes);
        first->addStretch(1);
        column->addLayout(first);
        auto* second = new QHBoxLayout;
        second->setSpacing(8);
        second->addWidget(caption(tr("Время"), m_intervalPanel));
        auto* fromLabel = new QLabel(tr("с"), m_intervalPanel);
        fromLabel->setObjectName(QStringLiteral("Faint"));
        second->addWidget(fromLabel);
        m_intervalFrom = new QTimeEdit(m_intervalPanel);
        m_intervalFrom->setDisplayFormat(Settings::instance().use24h() ? QStringLiteral("HH:mm")
                                                                       : QStringLiteral("h:mm AP"));
        m_intervalTo = new QTimeEdit(m_intervalPanel);
        m_intervalTo->setDisplayFormat(Settings::instance().use24h() ? QStringLiteral("HH:mm")
                                                                     : QStringLiteral("h:mm AP"));
        m_intervalFrom->setTime(QTime(0, 0));
        m_intervalTo->setTime(QTime(23, 59));
        second->addWidget(m_intervalFrom);
        auto* toLabel = new QLabel(tr("до"), m_intervalPanel);
        toLabel->setObjectName(QStringLiteral("Faint"));
        second->addWidget(toLabel);
        second->addWidget(m_intervalTo);
        second->addStretch(1);
        column->addLayout(second);
        auto* hint = new QLabel(tr("Например: каждые 30 минут с 09:00 до 18:00"), m_intervalPanel);
        hint->setObjectName(QStringLiteral("Faint"));
        column->addWidget(hint);
        repeatLayout->addWidget(m_intervalPanel);
    }

    m_everyDaysPanel = new QWidget(repeatBox);
    {
        auto* row = new QHBoxLayout(m_everyDaysPanel);
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(8);
        row->addWidget(caption(tr("Каждые"), m_everyDaysPanel));
        m_everyDays = new QSpinBox(m_everyDaysPanel);
        m_everyDays->setRange(1, 365);
        m_everyDays->setValue(2);
        m_everyDays->setSuffix(tr(" дн."));
        row->addWidget(m_everyDays);
        auto* hint = new QLabel(tr("Серия начинается с даты ниже"), m_everyDaysPanel);
        hint->setObjectName(QStringLiteral("Faint"));
        row->addWidget(hint);
        row->addStretch(1);
        repeatLayout->addWidget(m_everyDaysPanel);
    }

    m_everyWeeksPanel = new QWidget(repeatBox);
    {
        auto* row = new QHBoxLayout(m_everyWeeksPanel);
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(8);
        row->addWidget(caption(tr("Каждые"), m_everyWeeksPanel));
        m_everyWeeks = new QSpinBox(m_everyWeeksPanel);
        m_everyWeeks->setRange(1, 52);
        m_everyWeeks->setValue(2);
        m_everyWeeks->setSuffix(tr(" нед."));
        row->addWidget(m_everyWeeks);
        auto* hint = new QLabel(tr("Дни недели выбираются выше"), m_everyWeeksPanel);
        hint->setObjectName(QStringLiteral("Faint"));
        row->addWidget(hint);
        row->addStretch(1);
        repeatLayout->addWidget(m_everyWeeksPanel);
    }

    m_seriesRow = new QWidget(repeatBox);
    {
        auto* row = new QHBoxLayout(m_seriesRow);
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(8);
        row->addWidget(caption(tr("Начало серии"), m_seriesRow));
        m_seriesStart = new QDateEdit(m_seriesRow);
        m_seriesStart->setCalendarPopup(true);
        m_seriesStart->setDisplayFormat(QStringLiteral("dd.MM.yyyy"));
        m_seriesStart->setDate(QDate::currentDate());
        row->addWidget(m_seriesStart);
        row->addStretch(1);
        repeatLayout->addWidget(m_seriesRow);
    }

    m_nthPanel = new QWidget(repeatBox);
    {
        auto* row = new QHBoxLayout(m_nthPanel);
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(8);
        row->addWidget(caption(tr("Когда"), m_nthPanel));
        m_nthWeek = new QComboBox(m_nthPanel);
        m_nthWeek->addItem(tr("Первый"), 1);
        m_nthWeek->addItem(tr("Второй"), 2);
        m_nthWeek->addItem(tr("Третий"), 3);
        m_nthWeek->addItem(tr("Четвёртый"), 4);
        m_nthWeek->addItem(tr("Последний"), -1);
        row->addWidget(m_nthWeek);
        m_nthWeekday = new QComboBox(m_nthPanel);
        for (int day = Qt::Monday; day <= Qt::Sunday; ++day) {
            m_nthWeekday->addItem(catalog::dayLong(static_cast<Qt::DayOfWeek>(day)), day);
        }
        row->addWidget(m_nthWeekday, 1);
        row->addStretch(1);
        repeatLayout->addWidget(m_nthPanel);
    }

    {
        auto* row = new QHBoxLayout;
        row->setSpacing(6);
        row->addWidget(caption(tr("Период"), repeatBox));
        m_activeFromCheck = new QCheckBox(tr("с"), repeatBox);
        m_activeFromDate = new QDateEdit(repeatBox);
        m_activeFromDate->setCalendarPopup(true);
        m_activeFromDate->setDisplayFormat(QStringLiteral("dd.MM.yyyy"));
        m_activeFromDate->setDate(QDate::currentDate());
        m_activeToCheck = new QCheckBox(tr("по"), repeatBox);
        m_activeToDate = new QDateEdit(repeatBox);
        m_activeToDate->setCalendarPopup(true);
        m_activeToDate->setDisplayFormat(QStringLiteral("dd.MM.yyyy"));
        m_activeToDate->setDate(QDate::currentDate().addMonths(1));
        row->addWidget(m_activeFromCheck);
        row->addWidget(m_activeFromDate);
        row->addWidget(m_activeToCheck);
        row->addWidget(m_activeToDate);
        row->addStretch(1);
        repeatLayout->addLayout(row);
    }

    {
        auto* row = new QHBoxLayout;
        row->setSpacing(8);
        row->addWidget(caption(tr("Часовой пояс"), repeatBox, 104));
        m_timeZoneCombo = new QComboBox(repeatBox);
        populateTimeZones(m_timeZoneCombo);
        row->addWidget(m_timeZoneCombo, 1);
        repeatLayout->addLayout(row);
    }
    {
        auto* row = new QHBoxLayout;
        row->setSpacing(8);
        row->addWidget(caption(tr("Праздники"), repeatBox, 104));
        m_holidayPolicyCombo = new QComboBox(repeatBox);
        m_holidayPolicyCombo->addItem(tr("Не учитывать"),
                                      static_cast<int>(HolidayPolicy::Ignore));
        m_holidayPolicyCombo->addItem(tr("Пропускать праздники"),
                                      static_cast<int>(HolidayPolicy::Skip));
        m_holidayPolicyCombo->addItem(tr("Переносить на следующий рабочий день"),
                                      static_cast<int>(HolidayPolicy::ShiftToNextWorkday));
        row->addWidget(m_holidayPolicyCombo, 1);
        repeatLayout->addLayout(row);
    }
    {
        auto* row = new QHBoxLayout;
        row->setSpacing(8);
        row->addWidget(caption(tr("Исключения"), repeatBox, 104));
        m_exceptionsEdit = new QLineEdit(repeatBox);
        m_exceptionsEdit->setPlaceholderText(tr("01.01.2027, 08.03.2027"));
        m_exceptionsEdit->setToolTip(tr("Даты через запятую, в которые будильник не срабатывает"));
        row->addWidget(m_exceptionsEdit, 1);
        repeatLayout->addLayout(row);
    }
    auto* exceptionsHint = new QLabel(
        tr("Праздники и пользовательские выходные настраиваются на странице будильников."),
        repeatBox);
    exceptionsHint->setObjectName(QStringLiteral("Faint"));
    exceptionsHint->setWordWrap(true);
    repeatLayout->addWidget(exceptionsHint);

    {
        auto* row = new QHBoxLayout;
        row->setSpacing(6);
        row->addWidget(caption(tr("Случайно"), repeatBox, 104));
        m_randomWindow = new QCheckBox(tr("Включить сдвиг"), repeatBox);
        m_randomBefore = new QSpinBox(repeatBox);
        m_randomBefore->setRange(0, 720);
        m_randomBefore->setValue(15);
        m_randomBefore->setSuffix(tr(" мин"));
        m_randomAfter = new QSpinBox(repeatBox);
        m_randomAfter->setRange(0, 720);
        m_randomAfter->setValue(15);
        m_randomAfter->setSuffix(tr(" мин"));
        row->addWidget(m_randomWindow);
        row->addWidget(new QLabel(QStringLiteral("−"), repeatBox));
        row->addWidget(m_randomBefore);
        row->addWidget(new QLabel(QStringLiteral("… +"), repeatBox));
        row->addWidget(m_randomAfter);
        row->addStretch(1);
        repeatLayout->addLayout(row);
    }

    m_repeatSummary = new QLabel(repeatBox);
    QFont summaryFont = m_repeatSummary->font();
    summaryFont.setWeight(QFont::DemiBold);
    m_repeatSummary->setFont(summaryFont);
    m_repeatSummary->setWordWrap(true);
    repeatLayout->addWidget(m_repeatSummary);
    m_nextRunLabel = new QLabel(repeatBox);
    m_nextRunLabel->setObjectName(QStringLiteral("Muted"));
    m_nextRunLabel->setWordWrap(true);
    repeatLayout->addWidget(m_nextRunLabel);
    contentLayout->addWidget(repeatBox);

    QVBoxLayout* extraLayout = nullptr;
    auto* extraBox = makeGroup(tr("Дополнительные расписания"), content, &extraLayout);
    m_extraScheduleList = new QListWidget(extraBox);
    m_extraScheduleList->setFixedHeight(92);
    m_extraScheduleList->setSelectionMode(QAbstractItemView::SingleSelection);
    extraLayout->addWidget(m_extraScheduleList);
    {
        auto* row = new QHBoxLayout;
        row->setSpacing(6);
        auto* addButton = new QPushButton(tr("Добавить…"), extraBox);
        auto* editButton = new QPushButton(tr("Изменить"), extraBox);
        auto* removeButton = new QPushButton(tr("Удалить"), extraBox);
        row->addWidget(addButton);
        row->addWidget(editButton);
        row->addWidget(removeButton);
        row->addStretch(1);
        extraLayout->addLayout(row);
        connect(addButton, &QPushButton::clicked, this, &AlarmDialog::addExtraSchedule);
        connect(editButton, &QPushButton::clicked, this, &AlarmDialog::editExtraSchedule);
        connect(removeButton, &QPushButton::clicked, this, &AlarmDialog::removeExtraSchedule);
        connect(m_extraScheduleList, &QListWidget::itemDoubleClicked, this,
                [this](QListWidgetItem*) { editExtraSchedule(); });
    }
    auto* extraHint = new QLabel(
        tr("Каждое дополнительное расписание срабатывает независимо от основного. "
           "Часовой пояс и праздники общие."),
        extraBox);
    extraHint->setObjectName(QStringLiteral("Faint"));
    extraHint->setWordWrap(true);
    extraLayout->addWidget(extraHint);
    contentLayout->addWidget(extraBox);

    QVBoxLayout* soundLayout = nullptr;
    auto* soundBox = makeGroup(tr("Звук"), content, &soundLayout);
    m_soundEnabled = new QCheckBox(tr("Включить звук будильника"), soundBox);
    m_soundEnabled->setChecked(true);
    soundLayout->addWidget(m_soundEnabled);
    {
        auto* grid = new QGridLayout;
        grid->setContentsMargins(0, 0, 0, 0);
        grid->setHorizontalSpacing(8);
        grid->setVerticalSpacing(8);
        grid->addWidget(caption(tr("Сигнал"), soundBox), 0, 0);
        m_soundCombo = new QComboBox(soundBox);
        grid->addWidget(m_soundCombo, 0, 1);
        m_browseSound = new QPushButton(tr("Обзор…"), soundBox);
        grid->addWidget(m_browseSound, 0, 2);
        m_previewButton = new ThemedButton(QString(), icons::Icon::Play, soundBox);
        m_previewButton->setToolTip(tr("Прослушать"));
        m_previewButton->setFixedSize(28, 28);
        m_previewButton->setIconSizePx(15);
        grid->addWidget(m_previewButton, 0, 3);

        grid->addWidget(caption(tr("Громкость"), soundBox), 1, 0);
        m_volumeSlider = new QSlider(Qt::Horizontal, soundBox);
        m_volumeSlider->setRange(0, 100);
        m_volumeSlider->setValue(80);
        m_volumeSlider->setMinimumWidth(180);
        grid->addWidget(m_volumeSlider, 1, 1, 1, 2);
        m_volumeValue = new QLabel(QStringLiteral("80%"), soundBox);
        m_volumeValue->setFixedWidth(38);
        m_volumeValue->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        grid->addWidget(m_volumeValue, 1, 3);

        grid->addWidget(caption(tr("Параметры"), soundBox), 2, 0);
        auto* options = new QHBoxLayout;
        options->setSpacing(12);
        m_fadeIn = new QCheckBox(tr("Нарастание"), soundBox);
        m_fadeInSeconds = new QSpinBox(soundBox);
        m_fadeInSeconds->setRange(1, 120);
        m_fadeInSeconds->setValue(6);
        m_fadeInSeconds->setSuffix(tr(" с"));
        m_fadeInSeconds->setToolTip(tr("Длительность нарастания громкости"));
        m_loopSound = new QCheckBox(tr("Повторять"), soundBox);
        m_loopSound->setChecked(true);
        m_maxSoundMinutes = new QSpinBox(soundBox);
        m_maxSoundMinutes->setRange(1, 120);
        m_maxSoundMinutes->setValue(5);
        m_maxSoundMinutes->setSuffix(tr(" мин"));
        options->addWidget(m_fadeIn);
        options->addWidget(m_fadeInSeconds);
        options->addWidget(m_loopSound);
        options->addWidget(new QLabel(tr("не дольше"), soundBox));
        options->addWidget(m_maxSoundMinutes);
        options->addStretch(1);
        grid->addLayout(options, 2, 1, 1, 3);
        grid->setColumnStretch(1, 1);

        grid->addWidget(caption(tr("На повторе"), soundBox), 3, 0);
        m_repeatSoundCombo = new QComboBox(soundBox);
        grid->addWidget(m_repeatSoundCombo, 3, 1, 1, 3);

        grid->addWidget(caption(tr("Затухание"), soundBox), 4, 0);
        auto* tailOptions = new QHBoxLayout;
        tailOptions->setSpacing(10);
        m_fadeOutSeconds = new QSpinBox(soundBox);
        m_fadeOutSeconds->setRange(0, 60);
        m_fadeOutSeconds->setValue(2);
        m_fadeOutSeconds->setSuffix(tr(" с"));
        m_fadeOutSeconds->setToolTip(tr("Плавное затухание после выключения"));
        m_randomSound = new QCheckBox(tr("Случайная встроенная мелодия"), soundBox);
        tailOptions->addWidget(m_fadeOutSeconds);
        tailOptions->addWidget(m_randomSound);
        tailOptions->addStretch(1);
        grid->addLayout(tailOptions, 4, 1, 1, 3);
        soundLayout->addLayout(grid);
    }
    contentLayout->addWidget(soundBox);

    auto* columns = new QWidget(content);
    auto* columnsLayout = new QHBoxLayout(columns);
    columnsLayout->setContentsMargins(0, 0, 0, 0);
    columnsLayout->setSpacing(10);

    QVBoxLayout* snoozeLayout = nullptr;
    auto* snoozeBox = makeGroup(tr("Откладывание"), columns, &snoozeLayout);
    m_snoozeEnabled = new QCheckBox(tr("Разрешить откладывание"), snoozeBox);
    m_snoozeEnabled->setChecked(true);
    snoozeLayout->addWidget(m_snoozeEnabled);
    {
        auto* row = new QHBoxLayout;
        row->setSpacing(6);
        row->addWidget(caption(tr("Через"), snoozeBox, 84));
        m_snoozeMinutes = new QSpinBox(snoozeBox);
        m_snoozeMinutes->setRange(1, 180);
        m_snoozeMinutes->setValue(5);
        m_snoozeMinutes->setSuffix(tr(" мин"));
        row->addWidget(m_snoozeMinutes);
        row->addStretch(1);
        snoozeLayout->addLayout(row);
    }
    {
        auto* row = new QHBoxLayout;
        row->setSpacing(6);
        row->addWidget(caption(tr("Или через"), snoozeBox, 84));
        m_snoozeMinutesAlt = new QSpinBox(snoozeBox);
        m_snoozeMinutesAlt->setRange(1, 180);
        m_snoozeMinutesAlt->setValue(10);
        m_snoozeMinutesAlt->setSuffix(tr(" мин"));
        row->addWidget(m_snoozeMinutesAlt);
        row->addStretch(1);
        snoozeLayout->addLayout(row);
    }
    {
        auto* row = new QHBoxLayout;
        row->setSpacing(6);
        m_snoozeDecrease = new QCheckBox(tr("Сокращать"), snoozeBox);
        m_snoozeDecreaseStep = new QSpinBox(snoozeBox);
        m_snoozeDecreaseStep->setRange(1, 60);
        m_snoozeDecreaseStep->setValue(1);
        m_snoozeDecreaseStep->setSuffix(tr(" мин"));
        row->addWidget(m_snoozeDecrease);
        row->addWidget(m_snoozeDecreaseStep);
        row->addStretch(1);
        snoozeLayout->addLayout(row);
    }
    {
        auto* row = new QHBoxLayout;
        row->setSpacing(6);
        row->addWidget(caption(tr("Громче на"), snoozeBox, 84));
        m_snoozeVolumeIncrease = new QSpinBox(snoozeBox);
        m_snoozeVolumeIncrease->setRange(0, 50);
        m_snoozeVolumeIncrease->setValue(5);
        m_snoozeVolumeIncrease->setSuffix(tr(" %"));
        row->addWidget(m_snoozeVolumeIncrease);
        row->addStretch(1);
        snoozeLayout->addLayout(row);
    }
    {
        auto* row = new QHBoxLayout;
        row->setSpacing(6);
        row->addWidget(caption(tr("После лимита"), snoozeBox, 84));
        m_afterLastSnooze = new QComboBox(snoozeBox);
        m_afterLastSnooze->addItem(tr("Остановить"),
                                   static_cast<int>(SnoozeEndAction::Stop));
        m_afterLastSnooze->addItem(tr("Выключить будильник"),
                                   static_cast<int>(SnoozeEndAction::DisableAlarm));
        row->addWidget(m_afterLastSnooze, 1);
        snoozeLayout->addLayout(row);
    }
    {
        auto* row = new QHBoxLayout;
        row->setSpacing(6);
        row->addWidget(caption(tr("Не более"), snoozeBox, 84));
        m_maxSnoozes = new QSpinBox(snoozeBox);
        m_maxSnoozes->setRange(0, 20);
        m_maxSnoozes->setValue(3);
        m_maxSnoozes->setSuffix(tr(" раз"));
        row->addWidget(m_maxSnoozes);
        row->addStretch(1);
        snoozeLayout->addLayout(row);
    }
    snoozeLayout->addStretch(1);
    columnsLayout->addWidget(snoozeBox, 1);

    QVBoxLayout* notifyLayout = nullptr;
    auto* notifyBox = makeGroup(tr("Уведомление"), columns, &notifyLayout);
    m_popupEnabled = new QCheckBox(tr("Показывать окно будильника"), notifyBox);
    m_popupEnabled->setChecked(true);
    notifyLayout->addWidget(m_popupEnabled);
    {
        auto* row = new QHBoxLayout;
        row->setSpacing(6);
        m_preAlertEnabled = new QCheckBox(tr("Заранее"), notifyBox);
        m_preAlertMinutes = new QSpinBox(notifyBox);
        m_preAlertMinutes->setRange(1, 1440);
        m_preAlertMinutes->setValue(10);
        m_preAlertMinutes->setSuffix(tr(" мин"));
        row->addWidget(m_preAlertEnabled);
        row->addWidget(m_preAlertMinutes);
        row->addStretch(1);
        notifyLayout->addLayout(row);
    }
    {
        auto* row = new QHBoxLayout;
        row->setSpacing(6);
        m_backupEnabled = new QCheckBox(tr("Резервный сигнал через"), notifyBox);
        m_backupAfterMinutes = new QSpinBox(notifyBox);
        m_backupAfterMinutes->setRange(1, 120);
        m_backupAfterMinutes->setValue(5);
        m_backupAfterMinutes->setSuffix(tr(" мин"));
        row->addWidget(m_backupEnabled);
        row->addWidget(m_backupAfterMinutes);
        row->addStretch(1);
        notifyLayout->addLayout(row);
    }
    m_persistent = new QCheckBox(tr("Повторять до выключения"), notifyBox);
    notifyLayout->addWidget(m_persistent);
    auto* notifyHint = new QLabel(tr("Предупреждение появится в трее"), notifyBox);
    notifyHint->setObjectName(QStringLiteral("Faint"));
    notifyHint->setWordWrap(true);
    notifyLayout->addWidget(notifyHint);
    notifyLayout->addStretch(1);
    columnsLayout->addWidget(notifyBox, 1);
    contentLayout->addWidget(columns);

    QVBoxLayout* afterLayout = nullptr;
    auto* afterBox = makeGroup(tr("После выключения"), content, &afterLayout);
    m_postSummary = new QCheckBox(tr("Показать дату, день недели и ближайший будильник"),
                                  afterBox);
    m_postTasks = new QCheckBox(tr("Показать задачи на сегодня"), afterBox);
    afterLayout->addWidget(m_postSummary);
    afterLayout->addWidget(m_postTasks);
    {
        auto* row = new QHBoxLayout;
        row->setSpacing(8);
        row->addWidget(caption(tr("Сообщение"), afterBox));
        m_postMessage = new QLineEdit(afterBox);
        m_postMessage->setPlaceholderText(tr("Короткое сообщение после сигнала"));
        row->addWidget(m_postMessage, 1);
        afterLayout->addLayout(row);
    }
    {
        auto* row = new QHBoxLayout;
        row->setSpacing(8);
        row->addWidget(caption(tr("Открыть"), afterBox));
        m_postOpen = new QLineEdit(afterBox);
        m_postOpen->setPlaceholderText(tr("Приложение, файл или адрес сайта"));
        row->addWidget(m_postOpen, 1);
        afterLayout->addLayout(row);
    }
    m_disableAfterDismiss = new QCheckBox(tr("Выключать будильник после остановки"),
                                          afterBox);
    afterLayout->addWidget(m_disableAfterDismiss);
    auto* afterHint = new QLabel(
        tr("Действия выполняются, когда вы останавливаете сигнал, а не откладываете его."),
        afterBox);
    afterHint->setObjectName(QStringLiteral("Faint"));
    afterHint->setWordWrap(true);
    afterLayout->addWidget(afterHint);
    contentLayout->addWidget(afterBox);

    QVBoxLayout* lookLayout = nullptr;
    auto* lookBox = makeGroup(tr("Оформление"), content, &lookLayout);
    {
        auto* iconsRow = new QHBoxLayout;
        iconsRow->setSpacing(2);
        iconsRow->addWidget(caption(tr("Значок"), lookBox, 70));
        for (const auto& entry : catalog::alarmIcons()) {
            auto* button = new EmojiChoiceButton(entry.emoji, lookBox);
            button->setToolTip(entry.name);
            m_iconButtons.append(button);
            iconsRow->addWidget(button);
            connect(button, &QPushButton::clicked, this, [this, emoji = entry.emoji] {
                m_selectedIcon = emoji;
                updateAppearance();
            });
        }
        iconsRow->addStretch(1);
        lookLayout->addLayout(iconsRow);
    }
    {
        auto* colorsRow = new QHBoxLayout;
        colorsRow->setSpacing(3);
        colorsRow->addWidget(caption(tr("Цвет"), lookBox, 70));
        for (const auto& color : catalog::alarmColors()) {
            auto* button = new ColorChoiceButton(color, lookBox);
            m_colorButtons.append(button);
            colorsRow->addWidget(button);
            connect(button, &QPushButton::clicked, this, [this, color] {
                m_selectedColor = color;
                updateAppearance();
            });
        }
        colorsRow->addStretch(1);
        lookLayout->addLayout(colorsRow);
    }
    contentLayout->addWidget(lookBox);

    QVBoxLayout* noteLayout = nullptr;
    auto* noteBox = makeGroup(tr("Заметка"), content, &noteLayout);
    m_noteEdit = new QPlainTextEdit(noteBox);
    m_noteEdit->setPlaceholderText(tr("Дополнительная информация к будильнику"));
    m_noteEdit->setFixedHeight(62);
    m_noteEdit->setTabChangesFocus(true);
    noteLayout->addWidget(m_noteEdit);
    contentLayout->addWidget(noteBox);

    contentLayout->addStretch(1);

    auto* footer = new QWidget(this);
    auto* footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(2, 0, 2, 0);
    auto* footerHint = new QLabel(tr("Изменения применятся после сохранения"), footer);
    footerHint->setObjectName(QStringLiteral("Faint"));
    footerLayout->addWidget(footerHint);
    footerLayout->addStretch(1);
    m_buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, footer);
    auto* saveButton = m_buttons->button(QDialogButtonBox::Save);
    saveButton->setText(tr("Сохранить"));
    saveButton->setProperty("accent", true);
    saveButton->setDefault(true);
    saveButton->style()->unpolish(saveButton);
    saveButton->style()->polish(saveButton);
    m_buttons->button(QDialogButtonBox::Cancel)->setText(tr("Отмена"));
    footerLayout->addWidget(m_buttons);
    root->addWidget(footer);

    connect(m_buttons, &QDialogButtonBox::accepted, this, &AlarmDialog::accept);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &AlarmDialog::reject);

    connect(m_repeatCombo, &QComboBox::currentIndexChanged, this, [this] {
        updateRepeatPanels();
        if (static_cast<RepeatKind>(m_repeatCombo->currentData().toInt()) == RepeatKind::Once
            && m_onceDate->date() < QDate::currentDate()) {
            m_onceDate->setDate(QDate::currentDate().addDays(1));
        }
        updateSchedulePreview();
    });
    connect(m_timeEdit, &QTimeEdit::timeChanged, this, [this] { updateSchedulePreview(); });
    connect(m_onceDate, &QDateEdit::dateChanged, this, [this] { updateSchedulePreview(); });
    for (auto* button : m_dayButtons) {
        connect(button, &QPushButton::toggled, this, [this] { updateSchedulePreview(); });
    }
    connect(m_monthDay, &QSpinBox::valueChanged, this, [this] { updateSchedulePreview(); });
    connect(m_yearMonth, &QComboBox::currentIndexChanged, this, [this] {
        const int month = m_yearMonth->currentData().toInt();
        const int maxDay = QDate(2000, month, 1).daysInMonth();
        m_yearDay->setMaximum(maxDay);
        if (m_yearDay->value() > maxDay) {
            m_yearDay->setValue(maxDay);
        }
        updateSchedulePreview();
    });
    connect(m_yearDay, &QSpinBox::valueChanged, this, [this] { updateSchedulePreview(); });
    connect(m_intervalMinutes, &QSpinBox::valueChanged, this, [this] { updateSchedulePreview(); });
    connect(m_intervalFrom, &QTimeEdit::timeChanged, this, [this] { updateSchedulePreview(); });
    connect(m_intervalTo, &QTimeEdit::timeChanged, this, [this] { updateSchedulePreview(); });
    connect(m_activeFromCheck, &QCheckBox::toggled, this, [this](bool checked) {
        m_activeFromDate->setEnabled(checked);
        updateSchedulePreview();
    });
    connect(m_activeToCheck, &QCheckBox::toggled, this, [this](bool checked) {
        m_activeToDate->setEnabled(checked);
        updateSchedulePreview();
    });
    connect(m_activeFromDate, &QDateEdit::dateChanged, this, [this] { updateSchedulePreview(); });
    connect(m_activeToDate, &QDateEdit::dateChanged, this, [this] { updateSchedulePreview(); });
    for (auto* spin : {m_everyDays, m_everyWeeks}) {
        connect(spin, &QSpinBox::valueChanged, this, [this] { updateSchedulePreview(); });
    }
    connect(m_seriesStart, &QDateEdit::dateChanged, this, [this] { updateSchedulePreview(); });
    connect(m_nthWeek, &QComboBox::currentIndexChanged, this, [this] { updateSchedulePreview(); });
    connect(m_nthWeekday, &QComboBox::currentIndexChanged, this,
            [this] { updateSchedulePreview(); });
    connect(m_timeZoneCombo, &QComboBox::currentIndexChanged, this,
            [this] { updateSchedulePreview(); });
    connect(m_holidayPolicyCombo, &QComboBox::currentIndexChanged, this,
            [this] { updateSchedulePreview(); });
    connect(m_exceptionsEdit, &QLineEdit::textChanged, this,
            [this] { updateSchedulePreview(); });
    connect(m_randomWindow, &QCheckBox::toggled, this, [this](bool checked) {
        m_randomBefore->setEnabled(checked);
        m_randomAfter->setEnabled(checked);
        updateSchedulePreview();
    });
    connect(m_randomBefore, &QSpinBox::valueChanged, this, [this] { updateSchedulePreview(); });
    connect(m_randomAfter, &QSpinBox::valueChanged, this, [this] { updateSchedulePreview(); });
    connect(m_backupEnabled, &QCheckBox::toggled, this, [this](bool checked) {
        m_backupAfterMinutes->setEnabled(checked);
        m_persistent->setEnabled(checked);
    });
    connect(m_soundEnabled, &QCheckBox::toggled, this, [this] {
        updateSoundControls();
    });
    connect(m_volumeSlider, &QSlider::valueChanged, this, [this](int value) {
        m_volumeValue->setText(QStringLiteral("%1%").arg(value));
    });
    connect(m_fadeIn, &QCheckBox::toggled, this, [this] { updateSoundControls(); });
    connect(m_loopSound, &QCheckBox::toggled, this, [this] { updateSoundControls(); });
    connect(m_snoozeEnabled, &QCheckBox::toggled, this, [this] { updateSnoozeControls(); });
    connect(m_snoozeDecrease, &QCheckBox::toggled, this, [this] { updateSnoozeControls(); });
    connect(m_preAlertEnabled, &QCheckBox::toggled, this, [this] { updatePreAlertControls(); });
    connect(m_previewButton, &QPushButton::clicked, this, &AlarmDialog::togglePreview);
    connect(m_browseSound, &QPushButton::clicked, this, &AlarmDialog::chooseSoundFile);
    connect(m_soundCombo, &QComboBox::currentIndexChanged, this, [this] {
        if (m_previewing) {
            SoundPlayer::instance().stop();
            m_previewing = false;
            m_previewButton->setGlyph(icons::Icon::Play);
            m_previewButton->setToolTip(tr("Прослушать"));
        }
    });

    m_activeFromDate->setEnabled(false);
    m_activeToDate->setEnabled(false);
    updateRepeatPanels();
    updateSoundControls();
    updateSnoozeControls();
    updatePreAlertControls();
}

void AlarmDialog::populateGroups(const QStringList& groups)
{
    const QString current = m_alarm.group;
    m_groupCombo->clear();
    m_groupCombo->addItems(groups);
    if (!current.isEmpty() && m_groupCombo->findText(current) < 0) {
        m_groupCombo->addItem(current);
    }
    if (m_groupCombo->count() == 0) {
        m_groupCombo->addItem(QStringLiteral("Общие"));
    }
    m_groupCombo->setCurrentText(current.isEmpty() ? m_groupCombo->itemText(0) : current);
}

void AlarmDialog::populateSounds()
{
    m_soundCombo->clear();
    for (const auto& sound : catalog::availableSounds()) {
        m_soundCombo->addItem(sound.name, sound.id);
    }
    if (m_alarm.soundId.startsWith(QStringLiteral("file:"))) {
        m_customSoundId = m_alarm.soundId;
        if (m_soundCombo->findData(m_customSoundId) < 0) {
            m_soundCombo->addItem(
                tr("Файл: %1").arg(QFileInfo(m_alarm.soundId.mid(5)).fileName()),
                m_customSoundId);
        }
    }
    const int index = m_soundCombo->findData(m_alarm.soundId);
    m_soundCombo->setCurrentIndex(index >= 0 ? index : 0);

    if (m_repeatSoundCombo != nullptr) {
        m_repeatSoundCombo->clear();
        m_repeatSoundCombo->addItem(tr("Как основной сигнал"), QString());
        for (const auto& sound : catalog::availableSounds()) {
            m_repeatSoundCombo->addItem(sound.name, sound.id);
        }
        if (!m_customSoundId.isEmpty()
            && m_repeatSoundCombo->findData(m_customSoundId) < 0) {
            m_repeatSoundCombo->addItem(
                tr("Файл: %1").arg(QFileInfo(m_customSoundId.mid(5)).fileName()),
                m_customSoundId);
        }
        const int repeatSoundIndex = m_repeatSoundCombo->findData(m_alarm.repeatSoundId);
        m_repeatSoundCombo->setCurrentIndex(repeatSoundIndex >= 0 ? repeatSoundIndex : 0);
    }
}

void AlarmDialog::loadAlarm()
{
    if (m_alarm.time.isValid()) {
        m_timeEdit->setTime(m_alarm.time);
    }
    m_labelEdit->setText(m_alarm.label);
    m_groupCombo->setCurrentText(m_alarm.group.isEmpty() ? QStringLiteral("Общие") : m_alarm.group);
    m_enabledCheck->setChecked(m_alarm.enabled);

    int repeatIndex = m_repeatCombo->findData(static_cast<int>(m_alarm.repeat));
    if (repeatIndex < 0) {
        repeatIndex = 0;
    }
    m_repeatCombo->setCurrentIndex(repeatIndex);

    for (int i = 0; i < m_dayButtons.size(); ++i) {
        const auto day = static_cast<Qt::DayOfWeek>(Qt::Monday + i);
        m_dayButtons.at(i)->setChecked(m_alarm.days.contains(day));
    }
    if (m_alarm.repeat == RepeatKind::Weekly && m_alarm.days.isEmpty()) {
        for (int i = 0; i < 5; ++i) {
            m_dayButtons.at(i)->setChecked(true);
        }
    }

    m_onceDate->setDate(m_alarm.onceDate.isValid() ? m_alarm.onceDate
                                                   : QDate::currentDate().addDays(1));
    m_monthDay->setValue(std::clamp(m_alarm.monthDay, 1, 31));
    const QDate seed = m_alarm.onceDate.isValid() ? m_alarm.onceDate : QDate::currentDate();
    m_yearMonth->setCurrentIndex(seed.month() - 1);
    m_yearDay->setValue(seed.day());
    m_intervalMinutes->setValue(std::clamp(m_alarm.intervalMinutes, 1, 1440));
    m_intervalFrom->setTime(m_alarm.activeFrom.isValid() ? m_alarm.activeFrom : QTime(0, 0));
    m_intervalTo->setTime(m_alarm.activeTo.isValid() ? m_alarm.activeTo : QTime(23, 59));
    m_everyDays->setValue(std::clamp(m_alarm.intervalDays, 1, 365));
    m_everyWeeks->setValue(std::clamp(m_alarm.intervalWeeks, 1, 52));
    m_seriesStart->setDate(m_alarm.seriesStart.isValid() ? m_alarm.seriesStart
                                                         : QDate::currentDate());
    const Qt::DayOfWeek nthDay = m_alarm.days.isEmpty()
                                     ? static_cast<Qt::DayOfWeek>(Qt::Tuesday)
                                     : m_alarm.days.first();
    int nthDayIndex = m_nthWeekday->findData(static_cast<int>(nthDay));
    m_nthWeekday->setCurrentIndex(nthDayIndex >= 0 ? nthDayIndex : 0);
    int nthWeekIndex = m_nthWeek->findData(m_alarm.monthWeek == 0 ? 1 : m_alarm.monthWeek);
    m_nthWeek->setCurrentIndex(nthWeekIndex >= 0 ? nthWeekIndex : 0);
    const int zoneIndex = m_timeZoneCombo->findData(m_alarm.timeZoneId);
    m_timeZoneCombo->setCurrentIndex(zoneIndex >= 0 ? zoneIndex : 0);
    const int policyIndex = m_holidayPolicyCombo->findData(
        static_cast<int>(m_alarm.holidayPolicy));
    m_holidayPolicyCombo->setCurrentIndex(policyIndex >= 0 ? policyIndex : 0);
    m_exceptionsEdit->setText(exceptionsToString(m_alarm.exceptions));
    m_randomWindow->setChecked(m_alarm.randomWindow);
    m_randomBefore->setValue(std::clamp(m_alarm.randomBeforeMinutes, 0, 720));
    m_randomAfter->setValue(std::clamp(m_alarm.randomAfterMinutes, 0, 720));
    m_randomBefore->setEnabled(m_alarm.randomWindow);
    m_randomAfter->setEnabled(m_alarm.randomWindow);
    m_backupEnabled->setChecked(m_alarm.backupEnabled);
    m_backupAfterMinutes->setValue(std::clamp(m_alarm.backupAfterMinutes, 1, 120));
    m_backupAfterMinutes->setEnabled(m_alarm.backupEnabled);
    m_persistent->setChecked(m_alarm.persistent);
    m_persistent->setEnabled(m_alarm.backupEnabled);
    m_activeFromCheck->setChecked(m_alarm.activeFromDate.isValid());
    if (m_alarm.activeFromDate.isValid()) {
        m_activeFromDate->setDate(m_alarm.activeFromDate);
    }
    m_activeToCheck->setChecked(m_alarm.activeToDate.isValid());
    if (m_alarm.activeToDate.isValid()) {
        m_activeToDate->setDate(m_alarm.activeToDate);
    }

    const int soundIndex = m_soundCombo->findData(m_alarm.soundId);
    m_soundCombo->setCurrentIndex(soundIndex >= 0 ? soundIndex : 0);
    m_soundEnabled->setChecked(m_alarm.soundEnabled);
    m_volumeSlider->setValue(std::clamp(m_alarm.volume, 0, 100));
    m_volumeValue->setText(QStringLiteral("%1%").arg(m_volumeSlider->value()));
    m_fadeIn->setChecked(m_alarm.fadeIn);
    m_fadeInSeconds->setValue(std::clamp(m_alarm.fadeInSeconds, 1, 120));
    m_fadeOutSeconds->setValue(std::clamp(m_alarm.fadeOutSeconds, 0, 60));
    m_randomSound->setChecked(m_alarm.randomSound);
    if (m_repeatSoundCombo != nullptr) {
        const int repeatSoundIndex = m_repeatSoundCombo->findData(m_alarm.repeatSoundId);
        m_repeatSoundCombo->setCurrentIndex(repeatSoundIndex >= 0 ? repeatSoundIndex : 0);
    }
    m_loopSound->setChecked(m_alarm.loopSound);
    m_maxSoundMinutes->setValue(std::clamp(m_alarm.maxSoundMinutes, 1, 120));

    m_snoozeEnabled->setChecked(m_alarm.snoozeEnabled);
    m_snoozeMinutes->setValue(std::clamp(m_alarm.snoozeMinutes, 1, 180));
    m_snoozeMinutesAlt->setValue(std::clamp(m_alarm.snoozeMinutesAlt, 1, 180));
    m_snoozeDecrease->setChecked(m_alarm.snoozeDecrease);
    m_snoozeDecreaseStep->setValue(std::clamp(m_alarm.snoozeDecreaseStep, 1, 60));
    m_snoozeVolumeIncrease->setValue(std::clamp(m_alarm.snoozeVolumeIncrease, 0, 50));
    int endIndex = m_afterLastSnooze->findData(static_cast<int>(m_alarm.afterLastSnooze));
    m_afterLastSnooze->setCurrentIndex(endIndex >= 0 ? endIndex : 0);
    m_maxSnoozes->setValue(std::clamp(m_alarm.maxSnoozes, 0, 20));
    m_popupEnabled->setChecked(m_alarm.popupEnabled);
    m_preAlertEnabled->setChecked(m_alarm.preAlertMinutes > 0);
    m_preAlertMinutes->setValue(m_alarm.preAlertMinutes > 0 ? m_alarm.preAlertMinutes : 10);
    m_postSummary->setChecked(m_alarm.postSummary);
    m_postTasks->setChecked(m_alarm.postTasks);
    m_postMessage->setText(m_alarm.postMessage);
    m_postOpen->setText(m_alarm.postOpen);
    m_disableAfterDismiss->setChecked(m_alarm.disableAfterDismiss);
    m_noteEdit->setPlainText(m_alarm.note);

    m_selectedIcon = m_alarm.icon;
    m_selectedColor = m_alarm.color;
    updateAppearance();
    updateRepeatPanels();
    updateSoundControls();
    updateSnoozeControls();
    updatePreAlertControls();
    updateActiveRangeControls();
    updateExtraScheduleList();
    updateSchedulePreview();
}

void AlarmDialog::updateRepeatPanels()
{
    const auto repeat = static_cast<RepeatKind>(m_repeatCombo->currentData().toInt());
    m_oncePanel->setVisible(repeat == RepeatKind::Once);
    m_weeklyPanel->setVisible(repeat == RepeatKind::Weekly
                              || repeat == RepeatKind::EveryNWeeks);
    m_monthlyPanel->setVisible(repeat == RepeatKind::Monthly);
    m_yearlyPanel->setVisible(repeat == RepeatKind::Yearly);
    m_intervalPanel->setVisible(repeat == RepeatKind::Interval);
    m_everyDaysPanel->setVisible(repeat == RepeatKind::EveryNDays);
    m_everyWeeksPanel->setVisible(repeat == RepeatKind::EveryNWeeks);
    m_seriesRow->setVisible(repeat == RepeatKind::EveryNDays
                            || repeat == RepeatKind::EveryNWeeks);
    m_nthPanel->setVisible(repeat == RepeatKind::MonthlyNthWeekday);

    if (repeat == RepeatKind::Weekly || repeat == RepeatKind::EveryNWeeks) {
        const bool any = std::any_of(m_dayButtons.cbegin(), m_dayButtons.cend(),
                                     [](const QPushButton* button) { return button->isChecked(); });
        if (!any) {
            if (repeat == RepeatKind::Weekly) {
                for (int i = 0; i < 5; ++i) {
                    m_dayButtons.at(i)->setChecked(true);
                }
            } else {
                const int index = m_seriesStart->date().dayOfWeek() - 1;
                if (index >= 0 && index < m_dayButtons.size()) {
                    m_dayButtons.at(index)->setChecked(true);
                }
            }
        }
    }
}

void AlarmDialog::updateSchedulePreview()
{
    const Alarm draft = collect();
    m_repeatSummary->setText(schedule::repeatSummary(draft));
    const QDateTime now = QDateTime::currentDateTime();
    const QDateTime next = schedule::nextOccurrence(draft, now);
    if (next.isValid()) {
        m_nextRunLabel->setText(tr("Следующее срабатывание: %1")
                                    .arg(schedule::dateTimeText(next, now,
                                                                Settings::instance().use24h())));
        m_nextRunLabel->setStyleSheet(QString());
    } else {
        m_nextRunLabel->setText(tr("Нет будущих срабатываний"));
        m_nextRunLabel->setStyleSheet(
            QStringLiteral("color:%1;").arg(ThemeManager::instance().current().danger.name()));
    }
}

void AlarmDialog::updateAppearance()
{
    if (m_selectedIcon.isEmpty()) {
        m_selectedIcon = QStringLiteral("⏰");
    }
    if (m_selectedColor.isEmpty()) {
        m_selectedColor = QStringLiteral("#e2842b");
    }
    const QColor color(m_selectedColor);
    m_iconPreview->setPixmap(emojiPixmap(m_selectedIcon, 36));
    m_iconPreview->setStyleSheet(
        QStringLiteral("background: rgba(%1,%2,%3,40); border: 1px solid rgba(%1,%2,%3,115);"
                       " border-radius: 10px;")
            .arg(color.red())
            .arg(color.green())
            .arg(color.blue()));
    for (auto* button : m_iconButtons) {
        button->setChecked(button->property("emoji").toString() == m_selectedIcon);
    }
    for (auto* button : m_colorButtons) {
        button->setChecked(button->property("swatch").toString() == m_selectedColor);
    }
}

void AlarmDialog::updateSoundControls()
{
    const bool enabled = m_soundEnabled->isChecked();
    m_soundCombo->setEnabled(enabled);
    m_browseSound->setEnabled(enabled);
    m_previewButton->setEnabled(enabled);
    m_volumeSlider->setEnabled(enabled);
    m_volumeValue->setEnabled(enabled);
    m_fadeIn->setEnabled(enabled);
    m_fadeInSeconds->setEnabled(enabled && m_fadeIn->isChecked());
    m_fadeOutSeconds->setEnabled(enabled);
    m_randomSound->setEnabled(enabled);
    if (m_repeatSoundCombo != nullptr) {
        m_repeatSoundCombo->setEnabled(enabled);
    }
    m_loopSound->setEnabled(enabled);
    m_maxSoundMinutes->setEnabled(enabled && m_loopSound->isChecked());
    if (!enabled && m_previewing) {
        SoundPlayer::instance().stop();
        m_previewing = false;
        m_previewButton->setGlyph(icons::Icon::Play);
        m_previewButton->setToolTip(tr("Прослушать"));
    }
}

void AlarmDialog::updateSnoozeControls()
{
    const bool enabled = m_snoozeEnabled->isChecked();
    m_snoozeMinutes->setEnabled(enabled);
    m_snoozeMinutesAlt->setEnabled(enabled);
    m_snoozeDecrease->setEnabled(enabled);
    m_snoozeDecreaseStep->setEnabled(enabled && m_snoozeDecrease->isChecked());
    m_snoozeVolumeIncrease->setEnabled(enabled);
    m_afterLastSnooze->setEnabled(enabled);
    m_maxSnoozes->setEnabled(enabled);
}

void AlarmDialog::updatePreAlertControls()
{
    m_preAlertMinutes->setEnabled(m_preAlertEnabled->isChecked());
}

void AlarmDialog::updateActiveRangeControls()
{
    m_activeFromDate->setEnabled(m_activeFromCheck->isChecked());
    m_activeToDate->setEnabled(m_activeToCheck->isChecked());
}

void AlarmDialog::updateExtraScheduleList()
{
    if (m_extraScheduleList == nullptr) {
        return;
    }
    const int selected = m_extraScheduleList->currentRow();
    m_extraScheduleList->clear();
    for (int index = 0; index < static_cast<int>(m_alarm.extraSchedules.size()); ++index) {
        const AlarmSchedule& schedule = m_alarm.extraSchedules.at(
            static_cast<std::size_t>(index));
        QString text = QStringLiteral("%1 · %2")
                           .arg(schedule::timeText(schedule.time,
                                                   Settings::instance().use24h()),
                                scheduleSummary(m_alarm, schedule));
        if (!schedule.enabled) {
            text += tr(" · выключено");
        }
        auto* item = new QListWidgetItem(text, m_extraScheduleList);
        item->setData(Qt::UserRole, index);
    }
    if (selected >= 0 && selected < m_extraScheduleList->count()) {
        m_extraScheduleList->setCurrentRow(selected);
    }
}

void AlarmDialog::addExtraSchedule()
{
    AlarmSchedule schedule;
    schedule.time = m_timeEdit->time();
    schedule.repeat = RepeatKind::Daily;
    schedule.anchorDate = QDate::currentDate();
    schedule.activeFrom = QTime(0, 0);
    schedule.activeTo = QTime(23, 59);
    AlarmScheduleDialog dialog(schedule, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    m_alarm.extraSchedules.push_back(dialog.result());
    updateExtraScheduleList();
    updateSchedulePreview();
}

void AlarmDialog::editExtraSchedule()
{
    auto* item = m_extraScheduleList->currentItem();
    if (item == nullptr) {
        return;
    }
    const int index = item->data(Qt::UserRole).toInt();
    if (index < 0 || index >= static_cast<int>(m_alarm.extraSchedules.size())) {
        return;
    }
    AlarmSchedule schedule = m_alarm.extraSchedules.at(static_cast<std::size_t>(index));
    AlarmScheduleDialog dialog(schedule, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    m_alarm.extraSchedules.at(static_cast<std::size_t>(index)) = dialog.result();
    updateExtraScheduleList();
    updateSchedulePreview();
}

void AlarmDialog::removeExtraSchedule()
{
    auto* item = m_extraScheduleList->currentItem();
    if (item == nullptr) {
        return;
    }
    const int index = item->data(Qt::UserRole).toInt();
    if (index < 0 || index >= static_cast<int>(m_alarm.extraSchedules.size())) {
        return;
    }
    m_alarm.extraSchedules.erase(m_alarm.extraSchedules.begin() + index);
    updateExtraScheduleList();
    updateSchedulePreview();
}

void AlarmDialog::chooseSoundFile()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Выберите звук будильника"), QString(),
        tr("Аудиофайлы (*.wav *.mp3 *.wma *.ogg *.flac);;Все файлы (*.*)"));
    if (path.isEmpty()) {
        return;
    }
    for (int i = m_soundCombo->count() - 1; i >= 0; --i) {
        if (m_soundCombo->itemData(i).toString().startsWith(QStringLiteral("file:"))) {
            m_soundCombo->removeItem(i);
        }
    }
    m_customSoundId = QStringLiteral("file:") + QDir::toNativeSeparators(path);
    m_soundCombo->addItem(tr("Файл: %1").arg(QFileInfo(path).fileName()), m_customSoundId);
    m_soundCombo->setCurrentIndex(m_soundCombo->count() - 1);
    m_soundEnabled->setChecked(true);
    if (m_previewing) {
        SoundPlayer::instance().stop();
        m_previewing = false;
        m_previewButton->setGlyph(icons::Icon::Play);
        m_previewButton->setToolTip(tr("Прослушать"));
    }
    updateSoundControls();
}

void AlarmDialog::togglePreview()
{
    if (m_previewing) {
        SoundPlayer::instance().stop();
        m_previewing = false;
        m_previewButton->setGlyph(icons::Icon::Play);
        m_previewButton->setToolTip(tr("Прослушать"));
        return;
    }
    const QString soundId = selectedSoundId();
    if (SoundPlayer::resolveSoundPath(soundId).isEmpty()) {
        QMessageBox::warning(this, tr("Звук"), tr("Не удалось найти или открыть выбранный звук."));
        return;
    }
    SoundPlayer::instance().play(soundId, m_volumeSlider->value(), false, 0, false);
    m_previewing = true;
    m_previewButton->setGlyph(icons::Icon::Stop);
    m_previewButton->setToolTip(tr("Остановить"));
}

QString AlarmDialog::selectedSoundId() const
{
    const QString id = m_soundCombo->currentData().toString();
    return id.isEmpty() ? Settings::instance().defaultAlarmSound() : id;
}

Alarm AlarmDialog::collect() const
{
    Alarm alarm = m_alarm;
    alarm.time = m_timeEdit->time();
    alarm.time = QTime(alarm.time.hour(), alarm.time.minute());
    alarm.label = m_labelEdit->text().trimmed();
    alarm.group = m_groupCombo->currentText().trimmed();
    if (alarm.group.isEmpty()) {
        alarm.group = QStringLiteral("Общие");
    }
    alarm.enabled = m_enabledCheck->isChecked();
    alarm.repeat = static_cast<RepeatKind>(m_repeatCombo->currentData().toInt());
    alarm.days.clear();
    for (int i = 0; i < m_dayButtons.size(); ++i) {
        if (m_dayButtons.at(i)->isChecked()) {
            alarm.days.append(static_cast<Qt::DayOfWeek>(Qt::Monday + i));
        }
    }
    if (alarm.repeat == RepeatKind::Once) {
        alarm.onceDate = m_onceDate->date();
    } else if (alarm.repeat == RepeatKind::Yearly) {
        const int month = m_yearMonth->currentData().toInt();
        const int day = std::min(m_yearDay->value(), QDate(2000, month, 1).daysInMonth());
        alarm.onceDate = QDate(2000, month, day);
    }
    alarm.monthDay = m_monthDay->value();
    alarm.monthWeek = m_nthWeek->currentData().toInt();
    alarm.intervalMinutes = m_intervalMinutes->value();
    alarm.intervalDays = m_everyDays->value();
    alarm.intervalWeeks = m_everyWeeks->value();
    if (alarm.repeat == RepeatKind::EveryNDays || alarm.repeat == RepeatKind::EveryNWeeks) {
        alarm.seriesStart = m_seriesStart->date();
    }
    if (alarm.repeat == RepeatKind::MonthlyNthWeekday) {
        alarm.days.clear();
        alarm.days.append(static_cast<Qt::DayOfWeek>(m_nthWeekday->currentData().toInt()));
    }
    alarm.timeZoneId = m_timeZoneCombo->currentData().toString();
    alarm.holidayPolicy = static_cast<HolidayPolicy>(
        m_holidayPolicyCombo->currentData().toInt());
    alarm.exceptions = exceptionsFromString(m_exceptionsEdit->text(), nullptr);
    alarm.randomWindow = m_randomWindow->isChecked();
    alarm.randomBeforeMinutes = m_randomBefore->value();
    alarm.randomAfterMinutes = m_randomAfter->value();
    alarm.activeFrom = m_intervalFrom->time();
    alarm.activeTo = m_intervalTo->time();
    alarm.activeFromDate = m_activeFromCheck->isChecked() ? m_activeFromDate->date() : QDate();
    alarm.activeToDate = m_activeToCheck->isChecked() ? m_activeToDate->date() : QDate();
    alarm.soundEnabled = m_soundEnabled->isChecked();
    alarm.soundId = selectedSoundId();
    alarm.volume = m_volumeSlider->value();
    alarm.fadeIn = m_fadeIn->isChecked();
    alarm.fadeInSeconds = m_fadeInSeconds->value();
    alarm.fadeOutSeconds = m_fadeOutSeconds->value();
    alarm.randomSound = m_randomSound->isChecked();
    alarm.repeatSoundId = m_repeatSoundCombo->currentData().toString();
    alarm.loopSound = m_loopSound->isChecked();
    alarm.maxSoundMinutes = m_maxSoundMinutes->value();
    alarm.snoozeEnabled = m_snoozeEnabled->isChecked();
    alarm.snoozeMinutes = m_snoozeMinutes->value();
    alarm.snoozeMinutesAlt = m_snoozeMinutesAlt->value();
    alarm.snoozeDecrease = m_snoozeDecrease->isChecked();
    alarm.snoozeDecreaseStep = m_snoozeDecreaseStep->value();
    alarm.snoozeVolumeIncrease = m_snoozeVolumeIncrease->value();
    alarm.afterLastSnooze = static_cast<SnoozeEndAction>(
        m_afterLastSnooze->currentData().toInt());
    alarm.maxSnoozes = m_maxSnoozes->value();
    alarm.popupEnabled = m_popupEnabled->isChecked();
    alarm.preAlertMinutes = m_preAlertEnabled->isChecked() ? m_preAlertMinutes->value() : 0;
    alarm.backupEnabled = m_backupEnabled->isChecked();
    alarm.backupAfterMinutes = m_backupAfterMinutes->value();
    alarm.persistent = m_persistent->isChecked();
    alarm.postSummary = m_postSummary->isChecked();
    alarm.postTasks = m_postTasks->isChecked();
    alarm.postMessage = m_postMessage->text().trimmed();
    alarm.postOpen = m_postOpen->text().trimmed();
    alarm.disableAfterDismiss = m_disableAfterDismiss->isChecked();
    alarm.icon = m_selectedIcon;
    alarm.color = m_selectedColor;
    alarm.note = m_noteEdit->toPlainText();
    return alarm;
}

bool AlarmDialog::validate(const Alarm& alarm)
{
    QStringList invalidExceptions;
    const QList<QDate> parsedExceptions =
        exceptionsFromString(m_exceptionsEdit->text(), &invalidExceptions);
    Q_UNUSED(parsedExceptions)
    if (!invalidExceptions.isEmpty()) {
        QMessageBox::warning(
            this, tr("Исключения"),
            tr("Не удалось разобрать даты: %1. Используйте формат ДД.ММ.ГГГГ.")
                .arg(invalidExceptions.join(QStringLiteral(", "))));
        return false;
    }
    if (alarm.repeat == RepeatKind::Weekly && alarm.days.isEmpty()) {
        QMessageBox::warning(this, tr("Повторение"),
                             tr("Выберите хотя бы один день недели."));
        return false;
    }
    if (alarm.activeFromDate.isValid() && alarm.activeToDate.isValid()
        && alarm.activeFromDate > alarm.activeToDate) {
        QMessageBox::warning(this, tr("Период действия"),
                             tr("Дата начала не может быть позже даты окончания."));
        return false;
    }
    if (alarm.repeat == RepeatKind::Once && alarm.enabled) {
        const QDateTime candidate(alarm.onceDate, alarm.time);
        if (!candidate.isValid() || candidate <= QDateTime::currentDateTime()) {
            QMessageBox::warning(this, tr("Дата и время"),
                                 tr("Однократный будильник должен быть установлен на будущее время."));
            return false;
        }
    }
    if (alarm.enabled) {
        const QDateTime next = schedule::nextOccurrence(alarm, QDateTime::currentDateTime());
        if (!next.isValid()) {
            QMessageBox::warning(
                this, tr("Расписание"),
                tr("В выбранном расписании нет будущих срабатываний. "
                   "Проверьте дни недели и период действия."));
            return false;
        }
    }
    if (alarm.soundEnabled && SoundPlayer::resolveSoundPath(alarm.soundId).isEmpty()) {
        QMessageBox box(QMessageBox::Warning, tr("Звук"),
                        tr("Выбранный звуковой файл не найден. При срабатывании будет "
                           "использован встроенный резервный сигнал. Сохранить будильник?"),
                        QMessageBox::Yes | QMessageBox::No, this);
        box.setDefaultButton(QMessageBox::Yes);
        if (box.exec() != QMessageBox::Yes) {
            return false;
        }
    }
    return true;
}

void AlarmDialog::accept()
{
    Alarm draft = collect();
    if (!validate(draft)) {
        return;
    }
    if (m_isNew || draft.repeat != m_originalRepeat) {
        draft.lastTriggered = QDateTime();
        draft.snoozeUntil = QDateTime();
        draft.snoozeCount = 0;
        draft.intervalAnchor = draft.repeat == RepeatKind::Interval
                                   ? QDateTime::currentDateTime()
                                   : QDateTime();
    } else {
        draft.lastTriggered = m_original.lastTriggered;
        draft.snoozeUntil = m_original.snoozeUntil;
        draft.snoozeCount = m_original.snoozeCount;
        draft.intervalAnchor = m_original.intervalAnchor;
    }
    if (!draft.enabled) {
        draft.snoozeUntil = QDateTime();
        draft.snoozeCount = 0;
    }
    if (draft.repeat == RepeatKind::Interval && !draft.intervalAnchor.isValid()) {
        draft.intervalAnchor = QDateTime::currentDateTime();
    }
    m_alarm = draft;
    if (m_previewing) {
        SoundPlayer::instance().stop();
        m_previewing = false;
    }
    QDialog::accept();
}

void AlarmDialog::reject()
{
    if (m_previewing) {
        SoundPlayer::instance().stop();
        m_previewing = false;
    }
    QDialog::reject();
}

void AlarmDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    if (m_labelEdit != nullptr) {
        m_labelEdit->setFocus(Qt::OtherFocusReason);
        if (!m_isNew) {
            m_labelEdit->selectAll();
        }
    }
    const auto resetScroll = [this] {
        if (m_scroll != nullptr) {
            m_scroll->verticalScrollBar()->setValue(0);
        }
    };
    QTimer::singleShot(0, this, resetScroll);
    QTimer::singleShot(120, this, resetScroll);
    QTimer::singleShot(300, this, resetScroll);
}

} // namespace pac::ui
