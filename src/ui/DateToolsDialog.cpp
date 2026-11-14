#include "DateToolsDialog.h"

#include "UiKit.h"
#include "core/Catalog.h"
#include "core/DateMath.h"
#include "core/Fonts.h"
#include "core/Settings.h"
#include "core/Theme.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QDateEdit>
#include <QDateTimeEdit>
#include <QDialogButtonBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QScrollArea>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTimeEdit>
#include <QTimeZone>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>
#include <functional>

namespace pac::ui {
namespace {

[[nodiscard]] QString plural(int count, const QString& one, const QString& few, const QString& many)
{
    const int mod10 = count % 10;
    const int mod100 = count % 100;
    QString word = many;
    if (mod10 == 1 && mod100 != 11) {
        word = one;
    } else if (mod10 >= 2 && mod10 <= 4 && (mod100 < 12 || mod100 > 14)) {
        word = few;
    }
    return QStringLiteral("%1 %2").arg(count).arg(word);
}

[[nodiscard]] QString daysText(int count)
{
    return plural(count, QObject::tr("день"), QObject::tr("дня"), QObject::tr("дней"));
}

[[nodiscard]] QString monthsText(int count)
{
    return plural(count, QObject::tr("месяц"), QObject::tr("месяца"), QObject::tr("месяцев"));
}

[[nodiscard]] QString yearsText(int count)
{
    return plural(count, QObject::tr("год"), QObject::tr("года"), QObject::tr("лет"));
}

[[nodiscard]] QString dateSpanHeadline(const datemath::DateSpan& span)
{
    datemath::DateSpan value = span;
    const bool negative = value.negative;
    value.negative = false;
    QStringList parts;
    if (value.years > 0) {
        parts.append(yearsText(value.years));
    }
    if (value.months > 0) {
        parts.append(monthsText(value.months));
    }
    if (value.days > 0 || parts.isEmpty()) {
        parts.append(daysText(value.days));
    }
    const QString text = parts.join(QLatin1Char(' '));
    return negative ? QStringLiteral("−") + text : text;
}

[[nodiscard]] QLabel* caption(const QString& text, QWidget* parent, int width = 150)
{
    auto* label = new QLabel(text, parent);
    label->setObjectName(QStringLiteral("Muted"));
    label->setFixedWidth(width);
    label->setWordWrap(false);
    return label;
}

[[nodiscard]] QDateEdit* dateField(QWidget* parent, const QDate& date)
{
    auto* edit = new QDateEdit(parent);
    edit->setCalendarPopup(true);
    edit->setDisplayFormat(QStringLiteral("dd.MM.yyyy"));
    edit->setDate(date);
    return edit;
}

[[nodiscard]] QString timeFormat()
{
    return Settings::instance().use24h() ? QStringLiteral("HH:mm:ss")
                                         : QStringLiteral("h:mm:ss AP");
}

[[nodiscard]] QDateTimeEdit* dateTimeField(QWidget* parent, const QDateTime& value)
{
    auto* edit = new QDateTimeEdit(parent);
    edit->setCalendarPopup(true);
    edit->setDisplayFormat(QStringLiteral("dd.MM.yyyy ") + timeFormat());
    edit->setDateTime(value);
    return edit;
}

[[nodiscard]] QTimeEdit* timeField(QWidget* parent, const QTime& value)
{
    auto* edit = new QTimeEdit(parent);
    edit->setDisplayFormat(Settings::instance().use24h() ? QStringLiteral("HH:mm")
                                                          : QStringLiteral("h:mm AP"));
    edit->setTime(value);
    return edit;
}

[[nodiscard]] QString formattedDate(const QDate& date)
{
    if (!date.isValid()) {
        return QObject::tr("—");
    }
    return QStringLiteral("%1 (%2)")
        .arg(date.toString(QStringLiteral("dd.MM.yyyy")),
             catalog::dayShort(static_cast<Qt::DayOfWeek>(date.dayOfWeek())));
}

[[nodiscard]] QString formattedDateTime(const QDateTime& value)
{
    if (!value.isValid()) {
        return QObject::tr("—");
    }
    return value.toString(QStringLiteral("dd.MM.yyyy ") + timeFormat());
}

[[nodiscard]] QString offsetText(int seconds)
{
    const QChar sign = seconds < 0 ? QChar('-') : QChar('+');
    const int absSeconds = std::abs(seconds);
    return QStringLiteral("UTC%1%2:%3")
        .arg(sign)
        .arg(absSeconds / 3600, 2, 10, QLatin1Char('0'))
        .arg((absSeconds % 3600) / 60, 2, 10, QLatin1Char('0'));
}

[[nodiscard]] QDateTime wallTime(const QDateTimeEdit* edit, const QComboBox* zoneCombo)
{
    const QString id = zoneCombo->currentData().toString();
    const QTimeZone zone = id.isEmpty() ? QTimeZone::systemTimeZone()
                                        : (id == QStringLiteral("UTC") ? QTimeZone::utc()
                                                                       : QTimeZone(id.toUtf8()));
    if (!zone.isValid()) {
        return {};
    }
    const QTime raw = edit->time();
    const QTime clean(raw.hour(), raw.minute(), raw.second());
    return QDateTime(edit->date(), clean, zone);
}

[[nodiscard]] QDateTime wallTimeNow(const QTimeZone& zone)
{
    const QDateTime zoned = QDateTime::currentDateTime().toTimeZone(zone);
    const QTime time = zoned.time();
    return QDateTime(zoned.date(), QTime(time.hour(), time.minute(), time.second()));
}

void populateZones(QComboBox* combo)
{
    combo->addItem(QObject::tr("Местное время"), QString());
    combo->addItem(QStringLiteral("UTC"), QStringLiteral("UTC"));
    for (const auto& city : catalog::cities()) {
        combo->addItem(QStringLiteral("%1 · %2").arg(city.city, city.country), city.timeZoneId);
    }
    combo->setMaxVisibleItems(18);
    combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    combo->setMinimumContentsLength(20);
}

void selectZone(QComboBox* combo, const QString& zoneId)
{
    const int index = combo->findData(zoneId);
    if (index >= 0) {
        combo->setCurrentIndex(index);
    }
}

[[nodiscard]] QTimeZone zoneOf(const QComboBox* combo)
{
    const QString id = combo->currentData().toString();
    if (id.isEmpty()) {
        return QTimeZone::systemTimeZone();
    }
    return id == QStringLiteral("UTC") ? QTimeZone::utc() : QTimeZone(id.toUtf8());
}

[[nodiscard]] QString localDateTimeText(const QDateTime& value)
{
    if (!value.isValid()) {
        return QObject::tr("—");
    }
    return formattedDateTime(value.toLocalTime());
}

[[nodiscard]] QString isoText(const QDateTime& value)
{
    if (!value.isValid()) {
        return QObject::tr("—");
    }
    return value.toOffsetFromUtc(value.offsetFromUtc()).toString(Qt::ISODateWithMs);
}

[[nodiscard]] QString dstText(const QDateTime& value, const QTimeZone& zone)
{
    if (!value.isValid() || !zone.isValid()) {
        return QObject::tr("—");
    }
    return zone.isDaylightTime(value) ? QObject::tr("летнее время")
                                      : QObject::tr("стандартное время");
}

[[nodiscard]] QString spanToText(const QDateTime& from, const QDateTime& to)
{
    const datemath::DateTimeSpan span = datemath::dateTimeDifference(from, to);
    return datemath::dateTimeSpanText(span);
}

void setToolResult(QLabel* label, const QString& headline, const QString& detail = {})
{
    if (label == nullptr) {
        return;
    }
    const ThemeColors& colors = ThemeManager::instance().current();
    QString html = QStringLiteral("<div style=\"font-size:14pt; font-weight:600;\">%1</div>")
                       .arg(headline.toHtmlEscaped());
    if (!detail.isEmpty()) {
        QString text = detail.toHtmlEscaped();
        text.replace(QLatin1Char('\n'), QStringLiteral("<br/>"));
        html += QStringLiteral("<div style=\"font-size:8.5pt; color:%1;\">%2</div>")
                    .arg(colors.dim.name(), text);
    }
    label->setTextFormat(Qt::RichText);
    label->setText(html);
}

struct ToolPage {
    QWidget* page{nullptr};
    QFrame* card{nullptr};
    QVBoxLayout* body{nullptr};
};

[[nodiscard]] ToolPage makeToolPage(icons::Icon icon, const QString& title,
                                    const QString& subtitle)
{
    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto* content = new QWidget(scroll);
    content->setObjectName(QStringLiteral("Transparent"));
    auto* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(0, 0, 4, 0);
    contentLayout->setSpacing(0);

    auto* card = new QFrame(content);
    card->setObjectName(QStringLiteral("ToolCard"));
    card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(22, 20, 22, 20);
    cardLayout->setSpacing(13);

    auto* header = new QHBoxLayout;
    header->setSpacing(13);
    auto* chip = new QLabel(card);
    chip->setObjectName(QStringLiteral("ToolIconChip"));
    chip->setFixedSize(40, 40);
    chip->setAlignment(Qt::AlignCenter);
    chip->setPixmap(icons::pixmap(icon, 20, ThemeManager::instance().current().accent));
    header->addWidget(chip, 0, Qt::AlignTop);

    auto* titleColumn = new QVBoxLayout;
    titleColumn->setSpacing(2);
    auto* titleLabel = new QLabel(title, card);
    titleLabel->setObjectName(QStringLiteral("CardTitle"));
    QFont titleFont = titleLabel->font();
    titleFont.setPointSizeF(12.5);
    titleFont.setWeight(QFont::DemiBold);
    titleLabel->setFont(titleFont);
    auto* subtitleLabel = new QLabel(subtitle, card);
    subtitleLabel->setObjectName(QStringLiteral("Muted"));
    subtitleLabel->setWordWrap(true);
    titleColumn->addWidget(titleLabel);
    titleColumn->addWidget(subtitleLabel);
    header->addLayout(titleColumn, 1);
    cardLayout->addLayout(header);
    cardLayout->addWidget(makeHorizontalRule(card));

    auto* body = new QVBoxLayout;
    body->setSpacing(11);
    cardLayout->addLayout(body, 1);

    contentLayout->addWidget(card, 1);
    scroll->setWidget(content);
    return {scroll, card, body};
}

void addField(QVBoxLayout* body, const QString& label, QWidget* field, QWidget* parent,
              int fieldWidth)
{
    auto* row = new QHBoxLayout;
    row->setSpacing(12);
    row->addWidget(caption(label, parent));
    field->setFixedWidth(fieldWidth);
    row->addWidget(field, 0, Qt::AlignLeft);
    row->addStretch(1);
    body->addLayout(row);
}

void addHint(QVBoxLayout* body, const QString& text, QWidget* parent)
{
    auto* hint = new QLabel(text, parent);
    hint->setObjectName(QStringLiteral("Faint"));
    hint->setWordWrap(true);
    auto* row = new QHBoxLayout;
    row->setContentsMargins(162, 0, 0, 0);
    row->addWidget(hint, 1);
    body->addLayout(row);
}

[[nodiscard]] QLabel* addResultPanel(QVBoxLayout* body, QWidget* parent)
{
    auto* frame = new QFrame(parent);
    frame->setObjectName(QStringLiteral("ToolResult"));
    auto* layout = new QVBoxLayout(frame);
    layout->setContentsMargins(16, 13, 16, 14);
    auto* label = new QLabel(frame);
    label->setWordWrap(true);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    label->setTextFormat(Qt::RichText);
    layout->addWidget(label);
    body->addSpacing(3);
    body->addWidget(frame);
    return label;
}

[[nodiscard]] QFrame* makeZoneBox(QWidget* parent, const QString& title)
{
    auto* frame = new QFrame(parent);
    frame->setObjectName(QStringLiteral("ToolZone"));
    auto* layout = new QVBoxLayout(frame);
    layout->setContentsMargins(13, 11, 13, 12);
    layout->setSpacing(7);
    auto* header = new QLabel(title, frame);
    header->setObjectName(QStringLiteral("SectionLabel"));
    layout->addWidget(header);
    return frame;
}

} // namespace

DateToolsDialog::DateToolsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Инструменты календаря"));
    setMinimumSize(980, 640);
    resize(1140, 760);
    buildUi();
}

