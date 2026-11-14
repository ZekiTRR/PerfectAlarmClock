#include "SettingsDialog.h"

#include "UiKit.h"
#include "core/Catalog.h"
#include "core/Fonts.h"
#include "core/Platform.h"
#include "core/Settings.h"
#include "core/Theme.h"

#include <QAbstractButton>
#include <QCheckBox>
#include <QComboBox>
#include <QDate>
#include <QDialogButtonBox>
#include <QEnterEvent>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QFont>
#include <QFontComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QGuiApplication>
#include <QScreen>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLinearGradient>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QSettings>
#include <QSignalBlocker>
#include <QScrollArea>
#include <QSpinBox>
#include <QStackedWidget>
#include <QStyle>
#include <QVBoxLayout>

#include <algorithm>

namespace pac::ui {
namespace {

[[nodiscard]] QGroupBox* makeBox(const QString& title, QWidget* parent, QVBoxLayout** layout)
{
    auto* box = new QGroupBox(title, parent);
    auto* boxLayout = new QVBoxLayout(box);
    boxLayout->setContentsMargins(14, 12, 14, 12);
    boxLayout->setSpacing(8);
    *layout = boxLayout;
    return box;
}

void addFormRow(QGridLayout* grid, int row, const QString& label, QWidget* field)
{
    auto* caption = new QLabel(label, field->parentWidget());
    caption->setObjectName(QStringLiteral("Muted"));
    grid->addWidget(caption, row, 0);
    grid->addWidget(field, row, 1);
}

class ThemeOptionButton final : public QAbstractButton {
public:
    ThemeOptionButton(const ThemeColors& colors, QWidget* parent = nullptr)
        : QAbstractButton(parent)
        , m_colors(colors)
    {
        setCheckable(true);
        setCursor(Qt::PointingHandCursor);
        setFixedSize(178, 50);
        setToolTip(colors.name);
        setProperty("themeId", colors.id);
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event)
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const QRectF rect = QRectF(this->rect()).adjusted(1.0, 1.0, -1.0, -1.0);
        QPainterPath path;
        path.addRoundedRect(rect, 5.0, 5.0);
        painter.fillPath(path, m_colors.card);
        painter.setPen(QPen(isChecked() ? m_colors.accent : m_colors.border,
                            isChecked() ? 1.6 : 1.0));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(path);

        const QRectF preview(rect.left() + 8.0, rect.top() + 8.0, 58.0, 33.0);
        QPainterPath previewPath;
        previewPath.addRoundedRect(preview, 3.0, 3.0);
        painter.fillPath(previewPath, m_colors.pageTop);
        QLinearGradient band(preview.topLeft(), QPointF(preview.left(), preview.top() + 11.0));
        band.setColorAt(0.0, m_colors.ribbonTop);
        band.setColorAt(1.0, m_colors.ribbonBottom);
        painter.fillRect(QRectF(preview.left(), preview.top(), preview.width(), 11.0), band);
        painter.fillRect(QRectF(preview.left(), preview.top() + 11.0, 17.0, preview.height() - 11.0),
                         m_colors.navTop);
        painter.setPen(Qt::NoPen);
        painter.setBrush(m_colors.accent);
        painter.drawRoundedRect(QRectF(preview.left() + 3.0, preview.top() + 14.0, 11.0, 3.0), 1.0,
                                1.0);
        painter.setBrush(m_colors.headerTop);
        painter.drawRect(QRectF(preview.left() + 20.0, preview.top() + 15.0, 35.0, 2.0));
        painter.drawRect(QRectF(preview.left() + 20.0, preview.top() + 20.0, 27.0, 2.0));
        painter.drawRect(QRectF(preview.left() + 20.0, preview.top() + 25.0, 31.0, 2.0));
        painter.setPen(QPen(m_colors.border, 1.0));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(previewPath);

        QFont nameFont = font();
        nameFont.setPointSizeF(8.2);
        nameFont.setWeight(QFont::DemiBold);
        painter.setFont(nameFont);
        painter.setPen(ThemeManager::instance().current().text);
        const QRectF textRect(rect.left() + 74.0, rect.top() + 8.0, rect.width() - 82.0, 22.0);
        painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter,
                         painter.fontMetrics().elidedText(m_colors.name, Qt::ElideRight,
                                                          static_cast<int>(textRect.width())));
        if (isChecked()) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(m_colors.accent);
            painter.drawEllipse(QPointF(rect.right() - 12.0, rect.bottom() - 10.0), 4.0, 4.0);
        }
    }

    void enterEvent(QEnterEvent* event) override
    {
        update();
        QAbstractButton::enterEvent(event);
    }

    void leaveEvent(QEvent* event) override
    {
        update();
        QAbstractButton::leaveEvent(event);
    }

