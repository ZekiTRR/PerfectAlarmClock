#include "WorldClockPage.h"

#include "core/Catalog.h"
#include "core/Fonts.h"
#include "core/Settings.h"
#include "core/Store.h"
#include "core/Theme.h"

#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QEvent>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QTableWidget>
#include <QTimer>
#include <QAbstractItemView>
#include <QTimeZone>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>
#include <functional>

namespace pac::ui {
namespace {

[[nodiscard]] QString formatUtcOffset(int seconds)
{
    const QChar sign = seconds < 0 ? QChar('-') : QChar('+');
    const int absSeconds = std::abs(seconds);
    const int hours = absSeconds / 3600;
    const int minutes = (absSeconds % 3600) / 60;
    return QStringLiteral("UTC%1%2:%3")
        .arg(sign)
        .arg(hours, 2, 10, QLatin1Char('0'))
        .arg(minutes, 2, 10, QLatin1Char('0'));
}

[[nodiscard]] QString formatDifference(int seconds)
{
    if (seconds == 0) {
        return WorldClockPage::tr("Как у вас");
    }
    const QChar sign = seconds < 0 ? QChar('-') : QChar('+');
    const int absSeconds = std::abs(seconds);
    const int hours = absSeconds / 3600;
    const int minutes = (absSeconds % 3600) / 60;
    if (minutes == 0) {
        return WorldClockPage::tr("%1%2 ч").arg(sign).arg(hours);
    }
    return WorldClockPage::tr("%1%2:%3").arg(sign).arg(hours).arg(minutes, 2, 10, QLatin1Char('0'));
}

[[nodiscard]] QString cityCount(int count)
{
    const int mod10 = count % 10;
    const int mod100 = count % 100;
    if (mod10 == 1 && mod100 != 11) {
        return WorldClockPage::tr("%1 город").arg(count);
    }
    if (mod10 >= 2 && mod10 <= 4 && (mod100 < 12 || mod100 > 14)) {
        return WorldClockPage::tr("%1 города").arg(count);
    }
    return WorldClockPage::tr("%1 городов").arg(count);
}

} // namespace

class AnalogClock final : public QWidget {
public:
    explicit AnalogClock(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setFixedSize(92, 92);
    }

    void setTime(const QTime& time, const QColor& accent)
    {
        m_time = time;
        m_accent = accent;
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event)
        const ThemeColors& colors = ThemeManager::instance().current();
        QPainter painter(this);
        icons::paintClockFace(painter, QRectF(rect()).adjusted(2, 2, -2, -2), m_time, m_accent,
                              colors.card, tinted(colors.dim, 0.65), colors.text, true,
                              m_accent);
    }

private:
    QTime m_time{QTime::currentTime()};
    QColor m_accent;
};