void DateToolsDialog::buildUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 14, 16, 12);
    root->setSpacing(12);

    auto* hero = new QFrame(this);
    hero->setObjectName(QStringLiteral("ToolsHero"));
    auto* heroLayout = new QHBoxLayout(hero);
    heroLayout->setContentsMargins(18, 14, 18, 14);
    heroLayout->setSpacing(14);

    auto* chip = new QLabel(hero);
    chip->setObjectName(QStringLiteral("ToolIconChip"));
    chip->setFixedSize(46, 46);
    chip->setAlignment(Qt::AlignCenter);
    chip->setPixmap(icons::pixmap(icons::Icon::Calendar, 24,
                                  ThemeManager::instance().current().accent));
    heroLayout->addWidget(chip, 0, Qt::AlignVCenter);

    auto* titleColumn = new QVBoxLayout;
    titleColumn->setSpacing(3);
    auto* title = new QLabel(tr("Инструменты календаря"), hero);
    QFont titleFont = title->font();
    titleFont.setPointSizeF(16.5);
    titleFont.setWeight(QFont::DemiBold);
    title->setFont(titleFont);
    auto* subtitle = new QLabel(
        tr("Арифметика дат, рабочие дни, часовые пояса, Unix и ISO 8601"), hero);
    subtitle->setObjectName(QStringLiteral("Muted"));
    titleColumn->addWidget(title);
    titleColumn->addWidget(subtitle);
    heroLayout->addLayout(titleColumn, 1);

    auto* liveColumn = new QVBoxLayout;
    liveColumn->setSpacing(1);
    m_headerClock = new QLabel(hero);
    m_headerClock->setFont(fonts::digitFont(14.0, QFont::DemiBold));
    m_headerClock->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_headerDate = new QLabel(hero);
    m_headerDate->setObjectName(QStringLiteral("Faint"));
    m_headerDate->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    liveColumn->addWidget(m_headerClock);
    liveColumn->addWidget(m_headerDate);
    heroLayout->addLayout(liveColumn, 0);
    root->addWidget(hero);

    auto* content = new QHBoxLayout;
    content->setSpacing(12);
    buildNavigation(content);
    root->addLayout(content, 1);

    buildDatePages();
    buildTimePages();

    connect(m_nav, &QListWidget::currentRowChanged, this, [this](int row) {
        auto* item = m_nav->item(row);
        if (item == nullptr || !(item->flags() & Qt::ItemIsSelectable)) {
            return;
        }
        const int index = item->data(Qt::UserRole).toInt();
        if (index >= 0 && index < m_pages->count()) {
            m_pages->setCurrentIndex(index);
        }
    });
    for (int row = 0; row < m_nav->count(); ++row) {
        if ((m_nav->item(row)->flags() & Qt::ItemIsSelectable) != 0) {
            m_nav->setCurrentRow(row);
            break;
        }
    }

    auto* footer = new QDialogButtonBox(QDialogButtonBox::Close, this);
    footer->button(QDialogButtonBox::Close)->setText(tr("Закрыть"));
    footer->button(QDialogButtonBox::Close)->setProperty("accent", true);
    footer->button(QDialogButtonBox::Close)->setMinimumWidth(104);
    connect(footer, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(footer);

    m_ticker = new QTimer(this);
    m_ticker->setInterval(1000);
    connect(m_ticker, &QTimer::timeout, this, &DateToolsDialog::updateLive);
    m_ticker->start();

    connectInputs();
    recalculateDates();
    recalculateTime();
    updateLive();
}