private:
    ThemeColors m_colors;
};

void populateSoundCombo(QComboBox* combo, const QString& selected)
{
    for (const auto& sound : catalog::availableSounds()) {
        combo->addItem(sound.name, sound.id);
    }
    int index = combo->findData(selected);
    if (index < 0 && !selected.isEmpty()) {
        combo->addItem(catalog::soundName(selected), selected);
        index = combo->count() - 1;
    }
    combo->setCurrentIndex(index >= 0 ? index : 0);
}

} // namespace

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
    , m_themeId(Settings::instance().themeId())
{
    setWindowTitle(tr("Настройки"));
    const QScreen* targetScreen = screen() != nullptr ? screen() : QGuiApplication::primaryScreen();
    const int availableHeight = targetScreen != nullptr
                                    ? targetScreen->availableGeometry().height()
                                    : 900;
    resize(940, std::clamp(availableHeight - 80, 580, 760));
    setMinimumSize(800, 560);
    buildUi();
    readSettings();
}

void SettingsDialog::buildUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 10);
    root->setSpacing(10);

    auto* content = new QHBoxLayout;
    content->setSpacing(10);

    m_nav = new QListWidget(this);
    m_nav->setObjectName(QStringLiteral("SettingsNav"));
    m_nav->setFixedWidth(204);
    m_nav->setIconSize(QSize(18, 18));
    m_nav->setSpacing(1);
    const QColor accent = ThemeManager::instance().current().accent;
    const auto addNav = [this, &accent](icons::Icon icon, const QString& text) {
        auto* item = new QListWidgetItem(icons::qicon(icon, accent), text, m_nav);
        item->setSizeHint(QSize(0, 38));
    };
    addNav(icons::Icon::Settings, tr("Общие"));
    addNav(icons::Icon::Theme, tr("Внешний вид"));
    addNav(icons::Icon::Alarm, tr("Будильники"));
    addNav(icons::Icon::Timer, tr("Таймеры"));
    addNav(icons::Icon::Bell, tr("Уведомления"));
    addNav(icons::Icon::Calendar, tr("Календарь"));
    addNav(icons::Icon::Folder, tr("Данные"));
    content->addWidget(m_nav);

    m_pages = new QStackedWidget(this);
    m_pages->setObjectName(QStringLiteral("Transparent"));
    content->addWidget(m_pages, 1);
    root->addLayout(content, 1);

    buildGeneralPage();
    buildAppearancePage();
    buildAlarmsPage();
    buildTimersPage();
    buildNotificationsPage();
    buildCalendarPage();
    buildDataPage();
    m_nav->setCurrentRow(0);
    connect(m_nav, &QListWidget::currentRowChanged, m_pages, &QStackedWidget::setCurrentIndex);

    auto* footer = new QHBoxLayout;
    auto* defaultsButton = new QPushButton(tr("По умолчанию"), this);
    defaultsButton->setProperty("flat", true);
    connect(defaultsButton, &QPushButton::clicked, this, &SettingsDialog::resetToDefaults);
    footer->addWidget(defaultsButton);
    footer->addStretch(1);
    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply, this);
    buttons->button(QDialogButtonBox::Ok)->setText(tr("ОК"));
    buttons->button(QDialogButtonBox::Ok)->setProperty("accent", true);
    buttons->button(QDialogButtonBox::Ok)->style()->unpolish(buttons->button(QDialogButtonBox::Ok));
    buttons->button(QDialogButtonBox::Ok)->style()->polish(buttons->button(QDialogButtonBox::Ok));
    buttons->button(QDialogButtonBox::Cancel)->setText(tr("Отмена"));
    buttons->button(QDialogButtonBox::Apply)->setText(tr("Применить"));
    connect(buttons, &QDialogButtonBox::accepted, this, [this] {
        applySettings();
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttons->button(QDialogButtonBox::Apply), &QPushButton::clicked, this,
            &SettingsDialog::applySettings);
    footer->addWidget(buttons);
    root->addLayout(footer);
}

QWidget* SettingsDialog::makePage(const QString& title, QVBoxLayout** contentLayout)
{
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto* host = new QWidget(scroll);
    host->setObjectName(QStringLiteral("Transparent"));
    auto* layout = new QVBoxLayout(host);
    layout->setContentsMargins(18, 14, 18, 14);
    layout->setSpacing(10);

    auto* heading = new QLabel(title, host);
    QFont headingFont = heading->font();
    headingFont.setPointSizeF(15.0);
    headingFont.setWeight(QFont::DemiBold);
    heading->setFont(headingFont);
    layout->addWidget(heading);

    scroll->setWidget(host);
    m_pages->addWidget(scroll);
    *contentLayout = layout;
    return host;
}

