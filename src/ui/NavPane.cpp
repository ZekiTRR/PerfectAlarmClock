#include "NavPane.h"

#include "core/Settings.h"
#include "core/Theme.h"
#include "ui/UiKit.h"

#include <QEnterEvent>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainter>
#include <QVBoxLayout>

#include <algorithm>

namespace pac::ui {

NavItem::NavItem(QString id, QString title, icons::Icon icon, QColor accent, QWidget* parent)
    : QAbstractButton(parent)
    , m_id(std::move(id))
    , m_icon(icon)
    , m_accent(std::move(accent))
{
    setText(std::move(title));
    setCursor(Qt::PointingHandCursor);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFocusPolicy(Qt::StrongFocus);
    connect(&Settings::instance(), &Settings::changed, this, [this] { applyDensity(); });
    applyDensity();
}

void NavItem::applyDensity()
{
    const int density = Settings::instance().rowDensity();
    setFixedHeight(density == 0 ? 34 : (density == 2 ? 54 : 44));
    updateGeometry();
    update();
}

QString NavItem::id() const
{
    return m_id;
}

void NavItem::setActive(bool active)
{
    if (m_active == active) {
        return;
    }
    m_active = active;
    update();
}

void NavItem::setBadge(const QString& text)
{
    if (m_badge == text) {
        return;
    }
    m_badge = text;
    update();
}

void NavItem::setCompact(bool compact)
{
    if (m_compact == compact) {
        return;
    }
    m_compact = compact;
    setToolTip(compact ? text() : QString());
    update();
}

void NavItem::setFluentMode(bool fluent)
{
    if (m_fluent == fluent) {
        return;
    }
    m_fluent = fluent;
    update();
}

void NavItem::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    const ThemeColors& colors = ThemeManager::instance().current();
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    const QRectF itemRect = QRectF(rect()).adjusted(2.0, 2.0, -3.0, -2.0);

