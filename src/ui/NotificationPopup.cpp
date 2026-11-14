#include "NotificationPopup.h"

#include "UiKit.h"
#include "core/Theme.h"

#include <QComboBox>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QFontMetrics>
#include <QLinearGradient>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

namespace pac::ui {
namespace {

constexpr int kPopupWidth = 500;
constexpr int kPopupHeight = 182;

class PopupCard final : public QFrame {
public:
    explicit PopupCard(QWidget* parent = nullptr)
        : QFrame(parent)
    {
        setAttribute(Qt::WA_StyledBackground, false);
    }

    void setAccent(const QColor& accent)
    {
        m_accent = accent;
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event)
        const ThemeColors& colors = ThemeManager::instance().current();
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const QRectF rect = QRectF(this->rect()).adjusted(0.5, 0.5, -0.5, -0.5);
        QPainterPath path;
        path.addRoundedRect(rect, 9.0, 9.0);
        QLinearGradient gradient(rect.topLeft(), rect.bottomLeft());
        gradient.setColorAt(0.0, colors.card);
        gradient.setColorAt(1.0, colors.panel);
        painter.fillPath(path, gradient);
        painter.setClipPath(path);
        painter.fillRect(QRectF(rect.left(), rect.top(), rect.width(), 3.5), m_accent);
        painter.setClipping(false);
        painter.setPen(QPen(tinted(m_accent, 0.55), 1.0));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(path);
    }

private:
    QColor m_accent;
};

} // namespace

NotificationPopup::NotificationPopup(QWidget* parent)
    : QWidget(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
{
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_DeleteOnClose, true);
    setFocusPolicy(Qt::StrongFocus);
    setFixedSize(kPopupWidth, kPopupHeight);
    setWindowTitle(tr("Уведомление"));

    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(14, 10, 14, 18);
    outer->setSpacing(0);

    auto* card = new PopupCard(this);
    card->setObjectName(QStringLiteral("PopupCard"));
    auto* shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(26.0);
    shadow->setOffset(0.0, 5.0);
    shadow->setColor(QColor(0, 0, 0, 90));
    card->setGraphicsEffect(shadow);
    outer->addWidget(card);

    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(18, 14, 14, 12);
    layout->setSpacing(8);

    auto* header = new QHBoxLayout;
    header->setSpacing(12);
    m_icon = new QLabel(card);
    m_icon->setFixedSize(42, 42);
    m_icon->setAlignment(Qt::AlignCenter);
    header->addWidget(m_icon);

    auto* textColumn = new QVBoxLayout;
    textColumn->setSpacing(1);
    m_overline = new QLabel(card);
    m_overline->setObjectName(QStringLiteral("SectionLabel"));
    textColumn->addWidget(m_overline);
    m_title = new QLabel(card);
    QFont titleFont = m_title->font();
    titleFont.setPointSizeF(12.0);
    titleFont.setWeight(QFont::DemiBold);
    m_title->setFont(titleFont);
    m_title->setMaximumWidth(330);
    textColumn->addWidget(m_title);
    m_details = new QLabel(card);
    m_details->setObjectName(QStringLiteral("Muted"));
    m_details->setMaximumWidth(330);
    textColumn->addWidget(m_details);
    header->addLayout(textColumn, 1);

    m_close = new QPushButton(card);
    m_close->setFixedSize(22, 22);
    m_close->setFlat(true);
    m_close->setIcon(icons::qicon(icons::Icon::Close,
                                  ThemeManager::instance().current().dim));
    m_close->setIconSize(QSize(12, 12));
    m_close->setCursor(Qt::PointingHandCursor);
    m_close->setToolTip(tr("Закрыть"));
    connect(m_close, &QPushButton::clicked, this, &NotificationPopup::dismissed);
    header->addWidget(m_close, 0, Qt::AlignTop);
    layout->addLayout(header);

    auto* footer = new QHBoxLayout;
    footer->setSpacing(8);
    footer->setContentsMargins(54, 0, 0, 0);
    m_hint = new QLabel(card);
    m_hint->setObjectName(QStringLiteral("Faint"));
    m_hint->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    footer->addWidget(m_hint, 1);

    m_snoozeCombo = new QComboBox(card);
    m_snoozeCombo->setFixedWidth(92);
    m_snoozeCombo->setFixedHeight(28);
    m_snoozeCombo->setVisible(false);
    footer->addWidget(m_snoozeCombo);

    m_secondary = new ThemedButton(tr("Остановить"), icons::Icon::Stop, card);
    m_secondary->setMinimumHeight(28);
    m_secondary->setMinimumWidth(104);
    connect(m_secondary, &QPushButton::clicked, this, &NotificationPopup::dismissed);
    footer->addWidget(m_secondary);

    m_primary = new ThemedButton(tr("Отложить"), icons::Icon::Snooze, card, true);
    m_primary->setMinimumHeight(28);
    m_primary->setMinimumWidth(108);
    m_primary->setAutoDefault(false);
    m_primary->setDefault(false);
    connect(m_primary, &QPushButton::clicked, this, &NotificationPopup::primaryClicked);
    footer->addWidget(m_primary);
    layout->addLayout(footer);

    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this] {
        if (m_close != nullptr) {
            m_close->setIcon(icons::qicon(icons::Icon::Close,
                                          ThemeManager::instance().current().dim));
        }
        update();
    });
}