class ClockCard final : public QFrame {
public:
    ClockCard(const QString& city, const QString& country, const QString& zoneId, bool local,
              const QColor& accent, QWidget* parent = nullptr)
        : QFrame(parent)
        , m_zoneId(zoneId)
        , m_local(local)
        , m_accent(accent)
    {
        setObjectName(QStringLiteral("Card"));
        setFrameShape(QFrame::NoFrame);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setMinimumWidth(280);
        auto* layout = new QVBoxLayout(this);
        m_layout = layout;

        auto* titleRow = new QHBoxLayout;
        titleRow->setSpacing(6);
        auto* dot = new QLabel(this);
        dot->setFixedSize(8, 8);
        dot->setStyleSheet(QStringLiteral("background:%1; border-radius:4px;").arg(accent.name()));
        titleRow->addWidget(dot);
        m_city = new QLabel(this);
        QFont cityFont = m_city->font();
        cityFont.setPointSizeF(11.0);
        cityFont.setWeight(QFont::DemiBold);
        m_city->setFont(cityFont);
        titleRow->addWidget(m_city);
        titleRow->addStretch(1);
        if (!local) {
            m_remove = new ThemedToolButton(icons::Icon::Close, tr("Удалить город"), this);
            m_remove->setFixedSize(22, 22);
            m_remove->setIconSizePx(12);
            m_remove->setGlyphColor(ThemeManager::instance().current().faint);
            titleRow->addWidget(m_remove);
        }
        layout->addLayout(titleRow);

        m_country = new QLabel(this);
        m_country->setObjectName(QStringLiteral("Muted"));
        layout->addWidget(m_country);

        auto* timeRow = new QHBoxLayout;
        timeRow->setSpacing(10);
        m_clock = new AnalogClock(this);
        timeRow->addWidget(m_clock);
        auto* timeColumn = new QVBoxLayout;
        timeColumn->setSpacing(0);
        m_dayPeriod = new QLabel(this);
        m_dayPeriod->setObjectName(QStringLiteral("Muted"));
        timeColumn->addWidget(m_dayPeriod);
        m_time = new QLabel(this);
        m_time->setObjectName(QStringLiteral("Mono"));
        m_time->setFont(fonts::digitFont(22.0, QFont::Light));
        timeColumn->addWidget(m_time);
        m_date = new QLabel(this);
        m_date->setObjectName(QStringLiteral("Muted"));
        timeColumn->addWidget(m_date);
        timeRow->addLayout(timeColumn, 1);
        layout->addLayout(timeRow);

        auto* footer = new QHBoxLayout;
        footer->setSpacing(6);
        m_difference = new QLabel(this);
        m_difference->setAlignment(Qt::AlignCenter);
        m_difference->setFixedHeight(22);
        footer->addWidget(m_difference);
        footer->addStretch(1);
        m_utc = new QLabel(this);
        m_utc->setObjectName(QStringLiteral("Faint"));
        footer->addWidget(m_utc);
        layout->addLayout(footer);

        m_city->setText(city);
        m_country->setText(country);
        m_difference->setStyleSheet(
            QStringLiteral("background: rgba(%1,%2,%3,30); border-radius: 9px; padding: 0 9px;"
                           " color: %4;")
                .arg(m_accent.red())
                .arg(m_accent.green())
                .arg(m_accent.blue())
                .arg(m_accent.name()));
        m_difference->setVisible(!m_local);

        if (m_remove != nullptr) {
            connect(m_remove, &QToolButton::clicked, this, [this] {
                if (m_onRemove != nullptr) {
                    m_onRemove();
                }
            });
        }
        connect(&Settings::instance(), &Settings::changed, this, [this] { applyDensity(); });
        applyDensity();
    }

    void applyDensity()
    {
        if (m_layout == nullptr) {
            return;
        }
        const int density = Settings::instance().rowDensity();
        m_layout->setContentsMargins(density == 0 ? 13 : (density == 2 ? 22 : 18),
                                     density == 0 ? 10 : (density == 2 ? 18 : 14),
                                     density == 0 ? 12 : (density == 2 ? 20 : 16),
                                     density == 0 ? 10 : (density == 2 ? 18 : 14));
        m_layout->setSpacing(density == 0 ? 5 : (density == 2 ? 11 : 8));
        if (m_clock != nullptr) {
            const int clockSize = density == 0 ? 78 : (density == 2 ? 104 : 92);
            m_clock->setFixedSize(clockSize, clockSize);
        }
        if (m_time != nullptr) {
            m_time->setFont(fonts::digitFont(density == 0 ? 19.0 : (density == 2 ? 25.0 : 22.0),
                                             QFont::Light));
        }
        updateGeometry();
        update();
    }

    void setRemoveCallback(std::function<void()> callback)
    {
        m_onRemove = std::move(callback);
    }