void SettingsDialog::buildGeneralPage()
{
    QVBoxLayout* layout = nullptr;
    QWidget* page = makePage(tr("Общие"), &layout);

    QVBoxLayout* timeLayout = nullptr;
    auto* timeBox = makeBox(tr("Время и дата"), page, &timeLayout);
    auto* timeGrid = new QGridLayout;
    timeGrid->setHorizontalSpacing(12);
    timeGrid->setVerticalSpacing(8);
    m_timeFormat = new QComboBox(timeBox);
    m_timeFormat->addItem(tr("24 часа"), true);
    m_timeFormat->addItem(tr("12 часов (AM/PM)"), false);
    addFormRow(timeGrid, 0, tr("Формат времени"), m_timeFormat);
    m_firstDay = new QComboBox(timeBox);
    m_firstDay->addItem(tr("Понедельник"), true);
    m_firstDay->addItem(tr("Воскресенье"), false);
    addFormRow(timeGrid, 1, tr("Первый день недели"), m_firstDay);
    timeGrid->setColumnStretch(1, 1);
    timeLayout->addLayout(timeGrid);
    m_secondsTitle = new QCheckBox(tr("Показывать секунды в заголовке окна"), timeBox);
    m_secondsClocks = new QCheckBox(tr("Показывать секунды на часах мира"), timeBox);
    timeLayout->addWidget(m_secondsTitle);
    timeLayout->addWidget(m_secondsClocks);
    layout->addWidget(timeBox);

    QVBoxLayout* startupLayout = nullptr;
    auto* startupBox = makeBox(tr("Запуск и фон"), page, &startupLayout);
    m_autostart = new QCheckBox(tr("Запускать вместе с Windows"), startupBox);
    m_startMinimized = new QCheckBox(tr("Запускать свёрнутым в трей"), startupBox);
    m_closeToTray = new QCheckBox(tr("При закрытии окна оставаться в трее"), startupBox);
    m_minimizeToTray = new QCheckBox(tr("Сворачивать в трей вместо панели задач"), startupBox);
    startupLayout->addWidget(m_autostart);
    startupLayout->addWidget(m_startMinimized);
    startupLayout->addWidget(m_closeToTray);
    startupLayout->addWidget(m_minimizeToTray);
    auto* hint = new QLabel(tr("В трее приложение продолжает отслеживать будильники и таймеры."),
                            startupBox);
    hint->setObjectName(QStringLiteral("Faint"));
    hint->setWordWrap(true);
    startupLayout->addWidget(hint);
    layout->addWidget(startupBox);
    layout->addStretch(1);
}