    if (m_fluent) {
        if (m_active) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(colors.navSelTop);
            painter.drawRoundedRect(itemRect, 8.0, 8.0);
            painter.setBrush(colors.accent);
            painter.drawRoundedRect(QRectF(itemRect.left() + 10.0, itemRect.center().y() - 8.0,
                                           3.0, 16.0),
                                    1.5, 1.5);
        } else if (m_hovered) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(colors.navHover);
            painter.drawRoundedRect(itemRect, 8.0, 8.0);
        }

        const qreal iconSize = 20.0;
        const QRectF iconRect(itemRect.left() + (m_compact ? (itemRect.width() - iconSize) / 2.0 : 18.0),
                              itemRect.center().y() - iconSize / 2.0, iconSize, iconSize);
        icons::paint(painter, iconRect, m_icon,
                     m_active ? colors.navSelText : colors.navText, 1.7);

        if (!m_compact) {
            QFont labelFont = font();
            labelFont.setPointSizeF(10.0);
            labelFont.setWeight(m_active ? QFont::DemiBold : QFont::Medium);
            painter.setFont(labelFont);
            painter.setPen(m_active ? colors.navSelText : colors.navText);
            const QRectF textRect(iconRect.right() + 14.0, itemRect.top(),
                                  itemRect.right() - iconRect.right() - 22.0, itemRect.height());
            painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter,
                             painter.fontMetrics().elidedText(
                                 text(), Qt::ElideRight, static_cast<int>(textRect.width())));
        }
        return;
    }

    if (m_active) {
        QLinearGradient gradient(itemRect.topLeft(), itemRect.bottomLeft());
        gradient.setColorAt(0.0, colors.navSelTop);
        gradient.setColorAt(1.0, colors.navSelBottom);
        painter.setPen(QPen(tinted(colors.navBorder, 0.85), 1.0));
        painter.setBrush(gradient);
        painter.drawRoundedRect(itemRect, 4.0, 4.0);
        painter.setPen(Qt::NoPen);
        painter.setBrush(m_accent);
        painter.drawRoundedRect(QRectF(itemRect.left(), itemRect.top() + 4.0, 3.0,
                                       itemRect.height() - 8.0), 1.5, 1.5);
    } else if (m_hovered) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(tinted(colors.navHover, 0.85));
        painter.drawRoundedRect(itemRect, 4.0, 4.0);
    }

    const int density = Settings::instance().rowDensity();
    const qreal tileSize = density == 0 ? 22.0 : (density == 2 ? 30.0 : 26.0);
    const qreal tileX = m_compact ? itemRect.center().x() - tileSize / 2.0
                                  : itemRect.left() + 10.0;
    const QRectF iconTile(tileX, itemRect.center().y() - tileSize / 2.0, tileSize, tileSize);
    painter.setPen(Qt::NoPen);
    painter.setBrush(tinted(m_accent, m_active ? 0.20 : 0.13));
    painter.drawRoundedRect(iconTile, 5.0, 5.0);
    icons::paint(painter, iconTile.adjusted(4.0, 4.0, -4.0, -4.0), m_icon, m_accent, 1.85);

    if (m_compact) {
        if (!m_badge.isEmpty()) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(m_accent);
            painter.drawEllipse(QPointF(iconTile.right() - 3.0, iconTile.top() + 3.0), 3.4, 3.4);
        }
        return;
    }

    QFont labelFont = font();
    labelFont.setPointSizeF(density == 0 ? 8.5 : (density == 2 ? 9.5 : 9.0));
    labelFont.setWeight(m_active ? QFont::DemiBold : QFont::Normal);
    painter.setFont(labelFont);
    painter.setPen(m_active ? colors.navSelText : colors.navText);
    const QRectF textRect(iconTile.right() + 9.0, itemRect.top(),
                          itemRect.right() - iconTile.right() - (m_badge.isEmpty() ? 11.0 : 38.0),
                          itemRect.height());
    const QFontMetrics metrics(labelFont);
    painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter,
                     metrics.elidedText(text(), Qt::ElideRight, static_cast<int>(textRect.width())));

    if (!m_badge.isEmpty()) {
        QFont badgeFont = font();
        badgeFont.setPointSizeF(7.5);
        badgeFont.setWeight(QFont::DemiBold);
        painter.setFont(badgeFont);
        const QFontMetrics badgeMetrics(badgeFont);
        const int width = std::max(18, badgeMetrics.horizontalAdvance(m_badge) + 12);
        const QRectF badgeRect(itemRect.right() - width - 7.0, itemRect.center().y() - 8.0,
                               width, 16.0);
        painter.setPen(Qt::NoPen);
        painter.setBrush(tinted(m_accent, 0.16));
        painter.drawRoundedRect(badgeRect, 8.0, 8.0);
        painter.setPen(m_accent);
        painter.drawText(badgeRect, Qt::AlignCenter, m_badge);
    }
}

void NavItem::enterEvent(QEnterEvent* event)
{
    m_hovered = true;
    update();
    QAbstractButton::enterEvent(event);
}

void NavItem::leaveEvent(QEvent* event)
{
    m_hovered = false;
    update();
    QAbstractButton::leaveEvent(event);
}

NextAlarmCard::NextAlarmCard(QWidget* parent)
    : QWidget(parent)
{
    setCursor(Qt::PointingHandCursor);
    setToolTip(tr("Перейти к будильникам"));
    connect(&Settings::instance(), &Settings::changed, this, [this] { applyDensity(); });
    applyDensity();
}

void NextAlarmCard::applyDensity()
{
    const int density = Settings::instance().rowDensity();
    setFixedHeight(density == 0 ? 68 : (density == 2 ? 86 : 76));
    updateGeometry();
}

void NextAlarmCard::setAlarm(const QString& timeLine, const QString& label, const QString& icon,
                             bool active)
{
    m_timeLine = timeLine;
    m_label = label;
    m_icon = icon;
    m_active = active;
    update();
}