    void updateTime(const QDateTime& now)
    {
        QTimeZone zone(m_zoneId.toUtf8());
        QDateTime cityTime = now;
        if (zone.isValid()) {
            cityTime = now.toUTC().toTimeZone(zone);
        }
        const QTime time = cityTime.time();
        m_clock->setTime(time, m_accent);
        const bool use24h = Settings::instance().use24h();
        const bool seconds = Settings::instance().showSecondsInClocks();
        m_time->setText(time.toString(use24h ? (seconds ? QStringLiteral("HH:mm:ss")
                                                       : QStringLiteral("HH:mm"))
                                             : (seconds ? QStringLiteral("h:mm:ss AP")
                                                        : QStringLiteral("h:mm AP"))));
        m_dayPeriod->setText(time.hour() >= 6 && time.hour() < 19 ? tr("День") : tr("Ночь"));

        const QDate date = cityTime.date();
        const QDate localDate = now.date();
        QString relative;
        if (date == localDate.addDays(1)) {
            relative = QStringLiteral(" · ") + tr("завтра");
        } else if (date == localDate.addDays(-1)) {
            relative = QStringLiteral(" · ") + tr("вчера");
        }
        m_date->setText(tr("%1, %2 %3%4")
                            .arg(catalog::dayLong(static_cast<Qt::DayOfWeek>(date.dayOfWeek())))
                            .arg(date.day())
                            .arg(catalog::monthNameGenitive(date.month()))
                            .arg(relative));

        const int offset = zone.isValid() ? zone.offsetFromUtc(cityTime.toUTC()) : 0;
        const int localOffset = now.offsetFromUtc();
        const int difference = offset - localOffset;
        m_difference->setText(formatDifference(difference));
        m_utc->setText(formatUtcOffset(offset));
    }

private:
    QString m_zoneId;
    bool m_local{false};
    QColor m_accent;
    QVBoxLayout* m_layout{nullptr};
    QLabel* m_city{nullptr};
    QLabel* m_country{nullptr};
    QLabel* m_time{nullptr};
    QLabel* m_date{nullptr};
    QLabel* m_dayPeriod{nullptr};
    QLabel* m_difference{nullptr};
    QLabel* m_utc{nullptr};
    AnalogClock* m_clock{nullptr};
    ThemedToolButton* m_remove{nullptr};
    std::function<void()> m_onRemove;
};