void SettingsDialog::buildAppearancePage()
{
    QVBoxLayout* layout = nullptr;
    QWidget* page = makePage(tr("Внешний вид"), &layout);

    QVBoxLayout* themeLayout = nullptr;
    auto* themeBox = makeBox(tr("Темы Office"), page, &themeLayout);
    auto* gallery = new QWidget(themeBox);
    auto* grid = new QGridLayout(gallery);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(8);
    grid->setVerticalSpacing(8);
    int index = 0;
    for (const auto& colors : ThemeManager::instance().themes()) {
        auto* button = new ThemeOptionButton(colors, gallery);
        m_themeButtons.append(button);
        grid->addWidget(button, index / 3, index % 3);
        connect(button, &QAbstractButton::clicked, this, [this, id = colors.id] {
            m_themeId = id;
            updateThemeChecks();
        });
        ++index;
    }
    grid->setColumnStretch(3, 1);
    themeLayout->addWidget(gallery);
    layout->addWidget(themeBox);

    QVBoxLayout* interfaceLayout = nullptr;
    auto* interfaceBox = makeBox(tr("Интерфейс"), page, &interfaceLayout);
    auto* grid2 = new QGridLayout;
    grid2->setHorizontalSpacing(12);
    grid2->setVerticalSpacing(8);
    m_density = new QComboBox(interfaceBox);
    m_density->addItem(tr("Компактно"));
    m_density->addItem(tr("Обычно"));
    m_density->addItem(tr("Просторно"));
    addFormRow(grid2, 0, tr("Плотность строк"), m_density);
    grid2->setColumnStretch(1, 1);
    interfaceLayout->addLayout(grid2);
    m_showNav = new QCheckBox(tr("Показывать панель навигации"), interfaceBox);
    m_showStatus = new QCheckBox(tr("Показывать строку состояния"), interfaceBox);
    m_showGrid = new QCheckBox(tr("Показывать линии сетки в таблицах"), interfaceBox);
    m_alternateRows = new QCheckBox(tr("Чередовать цвет строк"), interfaceBox);
    interfaceLayout->addWidget(m_showNav);
    interfaceLayout->addWidget(m_showStatus);
    interfaceLayout->addWidget(m_showGrid);
    interfaceLayout->addWidget(m_alternateRows);
    layout->addWidget(interfaceBox);

    QVBoxLayout* fontLayout = nullptr;
    auto* fontBox = makeBox(tr("Шрифты"), page, &fontLayout);
    auto* fontGrid = new QGridLayout;
    fontGrid->setHorizontalSpacing(12);
    fontGrid->setVerticalSpacing(8);

    const auto addFontRow = [&](int row, const QString& label, QFontComboBox*& comboOut,
                                QString& filePath, const QString& defaultValue) {
        auto* rowWidget = new QWidget(fontBox);
        auto* rowLayout = new QHBoxLayout(rowWidget);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(8);
        auto* fontCombo = new QFontComboBox(rowWidget);
        fontCombo->setFontFilters(QFontComboBox::ScalableFonts);
        fontCombo->setMinimumWidth(260);
        comboOut = fontCombo;
        auto* loadButton = new QPushButton(tr("Файл…"), rowWidget);
        loadButton->setToolTip(tr("Загрузить шрифт из файла TTF или OTF"));
        auto* resetButton = new QPushButton(tr("Сбросить"), rowWidget);
        rowLayout->addWidget(fontCombo, 1);
        rowLayout->addWidget(loadButton);
        rowLayout->addWidget(resetButton);
        fontGrid->addWidget(new QLabel(label, fontBox), row, 0);
        fontGrid->addWidget(rowWidget, row, 1);

        connect(loadButton, &QPushButton::clicked, this, [this, fontCombo, &filePath, loadButton] {
            const QString path = QFileDialog::getOpenFileName(
                this, tr("Выберите файл шрифта"), QString(),
                tr("Шрифты (*.ttf *.otf);;Все файлы (*.*)"));
            if (path.isEmpty()) {
                return;
            }
            const QString family = fonts::loadFontFile(path);
            if (family.isEmpty()) {
                QMessageBox::warning(this, tr("Шрифт"),
                                     tr("Не удалось загрузить выбранный шрифт."));
                return;
            }
            fontCombo->setCurrentFont(QFont(family));
            filePath = path;
            loadButton->setToolTip(path);
        });
        connect(resetButton, &QPushButton::clicked, this, [this, fontCombo, &filePath, defaultValue] {
            filePath.clear();
            fontCombo->setCurrentFont(QFont(defaultValue));
        });
        connect(fontCombo, &QFontComboBox::currentFontChanged, this, [&filePath] {
            filePath.clear();
        });
    };
    addFontRow(0, tr("Интерфейс"), m_uiFontCombo, m_uiFontFile, QStringLiteral("Segoe UI"));
    addFontRow(1, tr("Цифры"), m_digitFontCombo, m_digitFontFile, QStringLiteral("Cascadia Mono"));
    fontGrid->setColumnStretch(1, 1);
    fontLayout->addLayout(fontGrid);
    auto* fontHint = new QLabel(
        tr("Шрифт интерфейса применяется ко всему приложению, шрифт цифр — к часам, "
           "таймерам, секундомеру и датам. Для полного применения перезапустите приложение."),
        fontBox);
    fontHint->setObjectName(QStringLiteral("Faint"));
    fontHint->setWordWrap(true);
    fontLayout->addWidget(fontHint);
    layout->addWidget(fontBox);
    layout->addStretch(1);
}

