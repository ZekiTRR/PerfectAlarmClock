#include "UiKit.h"

#include "core/Theme.h"

#include <QApplication>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPixmapCache>

#include <algorithm>

namespace pac::ui {

PageWidget::PageWidget(QWidget* parent)
    : QWidget(parent)
{
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this] {
        themeUpdated();
        update();
    });
}

void PageWidget::activate()
{
    if (m_active) {
        return;
    }
    m_active = true;
    onActivated();
}

void PageWidget::deactivate()
{
    if (!m_active) {
        return;
    }
    m_active = false;
    onDeactivated();
}

void PageWidget::refreshClock(const QDateTime& now)
{
    Q_UNUSED(now)
}

bool PageWidget::isActive() const noexcept
{
    return m_active;
}

void PageWidget::onActivated()
{
}

void PageWidget::applyThemeMode()
{
}

void PageWidget::onDeactivated()
{
}

void PageWidget::themeUpdated()
{
}

ThemedButton::ThemedButton(const QString& text, icons::Icon icon, QWidget* parent, bool accent)
    : QPushButton(text, parent)
    , m_glyph(icon)
    , m_accent(accent)
{
    if (m_accent) {
        setProperty("accent", true);
    }
    setCursor(Qt::PointingHandCursor);
    refreshIcon();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this] {
        refreshIcon();
    });
}

void ThemedButton::setGlyph(icons::Icon icon)
{
    m_glyph = icon;
    refreshIcon();
}

void ThemedButton::setGlyphColor(const QColor& color)
{
    m_customColor = color;
    refreshIcon();
}

void ThemedButton::setIconSizePx(int size)
{
    m_iconSize = size;
    refreshIcon();
}

void ThemedButton::refreshIcon()
{
    const ThemeColors& colors = ThemeManager::instance().current();
    const QColor color = m_customColor.isValid()
                             ? m_customColor
                             : (m_accent ? QColor(Qt::white) : colors.accent);
    setIcon(icons::qicon(m_glyph, color));
    setIconSize(QSize(m_iconSize, m_iconSize));
}

ThemedToolButton::ThemedToolButton(icons::Icon icon, const QString& tooltip, QWidget* parent)
    : QToolButton(parent)
    , m_glyph(icon)
{
    setToolTip(tooltip);
    setAutoRaise(true);
    setCursor(Qt::PointingHandCursor);
    refreshIcon();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this] {
        refreshIcon();
    });
}

ThemedToolButton::ThemedToolButton(icons::Icon icon, const QString& text, const QString& tooltip,
                                   QWidget* parent)
    : QToolButton(parent)
    , m_glyph(icon)
{
    setText(text);
    setToolTip(tooltip);
    setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    setAutoRaise(true);
    setCursor(Qt::PointingHandCursor);
    refreshIcon();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this] {
        refreshIcon();
    });
}

void ThemedToolButton::setGlyph(icons::Icon icon)
{
    m_glyph = icon;
    refreshIcon();
}

void ThemedToolButton::setGlyphColor(const QColor& color)
{
    m_customColor = color;
    refreshIcon();
}

void ThemedToolButton::setIconSizePx(int size)
{
    m_iconSize = size;
    refreshIcon();
}

void ThemedToolButton::refreshIcon()
{
    const ThemeColors& colors = ThemeManager::instance().current();
    const QColor color = m_customColor.isValid() ? m_customColor : colors.accent;
    setIcon(icons::qicon(m_glyph, color));
    setIconSize(QSize(m_iconSize, m_iconSize));
}

SwitchButton::SwitchButton(QWidget* parent)
    : QAbstractButton(parent)
{
    setCheckable(true);
    setFixedSize(44, 22);
    setCursor(Qt::PointingHandCursor);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

QSize SwitchButton::sizeHint() const
{
    return QSize(44, 22);
}

void SwitchButton::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    const ThemeColors& colors = ThemeManager::instance().current();
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    const QRectF track = QRectF(rect()).adjusted(1.0, 1.0, -1.0, -1.0);
    const qreal radius = track.height() / 2.0;

    QColor borderColor = isChecked() ? colors.accent : colors.inputBorder;
    if (hasFocus()) {
        borderColor = colors.accent;
    }
    painter.setPen(QPen(borderColor, isChecked() ? 1.0 : 1.4));
    painter.setBrush(isChecked() ? colors.accent : colors.inputBg);
    painter.drawRoundedRect(track, radius, radius);

    const qreal knobSize = 11.0;
    const qreal knobX = isChecked() ? track.right() - knobSize - 4.0 : track.left() + 4.0;
    painter.setPen(Qt::NoPen);
    painter.setBrush(isChecked() ? (colors.dark ? QColor(Qt::black) : QColor(Qt::white))
                                 : colors.dim);
    painter.drawEllipse(QRectF(knobX, track.center().y() - knobSize / 2.0, knobSize, knobSize));
}

