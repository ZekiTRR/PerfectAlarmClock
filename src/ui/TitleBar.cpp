#include "TitleBar.h"

#include "UiKit.h"
#include "core/Fonts.h"
#include "core/Theme.h"

#include <QAbstractButton>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainter>
#include <QWindow>

namespace pac::ui {

class WindowButton final : public QAbstractButton {
public:
    WindowButton(icons::Icon icon, const QString& tooltip, bool closeButton, QWidget* parent = nullptr)
        : QAbstractButton(parent)
        , m_icon(icon)
        , m_close(closeButton)
    {
        setToolTip(tooltip);
        setFixedSize(44, 34);
        setFocusPolicy(Qt::NoFocus);
        setCursor(Qt::ArrowCursor);
        setAttribute(Qt::WA_Hover, true);
    }

    void setGlyph(icons::Icon icon)
    {
        m_icon = icon;
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event)
        const ThemeColors& colors = ThemeManager::instance().current();
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const bool hot = underMouse();
        QColor background = Qt::transparent;
        QColor glyph = colors.tabMuted;
        if (hot) {
            if (m_close) {
                background = QColor(QStringLiteral("#e81123"));
                glyph = QColor(Qt::white);
            } else {
                background = tinted(colors.tabText, colors.dark ? 0.18 : 0.10);
                glyph = colors.tabText;
            }
        }
        if (isDown()) {
            background = m_close ? QColor(QStringLiteral("#c50f1f"))
                                 : tinted(colors.tabText, 0.18);
        }
        painter.fillRect(rect(), background);
        const qreal glyphSize = 12.0;
        icons::paint(painter,
                     QRectF(width() / 2.0 - glyphSize / 2.0, height() / 2.0 - glyphSize / 2.0,
                            glyphSize, glyphSize),
                     m_icon, glyph, 1.55);
    }

private:
    icons::Icon m_icon;
    bool m_close{false};
};

TitleBar::TitleBar(QWidget* parent)
    : QWidget(parent)
{
    setFixedHeight(34);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(16, 0, 0, 0);
    layout->setSpacing(8);

    m_icon = new QLabel(this);
    m_icon->setFixedSize(18, 18);
    m_icon->setPixmap(appIcon().pixmap(18, 18));
    m_icon->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_icon->hide();
    layout->addWidget(m_icon);

    m_title = new QLabel(QStringLiteral("Perfect Alarm Clock"), this);
    m_title->setAttribute(Qt::WA_TransparentForMouseEvents);
    layout->addWidget(m_title);
    layout->addStretch(1);

    m_clock = new QLabel(this);
    m_clock->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_clock->setContentsMargins(0, 0, 10, 0);
    layout->addWidget(m_clock);

    m_minimize = new WindowButton(icons::Icon::Minimize, tr("Свернуть"), false, this);
    m_maximize = new WindowButton(icons::Icon::Maximize, tr("Развернуть"), false, this);
    m_close = new WindowButton(icons::Icon::Close, tr("Закрыть"), true, this);
    auto* windowButtons = new QHBoxLayout;
    windowButtons->setContentsMargins(0, 0, 0, 0);
    windowButtons->setSpacing(0);
    windowButtons->addWidget(m_minimize);
    windowButtons->addWidget(m_maximize);
    windowButtons->addWidget(m_close);
    layout->addLayout(windowButtons);

    connect(m_minimize, &QAbstractButton::clicked, this, &TitleBar::minimizeRequested);
    connect(m_maximize, &QAbstractButton::clicked, this, &TitleBar::maximizeRestoreRequested);
    connect(m_close, &QAbstractButton::clicked, this, &TitleBar::closeRequested);
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this] {
        applyTheme();
        update();
    });
    applyTheme();
}

void TitleBar::setClockText(const QString& text)
{
    if (m_clock->text() != text) {
        m_clock->setText(text);
    }
}

void TitleBar::setClockVisible(bool visible)
{
    m_clock->setVisible(visible && !m_fluent);
}

void TitleBar::setFluentMode(bool fluent)
{
    if (m_fluent == fluent) {
        return;
    }
    m_fluent = fluent;
    m_clock->setVisible(!fluent);
    update();
}

void TitleBar::setMaximized(bool maximized)
{
    if (m_maximized == maximized) {
        return;
    }
    m_maximized = maximized;
    m_maximize->setGlyph(maximized ? icons::Icon::Restore : icons::Icon::Maximize);
    m_maximize->setToolTip(maximized ? tr("Восстановить") : tr("Развернуть"));
    update();
}

void TitleBar::applyTheme()
{
    const ThemeColors& colors = ThemeManager::instance().current();
    QFont titleFont = m_title->font();
    titleFont.setPointSizeF(9.4);
    titleFont.setWeight(QFont::DemiBold);
    m_title->setFont(titleFont);
    m_title->setStyleSheet(QStringLiteral("color:%1; background:transparent;")
                               .arg(colors.text.name()));

    m_clock->setFont(fonts::digitFont(8.2));
    m_clock->setStyleSheet(QStringLiteral("color:%1; background:transparent;")
                               .arg(colors.tabMuted.name()));
}

void TitleBar::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    const ThemeColors& colors = ThemeManager::instance().current();
    QPainter painter(this);
    QLinearGradient gradient(0.0, 0.0, 0.0, height());
    gradient.setColorAt(0.0, colors.ribbonTop);
    gradient.setColorAt(1.0, colors.tabActiveBottom);
    painter.fillRect(rect(), gradient);
    painter.setPen(QPen(QColor(255, 255, 255, colors.dark ? 16 : 115), 1.0));
    painter.drawLine(0, 0, width(), 0);
    painter.setPen(QPen(colors.ribbonBorder, 1.0));
    painter.drawLine(0, height() - 1, width(), height() - 1);
}

void TitleBar::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_pressPosition = event->globalPosition().toPoint();
        m_maybeDrag = true;
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void TitleBar::mouseMoveEvent(QMouseEvent* event)
{
    if (m_maybeDrag && (event->buttons() & Qt::LeftButton) && window() != nullptr
        && window()->windowHandle() != nullptr && !window()->isMaximized()
        && !window()->isFullScreen()) {
        if ((event->globalPosition().toPoint() - m_pressPosition).manhattanLength() >= 4) {
            m_maybeDrag = false;
            window()->windowHandle()->startSystemMove();
        }
    }
    QWidget::mouseMoveEvent(event);
}

void TitleBar::mouseReleaseEvent(QMouseEvent* event)
{
    m_maybeDrag = false;
    QWidget::mouseReleaseEvent(event);
}

void TitleBar::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        emit maximizeRestoreRequested();
        event->accept();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}

} // namespace pac::ui