void SettingsDialog::buildAlarmsPage()
{
    QVBoxLayout* layout = nullptr;
    QWidget* page = makePage(tr("Будильники"), &layout);

    QVBoxLayout* soundLayout = nullptr;
    auto* soundBox = makeBox(tr("Звук по умолчанию"), page, &soundLayout);
    auto* grid = new QGridLayout;
    grid->setHorizontalSpacing(12);
    grid->setVerticalSpacing(8);
    m_alarmSound = new QComboBox(soundBox);
    populateSoundCombo(m_alarmSound, Settings::instance().defaultAlarmSound());
    addFormRow(grid, 0, tr("Сигнал"), m_alarmSound);
    m_alarmVolume = new QSpinBox(soundBox);
    m_alarmVolume->setRange(0, 100);
    m_alarmVolume->setSuffix(QStringLiteral("%"));
    addFormRow(grid, 1, tr("Громкость"), m_alarmVolume);
    m_alarmMaxSound = new QSpinBox(soundBox);
    m_alarmMaxSound->setRange(1, 120);
    m_alarmMaxSound->setSuffix(tr(" мин"));
    addFormRow(grid, 2, tr("Звучать не дольше"), m_alarmMaxSound);
    grid->setColumnStretch(1, 1);
    soundLayout->addLayout(grid);
    m_alarmLoop = new QCheckBox(tr("Повторять сигнал до отключения"), soundBox);
    m_alarmPopup = new QCheckBox(tr("Показывать окно будильника"), soundBox);
    soundLayout->addWidget(m_alarmLoop);
    soundLayout->addWidget(m_alarmPopup);
    layout->addWidget(soundBox);

    QVBoxLayout* snoozeLayout = nullptr;
    auto* snoozeBox = makeBox(tr("Откладывание"), page, &snoozeLayout);
    auto* grid2 = new QGridLayout;
    grid2->setHorizontalSpacing(12);
    grid2->setVerticalSpacing(8);
    m_snoozeMinutes = new QSpinBox(snoozeBox);
    m_snoozeMinutes->setRange(1, 180);
    m_snoozeMinutes->setSuffix(tr(" мин"));
    addFormRow(grid2, 0, tr("Откладывать на"), m_snoozeMinutes);
    m_maxSnoozes = new QSpinBox(snoozeBox);
    m_maxSnoozes->setRange(0, 20);
    m_maxSnoozes->setSuffix(tr(" раз"));
    addFormRow(grid2, 1, tr("Не более"), m_maxSnoozes);
    m_preAlert = new QSpinBox(snoozeBox);
    m_preAlert->setRange(0, 1440);
    m_preAlert->setSuffix(tr(" мин"));
    addFormRow(grid2, 2, tr("Предупреждать заранее"), m_preAlert);
    grid2->setColumnStretch(1, 1);
    snoozeLayout->addLayout(grid2);
    auto* hint = new QLabel(tr("0 минут — предупреждение отключено."), snoozeBox);
    hint->setObjectName(QStringLiteral("Faint"));
    snoozeLayout->addWidget(hint);
    layout->addWidget(snoozeBox);
    layout->addStretch(1);
}

void SettingsDialog::buildTimersPage()
{
    QVBoxLayout* layout = nullptr;
    QWidget* page = makePage(tr("Таймеры"), &layout);

    QVBoxLayout* timerLayout = nullptr;
    auto* timerBox = makeBox(tr("Новые таймеры"), page, &timerLayout);
    auto* grid = new QGridLayout;
    grid->setHorizontalSpacing(12);
    grid->setVerticalSpacing(8);
    m_timerMinutes = new QSpinBox(timerBox);
    m_timerMinutes->setRange(1, 1800);
    m_timerMinutes->setSuffix(tr(" мин"));
    addFormRow(grid, 0, tr("Длительность по умолчанию"), m_timerMinutes);
    m_timerSound = new QComboBox(timerBox);
    populateSoundCombo(m_timerSound, Settings::instance().defaultTimerSound());
    addFormRow(grid, 1, tr("Сигнал"), m_timerSound);
    m_timerVolume = new QSpinBox(timerBox);
    m_timerVolume->setRange(0, 100);
    m_timerVolume->setSuffix(QStringLiteral("%"));
    addFormRow(grid, 2, tr("Громкость"), m_timerVolume);
    grid->setColumnStretch(1, 1);
    timerLayout->addLayout(grid);
    m_timerLoop = new QCheckBox(tr("Повторять сигнал до закрытия"), timerBox);
    m_timerPopup = new QCheckBox(tr("Показывать окно завершения"), timerBox);
    timerLayout->addWidget(m_timerLoop);
    timerLayout->addWidget(m_timerPopup);
    layout->addWidget(timerBox);
    layout->addStretch(1);
}

void SettingsDialog::buildNotificationsPage()
{
    QVBoxLayout* layout = nullptr;
    QWidget* page = makePage(tr("Уведомления"), &layout);

    QVBoxLayout* notifyLayout = nullptr;
    auto* notifyBox = makeBox(tr("Напоминания и звук"), page, &notifyLayout);
    m_preAlerts = new QCheckBox(tr("Показывать напоминания о скорых будильниках"), notifyBox);
    m_muted = new QCheckBox(tr("Отключить звук всех уведомлений"), notifyBox);
    notifyLayout->addWidget(m_preAlerts);
    notifyLayout->addWidget(m_muted);
    auto* hint = new QLabel(
        tr("Все уведомления показываются собственными окнами приложения. "
           "Системные уведомления Windows не используются. Окна будильников и таймеров "
           "остаются на экране до действия пользователя."),
        notifyBox);
    hint->setObjectName(QStringLiteral("Faint"));
    hint->setWordWrap(true);
    notifyLayout->addWidget(hint);
    layout->addWidget(notifyBox);
    layout->addStretch(1);
}