QFrame* makeCard(QWidget* parent)
{
    auto* frame = new QFrame(parent);
    frame->setObjectName(QStringLiteral("Card"));
    frame->setFrameShape(QFrame::NoFrame);
    return frame;
}

QFrame* makeInsetPanel(QWidget* parent)
{
    auto* frame = new QFrame(parent);
    frame->setObjectName(QStringLiteral("InsetPanel"));
    frame->setFrameShape(QFrame::NoFrame);
    return frame;
}

QWidget* makeHorizontalRule(QWidget* parent)
{
    auto* rule = new QFrame(parent);
    rule->setFixedHeight(1);
    rule->setStyleSheet(QStringLiteral("background: %1; border: none;")
                            .arg(ThemeManager::instance().current().border.name()));
    QObject::connect(&ThemeManager::instance(), &ThemeManager::themeChanged, rule, [rule] {
        rule->setStyleSheet(QStringLiteral("background: %1; border: none;")
                                .arg(ThemeManager::instance().current().border.name()));
    });
    return rule;
}

QLabel* makeLabel(const QString& text, const QString& objectName, QWidget* parent)
{
    auto* label = new QLabel(text, parent);
    if (!objectName.isEmpty()) {
        label->setObjectName(objectName);
    }
    return label;
}

QPixmap emojiPixmap(const QString& emoji, int size, qreal devicePixelRatio)
{
    if (emoji.isEmpty() || size <= 0) {
        return {};
    }
    qreal dpr = devicePixelRatio;
    if (dpr <= 0.0) {
        dpr = qApp != nullptr ? qApp->devicePixelRatio() : 1.0;
    }
    const QString key = QStringLiteral("pac-emoji-%1-%2-%3")
                            .arg(emoji)
                            .arg(size)
                            .arg(dpr, 0, 'f', 2);
    QPixmap cached;
    if (QPixmapCache::find(key, &cached)) {
        return cached;
    }
    QPixmap pixmap(qRound(size * dpr), qRound(size * dpr));
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.scale(dpr, dpr);
    QFont font(QStringLiteral("Segoe UI Emoji"));
    font.setPixelSize(qRound(size * 0.82));
    painter.setFont(font);
    painter.drawText(QRectF(0, 0, size, size), Qt::AlignCenter, emoji);
    painter.end();
    pixmap.setDevicePixelRatio(dpr);
    QPixmapCache::insert(key, pixmap);
    return pixmap;
}

QColor tinted(const QColor& color, qreal alpha)
{
    QColor result = color;
    result.setAlphaF(std::clamp(alpha, 0.0, 1.0));
    return result;
}

QIcon appIcon()
{
    static QIcon icon;
    if (!icon.isNull()) {
        return icon;
    }
    QIcon generated;
    for (const int size : {16, 24, 32, 48, 64, 128, 256}) {
        QPixmap pixmap(size, size);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const QRectF rect(0.5, 0.5, size - 1.0, size - 1.0);
        QLinearGradient background(rect.topLeft(), rect.bottomRight());
        background.setColorAt(0.0, QColor(QStringLiteral("#4a9be0")));
        background.setColorAt(0.55, QColor(QStringLiteral("#2466a8")));
        background.setColorAt(1.0, QColor(QStringLiteral("#153f6d")));
        painter.setPen(Qt::NoPen);
        painter.setBrush(background);
        painter.drawRoundedRect(rect, size * 0.22, size * 0.22);

        const qreal faceSize = size * 0.58;
        const QRectF face((size - faceSize) / 2.0, size * 0.24, faceSize, faceSize);
        painter.setBrush(QColor(QStringLiteral("#f7fbff")));
        painter.setPen(QPen(QColor(255, 255, 255, 189), std::max<qreal>(1.0, size * 0.018)));
        painter.drawEllipse(face);
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(QColor(QStringLiteral("#2a66a5")), std::max<qreal>(1.0, size * 0.035),
                            Qt::SolidLine, Qt::RoundCap));
        const QPointF center = face.center();
        painter.drawLine(center, center + QPointF(0, -face.height() * 0.28));
        painter.drawLine(center, center + QPointF(face.width() * 0.22, face.height() * 0.08));

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(QStringLiteral("#f0a03c")));
        painter.drawEllipse(QPointF(size * 0.24, size * 0.22), size * 0.075, size * 0.075);
        painter.drawEllipse(QPointF(size * 0.76, size * 0.22), size * 0.075, size * 0.075);
        painter.setBrush(QColor(QStringLiteral("#f7fbff")));
        painter.drawEllipse(center, size * 0.045, size * 0.045);

        painter.setBrush(QColor(QStringLiteral("#f0a03c")));
        QPainterPath bell;
        bell.moveTo(size * 0.30, size * 0.20);
        bell.cubicTo(size * 0.38, size * 0.06, size * 0.62, size * 0.06, size * 0.70, size * 0.20);
        bell.closeSubpath();
        painter.drawPath(bell);

        painter.end();
        generated.addPixmap(pixmap);
    }
    icon = generated;
    return icon;
}

} // namespace pac::ui