void DateToolsDialog::buildNavigation(QHBoxLayout* content)
{
    auto* sidebar = new QFrame(this);
    sidebar->setObjectName(QStringLiteral("ToolsSidebar"));
    sidebar->setFixedWidth(218);
    auto* sideLayout = new QVBoxLayout(sidebar);
    sideLayout->setContentsMargins(12, 12, 12, 12);
    sideLayout->setSpacing(6);

    auto* navTitle = new QLabel(tr("КАЛЬКУЛЯТОРЫ"), sidebar);
    navTitle->setObjectName(QStringLiteral("SectionLabel"));
    sideLayout->addWidget(navTitle);

    m_nav = new QListWidget(sidebar);
    m_nav->setObjectName(QStringLiteral("ToolsNav"));
    m_nav->setFrameShape(QFrame::NoFrame);
    m_nav->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_nav->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_nav->setSelectionMode(QAbstractItemView::SingleSelection);
    m_nav->setUniformItemSizes(false);
    m_nav->setSpacing(0);
    sideLayout->addWidget(m_nav, 1);

    auto* note = new QLabel(tr("Все вычисления выполняются локально."), sidebar);
    note->setObjectName(QStringLiteral("Faint"));
    note->setWordWrap(true);
    sideLayout->addWidget(note);

    content->addWidget(sidebar, 0);
    m_pages = new QStackedWidget(this);
    m_pages->setObjectName(QStringLiteral("Transparent"));
    content->addWidget(m_pages, 1);
}

void DateToolsDialog::addNavSection(const QString& title)
{
    auto* item = new QListWidgetItem(title.toUpper(), m_nav);
    item->setFlags(Qt::NoItemFlags);
    QFont font = item->font();
    font.setPointSizeF(7.5);
    font.setWeight(QFont::DemiBold);
    font.setLetterSpacing(QFont::AbsoluteSpacing, 0.7);
    item->setFont(font);
    item->setSizeHint(QSize(0, 30));
}

void DateToolsDialog::addToolPage(const QString& navTitle, QWidget* page)
{
    auto* item = new QListWidgetItem(navTitle, m_nav);
    item->setData(Qt::UserRole, m_pages->count());
    m_pages->addWidget(page);
}