void NotificationPopup::setContent(const QString& overline, const QString& title,
                                   const QString& details, const QString& emoji,
                                   const QColor& accent, const QString& hint,
                                   const QString& primaryText, icons::Icon primaryIcon,
                                   const QString& secondaryText)
{
    m_accent = accent.isValid() ? accent : ThemeManager::instance().current().accent;
    m_overline->setText(overline);
    const QFontMetrics titleMetrics(m_title->font());
    m_title->setText(titleMetrics.elidedText(title, Qt::ElideRight, 330));
    m_title->setToolTip(title);
    const QFontMetrics detailMetrics(m_details->font());
    m_details->setText(detailMetrics.elidedText(details, Qt::ElideRight, 330));
    m_details->setToolTip(details);

    if (!emoji.isEmpty()) {
        m_icon->setPixmap(emojiPixmap(emoji, 26));
    } else {
        m_icon->setPixmap(icons::pixmap(icons::Icon::Bell, 24, m_accent));
    }
    m_icon->setStyleSheet(QStringLiteral("background: rgba(%1,%2,%3,38); border: 1px solid rgba(%1,%2,%3,100); border-radius: 9px;")
                              .arg(m_accent.red())
                              .arg(m_accent.green())
                              .arg(m_accent.blue()));
    m_hint->setText(hint);
    m_primary->setText(primaryText);
    m_primary->setGlyph(primaryIcon);
    m_primary->setVisible(!primaryText.isEmpty());
    m_secondary->setText(secondaryText.isEmpty() ? tr("Закрыть") : secondaryText);
    if (auto* card = findChild<QFrame*>(QStringLiteral("PopupCard"))) {
        static_cast<PopupCard*>(card)->setAccent(m_accent);
    }
    update();
}

void NotificationPopup::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    setWindowOpacity(1.0);
    if (!m_activateOnShow) {
        return;
    }
    activateWindow();
    setFocus(Qt::OtherFocusReason);
}

void NotificationPopup::setActivateOnShow(bool activate)
{
    m_activateOnShow = activate;
    setAttribute(Qt::WA_ShowWithoutActivating, !activate);
}

void NotificationPopup::setSnoozeOptions(const QList<int>& minutes, int selected)
{
    if (m_snoozeCombo == nullptr) {
        return;
    }
    m_snoozeCombo->clear();
    for (const int value : minutes) {
        if (value > 0) {
            m_snoozeCombo->addItem(tr("%1 мин").arg(value), value);
        }
    }
    m_snoozeCombo->setVisible(m_snoozeCombo->count() > 1);
    if (selected > 0) {
        const int index = m_snoozeCombo->findData(selected);
        if (index >= 0) {
            m_snoozeCombo->setCurrentIndex(index);
        }
    }
}

int NotificationPopup::selectedSnoozeMinutes() const
{
    if (m_snoozeCombo == nullptr || !m_snoozeCombo->isVisible()) {
        return 0;
    }
    return m_snoozeCombo->currentData().toInt();
}

void NotificationPopup::dismissNow()
{
    emit dismissed();
}

void NotificationPopup::triggerPrimary()
{
    if (m_primary != nullptr && m_primary->isVisible()) {
        emit primaryClicked();
    }
}

bool NotificationPopup::isUrgent() const noexcept
{
    return m_activateOnShow;
}

void NotificationPopup::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape || event->key() == Qt::Key_Return
        || event->key() == Qt::Key_Enter || event->key() == Qt::Key_Space) {
        emit dismissed();
        return;
    }
    QWidget::keyPressEvent(event);
}

void NotificationPopup::mousePressEvent(QMouseEvent* event)
{
    Q_UNUSED(event)
}

} // namespace pac::ui