namespace {

class CityDialog final : public QDialog {
public:
    CityDialog(const QStringList& existingZones, QWidget* parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle(tr("Добавить город"));
        resize(560, 560);
        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(14, 14, 14, 12);
        layout->setSpacing(8);
        m_search = new QLineEdit(this);
        m_search->setPlaceholderText(tr("Поиск города или страны…"));
        m_search->setClearButtonEnabled(true);
        layout->addWidget(m_search);
        m_table = new QTableWidget(this);
        m_table->setColumnCount(3);
        m_table->setHorizontalHeaderLabels({tr("ГОРОД"), tr("СТРАНА"), tr("ВРЕМЯ")});
        m_table->verticalHeader()->setVisible(false);
        m_table->setShowGrid(false);
        m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_table->setSelectionMode(QAbstractItemView::SingleSelection);
        m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_table->setAlternatingRowColors(true);
        m_table->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
        m_table->setColumnWidth(2, 90);
        const QDateTime now = QDateTime::currentDateTime();
        for (const auto& city : catalog::cities()) {
            if (existingZones.contains(city.timeZoneId + QLatin1Char('|') + city.city)) {
                continue;
            }
            const int row = m_table->rowCount();
            m_table->insertRow(row);
            auto* cityItem = new QTableWidgetItem(city.city);
            cityItem->setData(Qt::UserRole, city.city);
            cityItem->setData(Qt::UserRole + 1, city.country);
            cityItem->setData(Qt::UserRole + 2, city.timeZoneId);
            auto* countryItem = new QTableWidgetItem(city.country);
            const QTimeZone zone(city.timeZoneId.toUtf8());
            const QDateTime local = now.toUTC().toTimeZone(zone);
            auto* timeItem = new QTableWidgetItem(
                local.toString(Settings::instance().use24h() ? QStringLiteral("HH:mm")
                                                              : QStringLiteral("h:mm AP")));
            timeItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            m_table->setItem(row, 0, cityItem);
            m_table->setItem(row, 1, countryItem);
            m_table->setItem(row, 2, timeItem);
        }
        layout->addWidget(m_table, 1);

        auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        buttons->button(QDialogButtonBox::Ok)->setText(tr("Добавить"));
        buttons->button(QDialogButtonBox::Ok)->setProperty("accent", true);
        buttons->button(QDialogButtonBox::Cancel)->setText(tr("Отмена"));
        connect(buttons, &QDialogButtonBox::accepted, this, &CityDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        layout->addWidget(buttons);
        connect(m_search, &QLineEdit::textChanged, this, [this](const QString& text) {
            filter(text);
        });
        connect(m_table, &QTableWidget::cellDoubleClicked, this, [this](int, int) { accept(); });
        filter(QString());
    }

    [[nodiscard]] QString city() const { return m_city; }
    [[nodiscard]] QString country() const { return m_country; }
    [[nodiscard]] QString timeZoneId() const { return m_zoneId; }

protected:
    void accept() override
    {
        const int row = m_table->currentRow();
        if (row < 0 || m_table->isRowHidden(row)) {
            return;
        }
        m_city = m_table->item(row, 0)->data(Qt::UserRole).toString();
        m_country = m_table->item(row, 0)->data(Qt::UserRole + 1).toString();
        m_zoneId = m_table->item(row, 0)->data(Qt::UserRole + 2).toString();
        QDialog::accept();
    }

private:
    void filter(const QString& text)
    {
        const QString query = text.trimmed();
        for (int row = 0; row < m_table->rowCount(); ++row) {
            const bool matches = query.isEmpty()
                                 || m_table->item(row, 0)->text().contains(query, Qt::CaseInsensitive)
                                 || m_table->item(row, 1)->text().contains(query, Qt::CaseInsensitive);
            m_table->setRowHidden(row, !matches);
        }
        for (int row = 0; row < m_table->rowCount(); ++row) {
            if (!m_table->isRowHidden(row)) {
                m_table->selectRow(row);
                break;
            }
        }
    }

    QLineEdit* m_search{nullptr};
    QTableWidget* m_table{nullptr};
    QString m_city;
    QString m_country;
    QString m_zoneId;
};

} // namespace

WorldClockPage::WorldClockPage(Store& store, QWidget* parent)
    : PageWidget(parent)
    , m_store(store)
{
    buildUi();
    rebuildCards();
    connect(&m_store, &Store::clocksChanged, this, [this] {
        QTimer::singleShot(0, this, [this] { rebuildCards(); });
    });
}

void WorldClockPage::buildUi()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(10);

    auto* toolbar = new QHBoxLayout;
    toolbar->setSpacing(8);
    auto* title = new QLabel(tr("Мировое время"), this);
    QFont titleFont = title->font();
    titleFont.setPointSizeF(12.0);
    titleFont.setWeight(QFont::DemiBold);
    title->setFont(titleFont);
    toolbar->addWidget(title);
    m_countLabel = new QLabel(this);
    m_countLabel->setObjectName(QStringLiteral("Muted"));
    toolbar->addWidget(m_countLabel);
    toolbar->addStretch(1);
    auto* addButton = new ThemedButton(tr("Добавить город"), icons::Icon::Plus, this, true);
    addButton->setMinimumHeight(28);
    connect(addButton, &QPushButton::clicked, this, &WorldClockPage::addCity);
    toolbar->addWidget(addButton);
    layout->addLayout(toolbar);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_gridHost = new QWidget(scroll);
    m_gridHost->setObjectName(QStringLiteral("Transparent"));
    m_grid = new QGridLayout(m_gridHost);
    m_grid->setContentsMargins(0, 0, 6, 0);
    m_grid->setHorizontalSpacing(10);
    m_grid->setVerticalSpacing(10);
    scroll->setWidget(m_gridHost);
    layout->addWidget(scroll, 1);
}