void DateToolsDialog::buildDatePages()
{
    addNavSection(tr("ДАТЫ"));
    const auto beginPage = [this](icons::Icon icon, const QString& navTitle,
                                  const QString& title, const QString& subtitle) {
        const ToolPage page = makeToolPage(icon, title, subtitle);
        addToolPage(navTitle, page.page);
        return page;
    };

    {
        const ToolPage page = beginPage(
            icons::Icon::Calendar, tr("Разница дат"), tr("Разница между датами"),
            tr("Полная разница в годах, месяцах и днях, а также общее число дней."));
        m_diffFrom = dateField(page.card, QDate::currentDate());
        m_diffTo = dateField(page.card, QDate::currentDate().addDays(30));
        addField(page.body, tr("Первая дата"), m_diffFrom, page.card, 210);
        addField(page.body, tr("Вторая дата"), m_diffTo, page.card, 210);
        m_diffResult = addResultPanel(page.body, page.card);
        page.body->addStretch(1);
    }

    {
        const ToolPage page = beginPage(
            icons::Icon::CalendarPlus, tr("Сдвиг даты"), tr("Прибавить или вычесть"),
            tr("Дни, недели, месяцы, годы и рабочие дни — в обе стороны."));
        m_addBase = dateField(page.card, QDate::currentDate());
        m_addAmount = new QSpinBox(page.card);
        m_addAmount->setRange(-100'000, 100'000);
        m_addAmount->setValue(7);
        m_addUnit = new QComboBox(page.card);
        m_addUnit->addItem(tr("дней"), 0);
        m_addUnit->addItem(tr("недель"), 1);
        m_addUnit->addItem(tr("месяцев"), 2);
        m_addUnit->addItem(tr("лет"), 3);
        m_addUnit->addItem(tr("рабочих дней"), 4);
        addField(page.body, tr("Начальная дата"), m_addBase, page.card, 210);
        addField(page.body, tr("Количество"), m_addAmount, page.card, 150);
        addField(page.body, tr("Единица"), m_addUnit, page.card, 210);
        m_addResult = addResultPanel(page.body, page.card);
        page.body->addStretch(1);
    }

    {
        const ToolPage page = beginPage(
            icons::Icon::Repeat, tr("Рабочие дни"), tr("Рабочие дни"),
            tr("Быстрый сдвиг по будням без учёта государственных праздников."));
        m_bizBase = dateField(page.card, QDate::currentDate());
        m_bizDays = new QSpinBox(page.card);
        m_bizDays->setRange(-100'000, 100'000);
        m_bizDays->setValue(5);
        addField(page.body, tr("Дата"), m_bizBase, page.card, 210);
        addField(page.body, tr("Рабочих дней"), m_bizDays, page.card, 150);
        addHint(page.body, tr("Считаются только понедельник–пятница."), page.card);
        m_bizResult = addResultPanel(page.body, page.card);
        page.body->addStretch(1);
    }

    {
        const ToolPage page = beginPage(
            icons::Icon::Info, tr("Сведения о дате"), tr("Сведения о дате"),
            tr("ISO-неделя, день года, границы месяца и квартала, високосность."));
        m_infoDate = dateField(page.card, QDate::currentDate());
        addField(page.body, tr("Дата"), m_infoDate, page.card, 210);
        addHint(page.body, tr("Для исторических дат используется пролептический "
                              "григорианский календарь."),
                page.card);
        m_infoResult = addResultPanel(page.body, page.card);
        page.body->addStretch(1);
    }

    {
        const ToolPage page = beginPage(
            icons::Icon::Star, tr("Возраст на дату"), tr("Возраст на дату"),
            tr("Полные годы, месяцы и дни, а также дата следующего дня рождения."));
        m_birthDate = dateField(page.card, QDate::currentDate().addYears(-30));
        m_ageDate = dateField(page.card, QDate::currentDate());
        addField(page.body, tr("Дата рождения"), m_birthDate, page.card, 210);
        addField(page.body, tr("На дату"), m_ageDate, page.card, 210);
        m_ageResult = addResultPanel(page.body, page.card);
        page.body->addStretch(1);
    }

    {
        const ToolPage page = beginPage(
            icons::Icon::Clock, tr("Прошло с события"), tr("Прошло с события"),
            tr("Сколько времени прошло с выбранного момента или осталось до него."));
        m_eventAt = dateTimeField(page.card, QDateTime::currentDateTime().addSecs(-3600));
        addField(page.body, tr("Событие"), m_eventAt, page.card, 260);
        m_eventResult = addResultPanel(page.body, page.card);
        page.body->addStretch(1);
    }

    {
        const ToolPage page = beginPage(
            icons::Icon::Bell, tr("Обратный отсчёт"), tr("Обратный отсчёт"),
            tr("Точный интервал до события в днях, часах, минутах и секундах."));
        m_countdownAt = dateTimeField(page.card, QDateTime::currentDateTime().addDays(7));
        addField(page.body, tr("Событие"), m_countdownAt, page.card, 260);
        m_countdownResult = addResultPanel(page.body, page.card);
        page.body->addStretch(1);
    }

    {
        const ToolPage page = beginPage(
            icons::Icon::Grid, tr("Дни недели"), tr("Дни недели в диапазоне"),
            tr("Сколько рабочих, выходных и отдельных дней недели попадает в период."));
        m_rangeFrom = dateField(page.card,
                                QDate(QDate::currentDate().year(), QDate::currentDate().month(), 1));
        m_rangeTo = dateField(page.card, QDate::currentDate());
        addField(page.body, tr("С даты"), m_rangeFrom, page.card, 210);
        addField(page.body, tr("По дату"), m_rangeTo, page.card, 210);
        addHint(page.body, tr("Обе даты включены в подсчёт."), page.card);
        m_rangeResult = addResultPanel(page.body, page.card);
        page.body->addStretch(1);
    }
}