void SettingsDialog::buildCalendarPage()
{
    QVBoxLayout* layout = nullptr;
    QWidget* page = makePage(tr("Календарь"), &layout);

    QVBoxLayout* calendarLayout = nullptr;
    auto* calendarBox = makeBox(tr("Отображение"), page, &calendarLayout);
    m_calendarAlarms = new QCheckBox(tr("Показывать будильники"), calendarBox);
    m_calendarTasks = new QCheckBox(tr("Показывать задачи"), calendarBox);
    m_weekNumbers = new QCheckBox(tr("Показывать номера недель"), calendarBox);
    calendarLayout->addWidget(m_calendarAlarms);
    calendarLayout->addWidget(m_calendarTasks);
    calendarLayout->addWidget(m_weekNumbers);
    auto* hint = new QLabel(tr("Первый день недели настраивается на вкладке «Общие»."), calendarBox);
    hint->setObjectName(QStringLiteral("Faint"));
    calendarLayout->addWidget(hint);
    layout->addWidget(calendarBox);
    layout->addStretch(1);
}

void SettingsDialog::buildDataPage()
{
    QVBoxLayout* layout = nullptr;
    QWidget* page = makePage(tr("Данные"), &layout);

    QVBoxLayout* pathsLayout = nullptr;
    auto* pathsBox = makeBox(tr("Расположение файлов"), page, &pathsLayout);
    auto* dataGrid = new QGridLayout;
    dataGrid->setHorizontalSpacing(12);
    dataGrid->setVerticalSpacing(8);
    m_dataPath = new QLineEdit(pathsBox);
    m_dataPath->setReadOnly(true);
    m_settingsPath = new QLineEdit(pathsBox);
    m_settingsPath->setReadOnly(true);
    addFormRow(dataGrid, 0, tr("База данных"), m_dataPath);
    addFormRow(dataGrid, 1, tr("Настройки"), m_settingsPath);
    dataGrid->setColumnStretch(1, 1);
    pathsLayout->addLayout(dataGrid);
    auto* buttons = new QHBoxLayout;
    auto* openButton = new QPushButton(tr("Открыть папку данных"), pathsBox);
    connect(openButton, &QPushButton::clicked, this, [this] {
        platform::revealInExplorer(m_dataPath->text());
    });
    buttons->addWidget(openButton);
    buttons->addStretch(1);
    pathsLayout->addLayout(buttons);
    layout->addWidget(pathsBox);

    QVBoxLayout* backupLayout = nullptr;
    auto* backupBox = makeBox(tr("Резервная копия"), page, &backupLayout);
    auto* hint = new QLabel(tr("Сохраните копию данных перед переустановкой или переносом "
                               "приложения на другой компьютер."),
                            backupBox);
    hint->setObjectName(QStringLiteral("Faint"));
    hint->setWordWrap(true);
    backupLayout->addWidget(hint);
    auto* backupButton = new QPushButton(tr("Сохранить копию данных…"), backupBox);
    connect(backupButton, &QPushButton::clicked, this, &SettingsDialog::makeBackup);
    backupLayout->addWidget(backupButton, 0, Qt::AlignLeft);
    layout->addWidget(backupBox);
    layout->addStretch(1);
}