void NextAlarmCard::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    const ThemeColors& colors = ThemeManager::instance().current();
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    const QRectF card = QRectF(rect()).adjusted(7.0, 2.0, -7.0, -4.0);

    painter.setPen(QPen(colors.navBorder, 1.0));
    painter.setBrush(m_hovered ? tinted(colors.card, 0.98) : tinted(colors.card, 0.82));
    painter.drawRoundedRect(card, 5.0, 5.0);
    painter.setPen(Qt::NoPen);
    painter.setBrush(m_active ? QColor(QStringLiteral("#e2842b")) : colors.navBorder);
    painter.drawRoundedRect(QRectF(card.left(), card.top() + 7.0, 3.0, card.height() - 14.0), 1.5, 1.5);

    const QRectF iconRect(card.left() + 11.0, card.top() + 12.0, 20.0, 20.0);
    if (!m_icon.isEmpty()) {
        painter.drawPixmap(iconRect.toRect(), emojiPixmap(m_icon, 18));
    } else {
        icons::paint(painter, iconRect, icons::Icon::Bell, colors.dim, 1.8);
    }

    QFont overline = font();
    overline.setPointSizeF(6.8);
    overline.setWeight(QFont::DemiBold);
    painter.setFont(overline);
    painter.setPen(colors.navDim);
    painter.drawText(QRectF(card.left() + 39.0, card.top() + 7.0, card.width() - 47.0, 12.0),
                     Qt::AlignLeft | Qt::AlignVCenter, tr("СЛЕДУЮЩИЙ"));

    QFont timeFont = font();
    timeFont.setPointSizeF(8.8);
    timeFont.setWeight(QFont::DemiBold);
    painter.setFont(timeFont);
    painter.setPen(m_active ? colors.navSelText : colors.navText);
    const QString timeText = m_timeLine.isEmpty() ? tr("Нет будильников") : m_timeLine;
    painter.drawText(QRectF(card.left() + 39.0, card.top() + 18.0, card.width() - 47.0, 16.0),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     painter.fontMetrics().elidedText(timeText, Qt::ElideRight,
                                                      static_cast<int>(card.width() - 47.0)));

    QFont detail = font();
    detail.setPointSizeF(7.5);
    painter.setFont(detail);
    painter.setPen(colors.navDim);
    const QString detailText = m_label.isEmpty()
                                   ? tr("Нажмите, чтобы создать")
                                   : m_label;
    painter.drawText(QRectF(card.left() + 39.0, card.top() + 34.0, card.width() - 47.0, 14.0),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     painter.fontMetrics().elidedText(detailText, Qt::ElideRight,
                                                      static_cast<int>(card.width() - 47.0)));
}

void NextAlarmCard::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && rect().contains(event->position().toPoint())) {
        emit clicked();
    }
    QWidget::mouseReleaseEvent(event);
}

void NextAlarmCard::enterEvent(QEnterEvent* event)
{
    m_hovered = true;
    update();
    QWidget::enterEvent(event);
}

void NextAlarmCard::leaveEvent(QEvent* event)
{
    m_hovered = false;
    update();
    QWidget::leaveEvent(event);
}

NavPane::NavPane(QWidget* parent)
    : QWidget(parent)
{
    setFixedWidth(202);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

    m_rootLayout = new QVBoxLayout(this);
    m_rootLayout->setContentsMargins(7, 34, 7, 7);
    m_rootLayout->setSpacing(1);

    m_itemsLayout = new QVBoxLayout;
    m_itemsLayout->setContentsMargins(0, 0, 0, 0);
    m_rootLayout->addLayout(m_itemsLayout);
    m_rootLayout->addStretch(1);

    m_nextCard = new NextAlarmCard(this);
    m_rootLayout->addWidget(m_nextCard);
    connect(m_nextCard, &NextAlarmCard::clicked, this, &NavPane::nextAlarmClicked);

    connect(&Settings::instance(), &Settings::changed, this, &NavPane::applyLayoutMetrics);
    applyLayoutMetrics();
}

void NavPane::applyLayoutMetrics()
{
    if (m_rootLayout == nullptr) {
        return;
    }
    const int density = Settings::instance().rowDensity();
    if (m_itemsLayout != nullptr) {
        m_itemsLayout->setSpacing(m_fluent ? 3 : (density == 0 ? 0 : (density == 2 ? 3 : 1)));
    }
    const int horizontal = m_compact ? 6 : (m_fluent ? 10 : 7);
    const int top = m_fluent ? 12 : (density == 0 ? 28 : (density == 2 ? 40 : 34));
    m_rootLayout->setContentsMargins(horizontal, top, horizontal, m_fluent ? 12 : 7);
    update();
}