void DateToolsDialog::buildTimePages()
{
    addNavSection(tr("ВРЕМЯ"));
    const auto beginPage = [this](icons::Icon icon, const QString& navTitle,
                                  const QString& title, const QString& subtitle) {
        const ToolPage page = makeToolPage(icon, title, subtitle);
        addToolPage(navTitle, page.page);
        return page;
    };

    const QDateTime now = QDateTime::currentDateTime();

    {
        const ToolPage page = beginPage(
            icons::Icon::Clock, tr("Разница моментов"), tr("Разница между моментами"),
            tr("Два момента в разных часовых поясах и точная длительность между ними."));
        m_momentA = dateTimeField(page.card, now);
        m_momentB = dateTimeField(page.card, now.addDays(1));
        m_momentZoneA = new QComboBox(page.card);
        m_momentZoneB = new QComboBox(page.card);
        populateZones(m_momentZoneA);
        populateZones(m_momentZoneB);
        selectZone(m_momentZoneA, QStringLiteral("Europe/Moscow"));
        selectZone(m_momentZoneB, QStringLiteral("Europe/London"));
        addField(page.body, tr("Момент A"), m_momentA, page.card, 250);
        addField(page.body, tr("Зона A"), m_momentZoneA, page.card, 340);
        addField(page.body, tr("Момент B"), m_momentB, page.card, 250);
        addField(page.body, tr("Зона B"), m_momentZoneB, page.card, 340);
        addHint(page.body, tr("Дата и время вводятся как местное время выбранной зоны."),
                page.card);
        m_momentResult = addResultPanel(page.body, page.card);
        page.body->addStretch(1);
    }

    {
        const ToolPage page = beginPage(
            icons::Icon::Globe, tr("Конвертер зон"), tr("Конвертер часовых поясов"),
            tr("Перевод момента из одной зоны в другую с учётом смещения и DST."));
        const QTimeZone sourceZone(QStringLiteral("Europe/Moscow").toUtf8());
        m_convertAt = dateTimeField(page.card, wallTimeNow(sourceZone));
        m_convertFrom = new QComboBox(page.card);
        m_convertTo = new QComboBox(page.card);
        populateZones(m_convertFrom);
        populateZones(m_convertTo);
        selectZone(m_convertFrom, QStringLiteral("Europe/Moscow"));
        selectZone(m_convertTo, QStringLiteral("Asia/Tokyo"));

        auto* timeRow = new QHBoxLayout;
        timeRow->setSpacing(12);
        timeRow->addWidget(caption(tr("Дата и время"), page.card));
        m_convertAt->setFixedWidth(250);
        timeRow->addWidget(m_convertAt);
        auto* nowButton = new QPushButton(tr("Сейчас"), page.card);
        nowButton->setFixedWidth(104);
        nowButton->setToolTip(tr("Подставить текущий момент в исходной зоне"));
        timeRow->addWidget(nowButton);
        timeRow->addStretch(1);
        page.body->addLayout(timeRow);
        addField(page.body, tr("Из зоны"), m_convertFrom, page.card, 340);
        addField(page.body, tr("В зону"), m_convertTo, page.card, 340);
        connect(nowButton, &QPushButton::clicked, this, [this] {
            m_convertAt->setDateTime(wallTimeNow(zoneOf(m_convertFrom)));
            recalculateTime();
        });
        m_convertResult = addResultPanel(page.body, page.card);
        page.body->addStretch(1);
    }

    {
        const ToolPage page = beginPage(
            icons::Icon::Timer, tr("Unix Timestamp"), tr("Unix Timestamp"),
            tr("Секунды с 01.01.1970 UTC — в дату и обратно, с выбором зоны."));
        m_unixInput = new QLineEdit(page.card);
        m_unixInput->setPlaceholderText(tr("Секунды с 01.01.1970 UTC"));
        m_unixInput->setValidator(new QRegularExpressionValidator(
            QRegularExpression(QStringLiteral("^-?[0-9]{0,18}$")), m_unixInput));
        m_unixInput->setText(QString::number(now.toSecsSinceEpoch()));
        m_unixInput->setFont(fonts::digitFont(m_unixInput->font().pointSizeF()));
        m_unixZone = new QComboBox(page.card);
        populateZones(m_unixZone);
        selectZone(m_unixZone, QString());
        m_unixDate = dateTimeField(page.card, now);
        addField(page.body, tr("Timestamp"), m_unixInput, page.card, 260);
        addField(page.body, tr("Зона"), m_unixZone, page.card, 340);
        addField(page.body, tr("Дата и время"), m_unixDate, page.card, 260);
        addHint(page.body, tr("Timestamp и дата независимы: первое переводится в дату, "
                              "второе — обратно в секунды."),
                page.card);
        m_unixResult = addResultPanel(page.body, page.card);
        page.body->addStretch(1);
    }

    {
        const ToolPage page = beginPage(
            icons::Icon::Sun, tr("Сейчас"), tr("Сейчас"),
            tr("Локальное время, UTC и ISO 8601 — обновляются каждую секунду."));
        auto* panel = new QFrame(page.card);
        panel->setObjectName(QStringLiteral("ToolResult"));
        auto* panelLayout = new QVBoxLayout(panel);
        panelLayout->setContentsMargins(18, 16, 18, 16);
        panelLayout->setSpacing(6);

        m_nowClock = new QLabel(panel);
        m_nowClock->setObjectName(QStringLiteral("HeroTime"));
        m_nowClock->setTextFormat(Qt::PlainText);
        m_nowClock->setTextInteractionFlags(Qt::TextSelectableByMouse);
        m_nowDate = new QLabel(panel);
        m_nowDate->setObjectName(QStringLiteral("Muted"));
        m_nowDate->setWordWrap(true);
        panelLayout->addWidget(m_nowClock);
        panelLayout->addWidget(m_nowDate);
        panelLayout->addWidget(makeHorizontalRule(panel));

        auto* stats = new QHBoxLayout;
        stats->setSpacing(10);

        auto* utcBox = makeZoneBox(panel, tr("UTC"));
        m_nowUtc = new QLabel(utcBox);
        m_nowUtc->setObjectName(QStringLiteral("Mono"));
        m_nowUtc->setTextInteractionFlags(Qt::TextSelectableByMouse);
        utcBox->layout()->addWidget(m_nowUtc);
        stats->addWidget(utcBox, 1);

        auto* isoBox = makeZoneBox(panel, tr("ISO 8601 / RFC 3339"));
        m_nowIso = new QLabel(isoBox);
        m_nowIso->setObjectName(QStringLiteral("Mono"));
        m_nowIso->setWordWrap(true);
        m_nowIso->setTextInteractionFlags(Qt::TextSelectableByMouse);
        isoBox->layout()->addWidget(m_nowIso);
        stats->addWidget(isoBox, 2);

        panelLayout->addLayout(stats);
        page.body->addWidget(panel);
        page.body->addStretch(1);
    }

    {
        const ToolPage page = beginPage(
            icons::Icon::Navigation, tr("Пересечение часов"), tr("Пересечение рабочих часов"),
            tr("Общие рабочие окна двух часовых поясов на неделю вперёд."));
        auto* zones = new QHBoxLayout;
        zones->setSpacing(10);

        auto* boxA = makeZoneBox(page.card, tr("ЗОНА A"));
        m_overlapZoneA = new QComboBox(boxA);
        populateZones(m_overlapZoneA);
        selectZone(m_overlapZoneA, QStringLiteral("Europe/Moscow"));
        m_overlapZoneA->setMinimumWidth(250);
        boxA->layout()->addWidget(m_overlapZoneA);
        auto* hoursA = new QHBoxLayout;
        hoursA->setSpacing(6);
        hoursA->addWidget(caption(tr("Рабочие часы"), boxA, 96));
        m_overlapStartA = timeField(boxA, QTime(9, 0));
        m_overlapEndA = timeField(boxA, QTime(18, 0));
        m_overlapStartA->setFixedWidth(100);
        m_overlapEndA->setFixedWidth(100);
        hoursA->addWidget(m_overlapStartA);
        hoursA->addWidget(new QLabel(QStringLiteral("—"), boxA));
        hoursA->addWidget(m_overlapEndA);
        hoursA->addStretch(1);
        static_cast<QVBoxLayout*>(boxA->layout())->addLayout(hoursA);
        zones->addWidget(boxA, 1);

        auto* boxB = makeZoneBox(page.card, tr("ЗОНА B"));
        m_overlapZoneB = new QComboBox(boxB);
        populateZones(m_overlapZoneB);
        selectZone(m_overlapZoneB, QStringLiteral("America/New_York"));
        m_overlapZoneB->setMinimumWidth(250);
        boxB->layout()->addWidget(m_overlapZoneB);
        auto* hoursB = new QHBoxLayout;
        hoursB->setSpacing(6);
        hoursB->addWidget(caption(tr("Рабочие часы"), boxB, 96));
        m_overlapStartB = timeField(boxB, QTime(9, 0));
        m_overlapEndB = timeField(boxB, QTime(17, 0));
        m_overlapStartB->setFixedWidth(100);
        m_overlapEndB->setFixedWidth(100);
        hoursB->addWidget(m_overlapStartB);
        hoursB->addWidget(new QLabel(QStringLiteral("—"), boxB));
        hoursB->addWidget(m_overlapEndB);
        hoursB->addStretch(1);
        static_cast<QVBoxLayout*>(boxB->layout())->addLayout(hoursB);
        zones->addWidget(boxB, 1);

        page.body->addLayout(zones);
        m_overlapDate = dateField(page.card, QDate::currentDate());
        addField(page.body, tr("Начиная с даты"), m_overlapDate, page.card, 210);
        addHint(page.body, tr("Показаны окна на выбранную дату и следующие 6 дней."),
                page.card);
        m_overlapResult = addResultPanel(page.body, page.card);
        page.body->addStretch(1);
    }
}