void SettingsDialog::readSettings()
{
    Settings& settings = Settings::instance();
    m_timeFormat->setCurrentIndex(m_timeFormat->findData(settings.use24h()));
    m_firstDay->setCurrentIndex(m_firstDay->findData(settings.weekStartsMonday()));
    m_secondsTitle->setChecked(settings.showSecondsInTitle());
    m_secondsClocks->setChecked(settings.showSecondsInClocks());
    m_autostart->setChecked(platform::autostartEnabled());
    m_startMinimized->setChecked(settings.startMinimized());
    m_closeToTray->setChecked(settings.closeToTray());
    m_minimizeToTray->setChecked(settings.minimizeToTray());

    m_showNav->setChecked(settings.showNavPane());
    m_showStatus->setChecked(settings.showStatusBar());
    m_showGrid->setChecked(settings.showGrid());
    m_alternateRows->setChecked(settings.alternateRows());
    m_density->setCurrentIndex(std::clamp(settings.rowDensity(), 0, 2));
    m_uiFontFile = settings.raw().value(QStringLiteral("appearance/uiFontFile")).toString();
    m_digitFontFile = settings.raw().value(QStringLiteral("appearance/digitFontFile")).toString();
    if (m_uiFontCombo != nullptr) {
        const QSignalBlocker blocker(m_uiFontCombo);
        m_uiFontCombo->setCurrentFont(QFont(fonts::uiFamily()));
    }
    if (m_digitFontCombo != nullptr) {
        const QSignalBlocker blocker(m_digitFontCombo);
        m_digitFontCombo->setCurrentFont(QFont(fonts::digitFamily()));
    }

    const int alarmSoundIndex = m_alarmSound->findData(settings.defaultAlarmSound());
    m_alarmSound->setCurrentIndex(alarmSoundIndex >= 0 ? alarmSoundIndex : 0);
    m_alarmVolume->setValue(settings.defaultAlarmVolume());
    m_alarmLoop->setChecked(settings.defaultLoopSound());
    m_alarmMaxSound->setValue(settings.defaultMaxSoundMinutes());
    m_alarmPopup->setChecked(settings.defaultAlarmPopup());
    m_snoozeMinutes->setValue(settings.defaultSnoozeMinutes());
    m_maxSnoozes->setValue(settings.defaultMaxSnoozes());
    m_preAlert->setValue(settings.defaultPreAlertMinutes());

    m_timerMinutes->setValue(settings.defaultTimerMinutes());
    const int timerSoundIndex = m_timerSound->findData(settings.defaultTimerSound());
    m_timerSound->setCurrentIndex(timerSoundIndex >= 0 ? timerSoundIndex : 0);
    m_timerVolume->setValue(settings.defaultTimerVolume());
    m_timerLoop->setChecked(settings.defaultTimerLoop());
    m_timerPopup->setChecked(settings.defaultTimerPopup());

    m_preAlerts->setChecked(settings.preAlertsEnabled());
    m_muted->setChecked(settings.raw()
                            .value(QStringLiteral("notifications/muted"), false)
                            .toBool());

    m_calendarAlarms->setChecked(settings.calendarShowAlarms());
    m_calendarTasks->setChecked(settings.calendarShowTasks());
    m_weekNumbers->setChecked(settings.showWeekNumbers());

    m_dataPath->setText(platform::dataDirectory() + QStringLiteral("/data.json"));
    m_settingsPath->setText(settings.filePath());
    m_themeId = settings.themeId();
    const bool known = std::any_of(ThemeManager::instance().themes().cbegin(),
                                   ThemeManager::instance().themes().cend(),
                                   [this](const ThemeColors& colors) {
                                       return colors.id == m_themeId;
                                   });
    if (!known) {
        m_themeId = QStringLiteral("office2007-blue");
    }
    updateThemeChecks();
}

void SettingsDialog::updateThemeChecks()
{
    for (auto* button : m_themeButtons) {
        button->setChecked(button->property("themeId").toString() == m_themeId);
    }
}

void SettingsDialog::resetToDefaults()
{
    m_themeId = QStringLiteral("office2007-blue");
    m_timeFormat->setCurrentIndex(m_timeFormat->findData(true));
    m_firstDay->setCurrentIndex(m_firstDay->findData(true));
    m_secondsTitle->setChecked(true);
    m_secondsClocks->setChecked(true);
    m_autostart->setChecked(false);
    m_startMinimized->setChecked(false);
    m_closeToTray->setChecked(true);
    m_minimizeToTray->setChecked(false);
    m_showNav->setChecked(true);
    m_showStatus->setChecked(true);
    m_showGrid->setChecked(true);
    m_alternateRows->setChecked(true);
    m_density->setCurrentIndex(1);
    m_uiFontFile.clear();
    m_digitFontFile.clear();
    if (m_uiFontCombo != nullptr) {
        m_uiFontCombo->setCurrentFont(QFont(QStringLiteral("Segoe UI")));
    }
    if (m_digitFontCombo != nullptr) {
        m_digitFontCombo->setCurrentFont(QFont(QStringLiteral("Cascadia Mono")));
    }
    m_alarmSound->setCurrentIndex(m_alarmSound->findData(QStringLiteral("builtin:chime")));
    m_alarmVolume->setValue(80);
    m_alarmLoop->setChecked(true);
    m_alarmMaxSound->setValue(5);
    m_alarmPopup->setChecked(true);
    m_snoozeMinutes->setValue(5);
    m_maxSnoozes->setValue(3);
    m_preAlert->setValue(0);
    m_timerMinutes->setValue(10);
    m_timerSound->setCurrentIndex(m_timerSound->findData(QStringLiteral("builtin:digital")));
    m_timerVolume->setValue(80);
    m_timerLoop->setChecked(false);
    m_timerPopup->setChecked(true);
    m_preAlerts->setChecked(true);
    m_muted->setChecked(false);
    m_calendarAlarms->setChecked(true);
    m_calendarTasks->setChecked(true);
    m_weekNumbers->setChecked(false);
    updateThemeChecks();
}