void NavPane::setCompact(bool compact)
{
    if (m_compact == compact) {
        return;
    }
    m_compact = compact;
    setFixedWidth(compact ? 64 : (m_fluent ? 226 : 202));
    for (auto* item : m_items) {
        item->setCompact(compact);
    }
    if (m_nextCard != nullptr) {
        m_nextCard->setVisible(!compact && !m_fluent);
    }
    applyLayoutMetrics();
    updateGeometry();
    update();
}

void NavPane::setFluentMode(bool fluent)
{
    if (m_fluent == fluent) {
        return;
    }
    m_fluent = fluent;
    setFixedWidth(m_compact ? 64 : (fluent ? 226 : 202));
    for (auto* item : m_items) {
        item->setFluentMode(fluent);
        item->setVisible(fluent || item->id() != QStringLiteral("settings"));
    }
    if (m_nextCard != nullptr) {
        m_nextCard->setVisible(!fluent && !m_compact);
    }
    applyLayoutMetrics();
    updateGeometry();
    update();
}

void NavPane::addItem(const QString& id, const QString& title, icons::Icon icon,
                      const QColor& accent)
{
    auto* item = new NavItem(id, title, icon, accent, this);
    item->setCompact(m_compact);
    item->setFluentMode(m_fluent);
    item->setVisible(m_fluent || id != QStringLiteral("settings"));
    m_items.append(item);
    m_itemsLayout->addWidget(item);
    connect(item, &QAbstractButton::clicked, this, [this, id] {
        setCurrent(id);
        emit currentChanged(id);
    });
    if (m_currentId.isEmpty()) {
        setCurrent(id);
    }
}

void NavPane::setCurrent(const QString& id)
{
    m_currentId = id;
    for (auto* item : m_items) {
        item->setActive(item->id() == id);
    }
}

QString NavPane::currentId() const
{
    return m_currentId;
}

void NavPane::setBadge(const QString& id, const QString& badge)
{
    for (auto* item : m_items) {
        if (item->id() == id) {
            item->setBadge(badge);
            return;
        }
    }
}

void NavPane::setNextAlarm(const QString& timeLine, const QString& label, const QString& icon,
                           bool active)
{
    m_nextCard->setAlarm(timeLine, label, icon, active);
}

void NavPane::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    const ThemeColors& colors = ThemeManager::instance().current();
    QPainter painter(this);

    if (m_fluent) {
        painter.fillRect(rect(), colors.window);
        return;
    }

    QLinearGradient gradient(0.0, 0.0, 0.0, height());
    gradient.setColorAt(0.0, colors.navTop);
    gradient.setColorAt(1.0, colors.navBottom);
    painter.fillRect(rect(), gradient);
    painter.setPen(QPen(colors.navBorder, 1.0));
    painter.drawLine(width() - 1, 0, width() - 1, height());

    if (m_compact) {
        icons::paint(painter, QRectF(width() / 2.0 - 7.0, 11.0, 14.0, 14.0), icons::Icon::Grid,
                     colors.navDim, 1.6);
    } else {
        QFont header = font();
        header.setPointSizeF(7.2);
        header.setWeight(QFont::DemiBold);
        painter.setFont(header);
        painter.setPen(colors.navDim);
        painter.drawText(QRectF(16.0, 8.0, width() - 32.0, 18.0),
                         Qt::AlignLeft | Qt::AlignVCenter, tr("НАВИГАЦИЯ"));
        icons::paint(painter, QRectF(width() - 30.0, 12.0, 12.0, 12.0), icons::Icon::Grid,
                     colors.navDim, 1.6);
    }
    painter.setPen(QPen(tinted(colors.navBorder, 0.55), 1.0));
    painter.drawLine(12, 29, width() - 12, 29);
}

} // namespace pac::ui