void DateToolsDialog::connectInputs()
{
    for (auto* w : findChildren<QDateEdit*>()) {
        connect(w, &QDateEdit::dateChanged, this, [this] {
            recalculateDates();
            recalculateTime();
        });
    }
    for (auto* w : findChildren<QTimeEdit*>()) {
        connect(w, &QTimeEdit::timeChanged, this, [this] {
            recalculateDates();
            recalculateTime();
        });
    }
    for (auto* w : findChildren<QDateTimeEdit*>()) {
        if (qobject_cast<QDateEdit*>(w) != nullptr || qobject_cast<QTimeEdit*>(w) != nullptr) {
            continue;
        }
        connect(w, &QDateTimeEdit::dateTimeChanged, this, [this] {
            recalculateDates();
            recalculateTime();
        });
    }
    for (auto* w : findChildren<QSpinBox*>()) {
        connect(w, &QSpinBox::valueChanged, this, [this] { recalculateDates(); });
    }
    for (auto* w : findChildren<QComboBox*>()) {
        connect(w, &QComboBox::currentIndexChanged, this, [this] {
            recalculateDates();
            recalculateTime();
        });
    }
    if (m_unixInput != nullptr) {
        connect(m_unixInput, &QLineEdit::textChanged, this, [this] { recalculateTime(); });
    }
}