void SettingsDialog::applySettings()
{
    QSettings& settings = Settings::instance().raw();
    settings.setValue(QStringLiteral("general/use24h"), m_timeFormat->currentData().toBool());
    settings.setValue(QStringLiteral("general/weekStartsMonday"),
                      m_firstDay->currentData().toBool());
    settings.setValue(QStringLiteral("general/showSecondsInTitle"),
                      m_secondsTitle->isChecked());
    settings.setValue(QStringLiteral("appearance/showSecondsInClocks"),
                      m_secondsClocks->isChecked());
    settings.setValue(QStringLiteral("general/startMinimized"), m_startMinimized->isChecked());
    settings.setValue(QStringLiteral("general/closeToTray"), m_closeToTray->isChecked());
    settings.setValue(QStringLiteral("general/minimizeToTray"), m_minimizeToTray->isChecked());
    settings.setValue(QStringLiteral("appearance/showNavPane"), m_showNav->isChecked());
    settings.setValue(QStringLiteral("appearance/showStatusBar"), m_showStatus->isChecked());
    settings.setValue(QStringLiteral("appearance/rowDensity"), m_density->currentIndex());
    settings.setValue(QStringLiteral("appearance/uiFontFamily"),
                      m_uiFontCombo->currentFont().family());
    settings.setValue(QStringLiteral("appearance/uiFontFile"), m_uiFontFile);
    settings.setValue(QStringLiteral("appearance/digitFontFamily"),
                      m_digitFontCombo->currentFont().family());
    settings.setValue(QStringLiteral("appearance/digitFontFile"), m_digitFontFile);
    settings.setValue(QStringLiteral("appearance/showGrid"), m_showGrid->isChecked());
    settings.setValue(QStringLiteral("appearance/alternateRows"), m_alternateRows->isChecked());
    settings.setValue(QStringLiteral("notifications/preAlerts"), m_preAlerts->isChecked());
    settings.setValue(QStringLiteral("notifications/muted"), m_muted->isChecked());
    settings.setValue(QStringLiteral("calendar/showAlarms"), m_calendarAlarms->isChecked());
    settings.setValue(QStringLiteral("calendar/showTasks"), m_calendarTasks->isChecked());
    settings.setValue(QStringLiteral("calendar/weekNumbers"), m_weekNumbers->isChecked());
    settings.setValue(QStringLiteral("alarms/sound"), m_alarmSound->currentData().toString());
    settings.setValue(QStringLiteral("alarms/volume"), m_alarmVolume->value());
    settings.setValue(QStringLiteral("alarms/loopSound"), m_alarmLoop->isChecked());
    settings.setValue(QStringLiteral("alarms/maxSoundMinutes"), m_alarmMaxSound->value());
    settings.setValue(QStringLiteral("alarms/popup"), m_alarmPopup->isChecked());
    settings.setValue(QStringLiteral("alarms/snoozeMinutes"), m_snoozeMinutes->value());
    settings.setValue(QStringLiteral("alarms/maxSnoozes"), m_maxSnoozes->value());
    settings.setValue(QStringLiteral("alarms/preAlertMinutes"), m_preAlert->value());
    settings.setValue(QStringLiteral("timers/minutes"), m_timerMinutes->value());
    settings.setValue(QStringLiteral("timers/sound"), m_timerSound->currentData().toString());
    settings.setValue(QStringLiteral("timers/volume"), m_timerVolume->value());
    settings.setValue(QStringLiteral("timers/loopSound"), m_timerLoop->isChecked());
    settings.setValue(QStringLiteral("timers/popup"), m_timerPopup->isChecked());
    settings.sync();
    platform::setAutostart(m_autostart->isChecked());
    fonts::apply();
    ThemeManager::instance().setTheme(m_themeId);
    Settings::instance().notifyChanged();
}

void SettingsDialog::makeBackup()
{
    const QString source = platform::dataDirectory() + QStringLiteral("/data.json");
    if (!QFileInfo::exists(source)) {
        QMessageBox::warning(this, tr("Резервная копия"), tr("Файл данных ещё не создан."));
        return;
    }
    const QString target = QFileDialog::getSaveFileName(
        this, tr("Сохранить копию данных"),
        QDir(platform::dataDirectory())
            .filePath(QStringLiteral("PerfectAlarmClock-backup-%1.json")
                          .arg(QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd")))),
        tr("JSON (*.json);;Все файлы (*.*)"));
    if (target.isEmpty()) {
        return;
    }
    if (QFile::exists(target)) {
        QFile::remove(target);
    }
    if (!QFile::copy(source, target)) {
        QMessageBox::warning(this, tr("Резервная копия"),
                             tr("Не удалось сохранить резервную копию."));
        return;
    }
    QMessageBox::information(this, tr("Резервная копия"),
                             tr("Копия данных сохранена:\n%1").arg(target));
}

} // namespace pac::ui