void WorldClockPage::rebuildCards()
{
    while (QLayoutItem* item = m_grid->takeAt(0)) {
        if (item->widget() != nullptr) {
            delete item->widget();
        }
        delete item;
    }
    m_cards.clear();
    m_lastColumns = -1;

    const QTimeZone localZone = QTimeZone::systemTimeZone();
    const QString localName = localZone.id();
    auto* local = new ClockCard(tr("Местное время"),
                                localZone.displayName(QTimeZone::StandardTime, QTimeZone::LongName),
                                localName, true, ThemeManager::instance().current().accent, m_gridHost);
    m_cards.append(local);
    const auto& palette = catalog::alarmColors();
    int index = 0;
    for (const auto& clock : m_store.data().clocks) {
        const QColor accent(palette.at((index + 2) % palette.size()));
        auto* card = new ClockCard(clock.city, clock.country, clock.timeZoneId, false, accent,
                                   m_gridHost);
        const QUuid id = clock.id;
        card->setRemoveCallback([this, id] { removeCity(id); });
        m_cards.append(card);
        ++index;
    }
    m_countLabel->setText(tr("%1 · местное время").arg(cityCount(static_cast<int>(m_store.data().clocks.size()))));
    m_lastColumns = 0;
    relayoutCards();
    refreshClock(QDateTime::currentDateTime());
    updateGeometry();
}

int WorldClockPage::columnCount() const
{
    const int width = m_gridHost != nullptr ? m_gridHost->width() : 0;
    return std::clamp(width / 300, 1, 4);
}

void WorldClockPage::relayoutCards()
{
    if (m_grid == nullptr) {
        return;
    }
    const int columns = columnCount();
    if (columns == m_lastColumns) {
        return;
    }
    m_lastColumns = columns;
    for (int column = 0; column < 4; ++column) {
        m_grid->setColumnStretch(column, column < columns ? 1 : 0);
    }
    for (int i = 0; i < m_cards.size(); ++i) {
        ClockCard* card = m_cards.at(i);
        m_grid->removeWidget(card);
        m_grid->addWidget(card, i / columns, i % columns, 1, 1);
    }
}

void WorldClockPage::resizeEvent(QResizeEvent* event)
{
    PageWidget::resizeEvent(event);
    QTimer::singleShot(0, this, &WorldClockPage::relayoutCards);
}

void WorldClockPage::refreshClock(const QDateTime& now)
{
    for (auto* card : m_cards) {
        card->updateTime(now);
    }
}

void WorldClockPage::onActivated()
{
    refreshClock(QDateTime::currentDateTime());
}

void WorldClockPage::themeUpdated()
{
    rebuildCards();
}

void WorldClockPage::addCity()
{
    QStringList existing;
    for (const auto& clock : m_store.data().clocks) {
        existing.append(clock.timeZoneId + QLatin1Char('|') + clock.city);
    }
    CityDialog dialog(existing, this);
    if (dialog.exec() != QDialog::Accepted || dialog.city().isEmpty()) {
        return;
    }
    const QString name = dialog.city();
    const QString zone = dialog.timeZoneId();
    for (const auto& clock : m_store.data().clocks) {
        if (clock.city == name && clock.timeZoneId == zone) {
            return;
        }
    }
    WorldClock clock;
    clock.city = name;
    clock.country = dialog.country();
    clock.timeZoneId = zone;
    m_store.data().clocks.push_back(clock);
    m_store.commit(Domain::Clocks);
}

void WorldClockPage::removeCity(const QUuid& id)
{
    const auto it = std::find_if(m_store.data().clocks.begin(), m_store.data().clocks.end(),
                                 [&id](const WorldClock& clock) { return clock.id == id; });
    if (it == m_store.data().clocks.end()) {
        return;
    }
    m_store.data().clocks.erase(it);
    m_store.commit(Domain::Clocks);
}

} // namespace pac::ui