void DateToolsDialog::recalculateDates()
{
    if (m_diffFrom == nullptr) {
        return;
    }
    const QDate from = m_diffFrom->date();
    const QDate to = m_diffTo->date();
    const datemath::DateSpan span = datemath::dateDifference(from, to);
    const qint64 totalDays = std::abs(from.daysTo(to));
    if (from == to) {
        setToolResult(m_diffResult, tr("Даты совпадают"),
                  tr("Точка отсчёта и выбранная дата — один и тот же день."));
    } else {
        const QString direction = to > from ? tr("Вторая дата позже первой")
                                            : tr("Вторая дата раньше первой");
        setToolResult(m_diffResult, dateSpanHeadline(span),
                  tr("%1\nВсего: %2 · %3 нед. %4 дн.")
                      .arg(direction, daysText(static_cast<int>(totalDays)))
                      .arg(totalDays / 7)
                      .arg(totalDays % 7));
    }

    const int unit = m_addUnit->currentData().toInt();
    const int amount = m_addAmount->value();
    QDate added;
    switch (unit) {
    case 1:
        added = m_addBase->date().addDays(amount * 7);
        break;
    case 2:
        added = m_addBase->date().addMonths(amount);
        break;
    case 3:
        added = m_addBase->date().addYears(amount);
        break;
    case 4:
        added = datemath::addBusinessDays(m_addBase->date(), amount);
        break;
    default:
        added = m_addBase->date().addDays(amount);
        break;
    }
    const QString sign = amount > 0 ? QStringLiteral("+") : QString();
    const QString shift = QStringLiteral("%1%2 %3")
                              .arg(sign)
                              .arg(amount)
                              .arg(m_addUnit->currentText());
    if (!added.isValid()) {
        setToolResult(m_addResult, tr("Нет результата"), tr("Проверьте исходную дату."));
    } else {
        setToolResult(m_addResult, formattedDate(added),
                  tr("%1\nТочка отсчёта: %2").arg(shift, formattedDate(m_addBase->date())));
    }

    const int businessDays = m_bizDays->value();
    const QDate businessDate = datemath::addBusinessDays(m_bizBase->date(), businessDays);
    setToolResult(m_bizResult, formattedDate(businessDate),
              tr("Рабочих дней: %1\nБазис: %2\nТолько понедельник–пятница, без праздников.")
                  .arg(businessDays)
                  .arg(formattedDate(m_bizBase->date())));

    const QDate info = m_infoDate->date();
    const QDate firstMonth = datemath::firstDayOfMonth(info);
    const QDate lastMonth = datemath::lastDayOfMonth(info);
    const QDate firstQuarter = datemath::firstDayOfQuarter(info);
    const QDate lastQuarter = datemath::lastDayOfQuarter(info);
    const QString monthRange = QStringLiteral("%1 — %2")
                                   .arg(firstMonth.toString(QStringLiteral("dd.MM.yyyy")),
                                        lastMonth.toString(QStringLiteral("dd.MM.yyyy")));
    const QString quarterRange = QStringLiteral("%1 — %2")
                                     .arg(firstQuarter.toString(QStringLiteral("dd.MM.yyyy")),
                                          lastQuarter.toString(QStringLiteral("dd.MM.yyyy")));
    setToolResult(m_infoResult, datemath::isoWeekText(info),
              tr("%1\nДень года: %2 из %3 · до конца года %4 дн.\nМесяц: %5\nКвартал: %6\n"
                 "%7 г. · %8")
                  .arg(formattedDate(info))
                  .arg(datemath::dayOfYear(info))
                  .arg(info.daysInYear())
                  .arg(datemath::daysToEndOfYear(info))
                  .arg(monthRange, quarterRange)
                  .arg(info.year())
                  .arg(QDate::isLeapYear(info.year()) ? tr("високосный")
                                                      : tr("невисокосный")));

    const QDate birth = m_birthDate->date();
    const QDate onDate = m_ageDate->date();
    if (onDate < birth) {
        setToolResult(m_ageResult, tr("Дата раньше рождения"),
                  tr("Укажите дату не раньше дня рождения."));
    } else {
        const datemath::DateSpan age = datemath::dateDifference(birth, onDate);
        setToolResult(m_ageResult, dateSpanHeadline(age),
                  tr("Всего дней: %1\nДо следующего дня рождения: %2")
                      .arg(std::abs(birth.daysTo(onDate)))
                      .arg(std::abs(onDate.daysTo(birth.addYears(age.years + 1)))));
    }

    const QDateTime now = QDateTime::currentDateTime();
    const QDateTime event = m_eventAt->dateTime();
    const bool eventFuture = event > now;
    setToolResult(m_eventResult, spanToText(eventFuture ? now : event, eventFuture ? event : now),
              eventFuture ? tr("Событие ещё не наступило.")
                          : tr("Событие в прошлом · всего %1 ч.")
                                .arg(static_cast<double>(event.secsTo(now)) / 3600.0, 0, 'f',
                                     1));

    const QDateTime target = m_countdownAt->dateTime();
    const bool countdownFuture = target >= now;
    setToolResult(m_countdownResult,
              spanToText(countdownFuture ? now : target, countdownFuture ? target : now),
              countdownFuture
                  ? tr("До события · %1 дн.")
                        .arg(static_cast<double>(now.secsTo(target)) / 86'400.0, 0, 'f', 1)
                  : tr("Событие уже прошло."));

    const QDate rangeStart = m_rangeFrom->date();
    const QDate rangeEnd = m_rangeTo->date();
    const qint64 rangeDays = std::abs(rangeStart.daysTo(rangeEnd)) + 1;
    QStringList weekdayCounts;
    for (int day = Qt::Monday; day <= Qt::Sunday; ++day) {
        weekdayCounts.append(QStringLiteral("%1 %2")
                                 .arg(catalog::dayShort(static_cast<Qt::DayOfWeek>(day)))
                                 .arg(datemath::countWeekdayInclusive(
                                     rangeStart, rangeEnd, static_cast<Qt::DayOfWeek>(day))));
    }
    setToolResult(m_rangeResult, daysText(static_cast<int>(rangeDays)),
              tr("Рабочих: %1 · выходных: %2\n%3")
                  .arg(datemath::countBusinessDaysInclusive(rangeStart, rangeEnd))
                  .arg(datemath::countWeekendDaysInclusive(rangeStart, rangeEnd))
                  .arg(weekdayCounts.join(QStringLiteral(" · "))));
}

void DateToolsDialog::recalculateTime()
{
    if (m_momentA == nullptr) {
        return;
    }
    const QDateTime a = wallTime(m_momentA, m_momentZoneA);
    const QDateTime b = wallTime(m_momentB, m_momentZoneB);
    if (!a.isValid() || !b.isValid()) {
        setToolResult(m_momentResult, tr("Нет данных"),
                  tr("Проверьте дату и часовой пояс."));
    } else {
        const qint64 seconds = std::abs(a.secsTo(b));
        setToolResult(m_momentResult, spanToText(a, b),
                  tr("A: %1 · %2\nB: %3 · %4\nВсего: %5 ч. · %6 мин · %7 с")
                      .arg(formattedDateTime(a.toTimeZone(zoneOf(m_momentZoneA))),
                           offsetText(a.offsetFromUtc()))
                      .arg(formattedDateTime(b.toTimeZone(zoneOf(m_momentZoneB))),
                           offsetText(b.offsetFromUtc()))
                      .arg(seconds / 3'600)
                      .arg(seconds / 60)
                      .arg(seconds));
    }

    const QDateTime sourceWall = wallTime(m_convertAt, m_convertFrom);
    if (!sourceWall.isValid()) {
        setToolResult(m_convertResult, tr("Нет времени"),
                  tr("Это локальное время не существует из-за перехода DST."));
    } else {
        const QTimeZone fromZone = zoneOf(m_convertFrom);
        const QTimeZone toZone = zoneOf(m_convertTo);
        const QDateTime target = sourceWall.toTimeZone(toZone);
        QString detail = tr("%1 → %2\nUTC: %3 · смещение цели %4\nDST: %5 · источник %6\n"
                            "ISO 8601 / RFC 3339: %7\nЛокальное время источника: %8")
                             .arg(m_convertFrom->currentText(), m_convertTo->currentText())
                             .arg(formattedDateTime(sourceWall.toUTC()),
                                  offsetText(target.offsetFromUtc()))
                             .arg(dstText(target, toZone),
                                  offsetText(sourceWall.offsetFromUtc()))
                             .arg(isoText(target))
                             .arg(localDateTimeText(sourceWall));
        const QTime raw = m_convertAt->time();
        const QTime clean(raw.hour(), raw.minute(), raw.second());
        if (sourceWall.time() != clean) {
            detail += tr("\nВнимание: это локальное время не существует из-за перехода DST.");
        }
        setToolResult(m_convertResult, formattedDateTime(target), detail);
    }

    const QString unixText = m_unixInput->text().trimmed();
    QString headline;
    QString detail;
    if (unixText.isEmpty()) {
        headline = tr("Введите timestamp");
        detail = tr("Секунды с 01.01.1970 UTC.");
    } else {
        bool ok = false;
        const qint64 seconds = unixText.toLongLong(&ok);
        if (!ok) {
            headline = tr("Некорректный timestamp");
            detail = tr("Нужно целое число секунд Unix timestamp.");
        } else {
            const QDateTime unixDate = QDateTime::fromSecsSinceEpoch(seconds, zoneOf(m_unixZone));
            if (unixDate.isValid()) {
                headline = formattedDateTime(unixDate);
                detail = tr("Зона: %1\nUTC: %2\nISO 8601: %3")
                             .arg(m_unixZone->currentText(),
                                  formattedDateTime(unixDate.toUTC()), isoText(unixDate));
            } else {
                headline = tr("Вне диапазона");
                detail = tr("Timestamp вне допустимого диапазона дат.");
            }
        }
    }
    const QDateTime reverseDate = wallTime(m_unixDate, m_unixZone);
    if (reverseDate.isValid()) {
        detail += tr("\n\nОбратно: %1 с · %2 мс")
                      .arg(reverseDate.toSecsSinceEpoch())
                      .arg(reverseDate.toMSecsSinceEpoch());
    } else {
        detail += tr("\n\nОбратная конвертация: проверьте дату и зону.");
    }
    setToolResult(m_unixResult, headline, detail);

    const QTimeZone zoneA = zoneOf(m_overlapZoneA);
    const QTimeZone zoneB = zoneOf(m_overlapZoneB);
    const QDate baseDate = m_overlapDate->date();
    const QString hourFormat = Settings::instance().use24h() ? QStringLiteral("HH:mm")
                                                             : QStringLiteral("h:mm AP");
    QStringList overlapLines;
    int overlapDays = 0;
    for (int offset = 0; offset < 7; ++offset) {
        const QDate date = baseDate.addDays(offset);
        auto startA = QDateTime(date, m_overlapStartA->time(), zoneA);
        auto endA = QDateTime(date, m_overlapEndA->time(), zoneA);
        auto startB = QDateTime(date, m_overlapStartB->time(), zoneB);
        auto endB = QDateTime(date, m_overlapEndB->time(), zoneB);
        if (m_overlapEndA->time() <= m_overlapStartA->time() && endA.isValid()) {
            endA = endA.addDays(1);
        }
        if (m_overlapEndB->time() <= m_overlapStartB->time() && endB.isValid()) {
            endB = endB.addDays(1);
        }
        if (!startA.isValid() || !endA.isValid() || !startB.isValid() || !endB.isValid()) {
            overlapLines.append(tr("%1 · некорректные рабочие часы").arg(formattedDate(date)));
            continue;
        }
        const QDateTime start = std::max(startA.toUTC(), startB.toUTC());
        const QDateTime end = std::min(endA.toUTC(), endB.toUTC());
        if (start < end) {
            ++overlapDays;
            const QDateTime localA = start.toTimeZone(zoneA);
            const QDateTime localB = start.toTimeZone(zoneB);
            overlapLines.append(
                tr("%1 · %2–%3 у A · %4–%5 у B · UTC %6–%7")
                    .arg(formattedDate(date), localA.time().toString(hourFormat),
                         end.toTimeZone(zoneA).time().toString(hourFormat),
                         localB.time().toString(hourFormat),
                         end.toTimeZone(zoneB).time().toString(hourFormat),
                         start.toUTC().time().toString(QStringLiteral("HH:mm")),
                         end.toUTC().time().toString(QStringLiteral("HH:mm"))));
        } else {
            overlapLines.append(tr("%1 · пересечения нет").arg(formattedDate(date)));
        }
    }
    setToolResult(m_overlapResult, overlapDays > 0 ? tr("%1 из 7 дней").arg(overlapDays)
                                               : tr("Пересечений нет"),
              overlapLines.join(QLatin1Char('\n')));
}

void DateToolsDialog::updateLive()
{
    const QDateTime now = QDateTime::currentDateTime();
    const QString clock = now.time().toString(timeFormat());
    if (m_headerClock != nullptr) {
        m_headerClock->setText(clock);
    }
    if (m_headerDate != nullptr) {
        m_headerDate->setText(tr("%1 · %2 · %3")
                                  .arg(catalog::dayShort(
                                           static_cast<Qt::DayOfWeek>(now.date().dayOfWeek())),
                                       now.date().toString(QStringLiteral("dd.MM.yyyy")),
                                       offsetText(now.offsetFromUtc())));
    }
    if (m_nowClock != nullptr) {
        m_nowClock->setText(clock);
    }
    if (m_nowDate != nullptr) {
        m_nowDate->setText(
            tr("%1, %2 %3 %4 · %5")
                .arg(catalog::dayLong(static_cast<Qt::DayOfWeek>(now.date().dayOfWeek())))
                .arg(now.date().day())
                .arg(catalog::monthNameGenitive(now.date().month()))
                .arg(now.date().year())
                .arg(offsetText(now.offsetFromUtc())));
    }
    if (m_nowUtc != nullptr) {
        m_nowUtc->setText(formattedDateTime(now.toUTC()));
    }
    if (m_nowIso != nullptr) {
        m_nowIso->setText(isoText(now));
    }
    recalculateDates();
}

} // namespace pac::ui
