#include "Ribbon.h"

#include "core/Theme.h"

#include <QEnterEvent>
#include <QFontMetrics>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QScrollArea>
#include <QScrollBar>
#include <QStackedWidget>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWheelEvent>

#include <algorithm>
#include <utility>

namespace pac::ui {
namespace {

constexpr int kTabBarHeight = 30;
constexpr int kContentHeight = 92;

[[nodiscard]] bool isGlossy()
{
    return ThemeManager::instance().current().flavor == ThemeFlavor::Office2007;
}

} // namespace

RibbonButton::RibbonButton(Size size, icons::Icon icon, QString text, QString tooltip, QColor accent,
                           QWidget* parent)
    : QAbstractButton(parent)
    , m_size(size)
    , m_icon(icon)
    , m_accent(std::move(accent))
{
    setText(std::move(text));
    setToolTip(std::move(tooltip));
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

QSize RibbonButton::sizeHint() const
{
    if (m_size == Size::Large) {
        return QSize(68, 68);
    }
    const QFontMetrics metrics(font());
    const int textWidth = metrics.horizontalAdvance(text());
    return QSize(std::clamp(textWidth + 32, 78, 190), 22);
}

void RibbonButton::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    const ThemeColors& colors = ThemeManager::instance().current();
    const bool enabled = isEnabled();
    const QRectF bounds = QRectF(rect()).adjusted(1.0, 1.0, -1.0, -1.0);

    if (isChecked()) {
        painter.setPen(QPen(tinted(colors.accent, 0.45), 1.0));
        painter.setBrush(tinted(colors.accent, 0.13));
        painter.drawRoundedRect(bounds, 3.5, 3.5);
    } else if (isDown()) {
        painter.setPen(QPen(colors.ribbonBorder, 1.0));
        painter.setBrush(tinted(colors.ribbonBorder, colors.dark ? 0.35 : 0.22));
        painter.drawRoundedRect(bounds, 3.5, 3.5);
    } else if (m_hovered && enabled) {
        painter.setPen(QPen(tinted(colors.ribbonBorder, 0.7), 1.0));
        painter.setBrush(tinted(colors.tabHover, 0.72));
        painter.drawRoundedRect(bounds, 3.5, 3.5);
    }

    const QColor iconColor = enabled ? QColor(Qt::white) : QColor(255, 255, 255, 125);
    const QColor textColor = enabled ? colors.tabText : colors.faint;

    if (m_size == Size::Large) {
        const QRectF tile(bounds.center().x() - 16.0, 5.0, 32.0, 32.0);
        QColor accent = enabled ? m_accent : QColor::fromHsl(m_accent.hslHue(), 25, 150);
        QLinearGradient gradient(tile.topLeft(), tile.bottomLeft());
        gradient.setColorAt(0.0, accent.lighter(128));
        gradient.setColorAt(0.48, accent.lighter(106));
        gradient.setColorAt(1.0, accent.darker(118));
        painter.setPen(QPen(accent.darker(155), 1.0));
        painter.setBrush(gradient);
        painter.drawRoundedRect(tile, 5.0, 5.0);

        if (isGlossy()) {
            QPainterPath gloss;
            gloss.addRoundedRect(tile.adjusted(1.0, 1.0, -1.0, -tile.height() * 0.45), 4.0, 4.0);
            painter.fillPath(gloss, QColor(255, 255, 255, 52));
        } else {
            painter.setPen(QPen(QColor(255, 255, 255, 45), 1.0));
            painter.drawLine(tile.topLeft() + QPointF(5.0, 1.5), tile.topRight() + QPointF(-5.0, 1.5));
        }
        icons::paint(painter, tile.adjusted(5.0, 5.0, -5.0, -5.0), m_icon, iconColor, 1.85);

        QFont labelFont = font();
        labelFont.setPointSizeF(8.0);
        labelFont.setWeight(QFont::Medium);
        painter.setFont(labelFont);
        painter.setPen(textColor);
        painter.drawText(QRectF(0.0, 40.0, width(), 27.0),
                         Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap, text());
    } else {
        const QRectF iconRect(5.0, (height() - 16.0) / 2.0, 16.0, 16.0);
        const QColor smallIconColor = enabled ? m_accent : colors.faint;
        icons::paint(painter, iconRect, m_icon, smallIconColor, 1.8);
        QFont smallFont = font();
        smallFont.setPointSizeF(8.0);
        painter.setFont(smallFont);
        painter.setPen(textColor);
        const QFontMetrics metrics(smallFont);
        const QString elided = metrics.elidedText(text(), Qt::ElideRight, width() - 30);
        painter.drawText(QRectF(25.0, 0.0, width() - 28.0, height()),
                         Qt::AlignLeft | Qt::AlignVCenter, elided);
        if (isChecked()) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(colors.accent);
            painter.drawEllipse(QPointF(width() - 5.0, height() / 2.0), 2.0, 2.0);
        }
    }

    if (hasFocus()) {
        painter.setPen(QPen(tinted(colors.accent, 0.8), 1.0, Qt::DotLine));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(bounds.adjusted(2.0, 2.0, -2.0, -2.0), 3.0, 3.0);
    }
}

void RibbonButton::enterEvent(QEnterEvent* event)
{
    m_hovered = true;
    update();
    QAbstractButton::enterEvent(event);
}

void RibbonButton::leaveEvent(QEvent* event)
{
    m_hovered = false;
    update();
    QAbstractButton::leaveEvent(event);
}

void RibbonButton::focusInEvent(QFocusEvent* event)
{
    update();
    QAbstractButton::focusInEvent(event);
}

void RibbonButton::focusOutEvent(QFocusEvent* event)
{
    update();
    QAbstractButton::focusOutEvent(event);
}

RibbonGroup::RibbonGroup(const QString& title, QWidget* parent)
    : QWidget(parent)
    , m_title(title)
{
    setFixedHeight(88);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 2, 5, 16);
    layout->setSpacing(0);
    m_row = new QHBoxLayout;
    m_row->setContentsMargins(0, 0, 0, 0);
    m_row->setSpacing(2);
    layout->addLayout(m_row);
}

RibbonButton* RibbonGroup::addLarge(icons::Icon icon, const QString& text, const QString& tooltip,
                                    const QColor& accent)
{
    closeSmallColumn();
    auto* button = new RibbonButton(RibbonButton::Size::Large, icon, text, tooltip, accent, this);
    m_row->addWidget(button, 0, Qt::AlignTop);
    return button;
}

RibbonButton* RibbonGroup::addSmall(icons::Icon icon, const QString& text, const QString& tooltip,
                                    const QColor& accent)
{
    if (m_smallColumn == nullptr || m_smallCount >= 3) {
        m_smallColumn = new QVBoxLayout;
        m_smallColumn->setContentsMargins(0, 0, 0, 0);
        m_smallColumn->setSpacing(1);
        m_row->addLayout(m_smallColumn, 0);
        m_smallCount = 0;
    }
    auto* button = new RibbonButton(RibbonButton::Size::Small, icon, text, tooltip, accent, this);
    m_smallColumn->addWidget(button, 0, Qt::AlignLeft | Qt::AlignTop);
    ++m_smallCount;
    return button;
}

void RibbonGroup::addWidget(QWidget* widget)
{
    closeSmallColumn();
    m_row->addWidget(widget, 0, Qt::AlignVCenter);
}

void RibbonGroup::closeSmallColumn()
{
    m_smallColumn = nullptr;
    m_smallCount = 0;
}

void RibbonGroup::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    const ThemeColors& colors = ThemeManager::instance().current();
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(tinted(colors.groupLine, colors.dark ? 0.9 : 0.85), 1.0));
    painter.drawLine(QPointF(width() - 1.0, 7.0), QPointF(width() - 1.0, height() - 17.0));

    QFont labelFont = font();
    labelFont.setPointSizeF(7.5);
    painter.setFont(labelFont);
    painter.setPen(colors.groupText);
    const QRect titleRect(2, height() - 16, width() - 4, 13);
    const QString title = painter.fontMetrics().elidedText(m_title, Qt::ElideRight, titleRect.width());
    painter.drawText(titleRect, Qt::AlignHCenter | Qt::AlignVCenter, title);
}

RibbonPage::RibbonPage(QWidget* parent)
    : QWidget(parent)
{
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(7, 2, 7, 2);
    m_layout->setSpacing(0);
    m_layout->addStretch(1);
}

RibbonGroup* RibbonPage::addGroup(const QString& title)
{
    auto* group = new RibbonGroup(title, this);
    m_layout->insertWidget(m_layout->count() - 1, group, 0, Qt::AlignTop);
    return group;
}

void RibbonPage::addSpacer()
{
    auto* spacer = new QWidget(this);
    spacer->setFixedHeight(1);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    const int index = m_layout->count() - 1;
    m_layout->insertWidget(index, spacer, 50);
}

void RibbonPage::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    const ThemeColors& colors = ThemeManager::instance().current();
    QPainter painter(this);
    QLinearGradient gradient(0.0, 0.0, 0.0, height());
    gradient.setColorAt(0.0, colors.tabActiveBottom);
    gradient.setColorAt(0.35, colors.ribbonTop);
    gradient.setColorAt(1.0, colors.ribbonBottom);
    painter.fillRect(rect(), gradient);
    painter.setPen(QPen(QColor(255, 255, 255, colors.dark ? 18 : 95), 1.0));
    painter.drawLine(0, 1, width(), 1);
    painter.setPen(QPen(colors.ribbonBorder, 1.0));
    painter.drawLine(0, height() - 1, width(), height() - 1);
}

RibbonTabBar::RibbonTabBar(QWidget* parent)
    : QWidget(parent)
{
    setFixedHeight(kTabBarHeight);
    setMouseTracking(true);
}

void RibbonTabBar::addTab(const QString& id, const QString& title, icons::Icon icon)
{
    m_tabs.append({id, title, icon, 0});
    updateWidths();
    if (m_currentId.isEmpty()) {
        m_currentId = id;
    }
    update();
}

void RibbonTabBar::setCurrentId(const QString& id)
{
    if (m_currentId == id) {
        return;
    }
    for (const auto& tab : m_tabs) {
        if (tab.id == id) {
            m_currentId = id;
            update();
            return;
        }
    }
}

QString RibbonTabBar::currentId() const
{
    return m_currentId;
}

void RibbonTabBar::updateWidths()
{
    QFont font = this->font();
    font.setPointSizeF(8.5);
    font.setWeight(QFont::DemiBold);
    const QFontMetrics metrics(font);
    for (auto& tab : m_tabs) {
        tab.width = std::max(80, 14 + 7 + metrics.horizontalAdvance(tab.title) + 24);
    }
    update();
}

int RibbonTabBar::tabAt(int x) const
{
    int left = 80;
    for (int i = 0; i < m_tabs.size(); ++i) {
        const auto& tab = m_tabs.at(i);
        if (x >= left && x < left + tab.width) {
            return i;
        }
        left += tab.width;
    }
    return -1;
}

void RibbonTabBar::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    const ThemeColors& colors = ThemeManager::instance().current();
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF fileRect(8.0, 3.0, 64.0, height() - 3.0);
    QColor fileBase = colors.accent;
    if (m_fileHovered) {
        fileBase = fileBase.lighter(112);
    }
    QPainterPath filePath;
    filePath.addRoundedRect(fileRect, 4.0, 4.0);
    QLinearGradient fileGradient(fileRect.topLeft(), fileRect.bottomLeft());
    fileGradient.setColorAt(0.0, fileBase.lighter(isGlossy() ? 132 : 116));
    fileGradient.setColorAt(0.5, fileBase);
    fileGradient.setColorAt(1.0, colors.accentDeep);
    painter.setPen(QPen(colors.accentDeep, 1.0));
    painter.setBrush(fileGradient);
    painter.drawPath(filePath);
    if (isGlossy()) {
        QPainterPath gloss;
        gloss.addRoundedRect(fileRect.adjusted(1.0, 1.0, -1.0, -fileRect.height() * 0.5), 3.0, 3.0);
        painter.fillPath(gloss, QColor(255, 255, 255, 46));
    }
    QFont fileFont = font();
    fileFont.setPointSizeF(8.5);
    fileFont.setWeight(QFont::Bold);
    painter.setFont(fileFont);
    painter.setPen(QColor(255, 255, 255));
    painter.drawText(fileRect, Qt::AlignCenter, tr("Файл"));

    int left = 80;
    for (int i = 0; i < m_tabs.size(); ++i) {
        const auto& tab = m_tabs.at(i);
        const QRectF tabRect(left + 2.0, 4.0, tab.width - 4.0, height() - 4.0);
        const bool active = tab.id == m_currentId;
        const bool hovered = i == m_hoverIndex;

        if (active) {
            QPainterPath path;
            path.moveTo(tabRect.left(), tabRect.bottom());
            path.lineTo(tabRect.left(), tabRect.top() + 4.0);
            path.quadTo(tabRect.left(), tabRect.top(), tabRect.left() + 4.0, tabRect.top());
            path.lineTo(tabRect.right() - 4.0, tabRect.top());
            path.quadTo(tabRect.right(), tabRect.top(), tabRect.right(), tabRect.top() + 4.0);
            path.lineTo(tabRect.right(), tabRect.bottom());
            path.closeSubpath();
            QLinearGradient gradient(tabRect.topLeft(), tabRect.bottomLeft());
            gradient.setColorAt(0.0, colors.tabActiveTop);
            gradient.setColorAt(1.0, colors.tabActiveBottom);
            painter.setPen(QPen(colors.ribbonBorder, 1.0));
            painter.setBrush(gradient);
            painter.drawPath(path);
            painter.setPen(QPen(QColor(255, 255, 255, colors.dark ? 20 : 110), 1.0));
            painter.drawLine(tabRect.left() + 5.0, tabRect.top() + 1.0,
                             tabRect.right() - 5.0, tabRect.top() + 1.0);
        } else if (hovered) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(tinted(colors.tabHover, 0.8));
            painter.drawRoundedRect(tabRect.adjusted(1.0, 2.0, -1.0, 2.0), 4.0, 4.0);
        }

        QFont tabFont = font();
        tabFont.setPointSizeF(8.5);
        tabFont.setWeight(active ? QFont::DemiBold : QFont::Medium);
        painter.setFont(tabFont);
        painter.setPen(active ? colors.tabText : colors.tabMuted);
        const QFontMetrics metrics(tabFont);
        const int textWidth = metrics.horizontalAdvance(tab.title);
        const qreal total = 14.0 + 7.0 + textWidth;
        const qreal startX = tabRect.center().x() - total / 2.0;
        const QRectF iconRect(startX, tabRect.center().y() - 7.0, 14.0, 14.0);
        icons::paint(painter, iconRect, tab.icon,
                     active ? colors.tabText : tinted(colors.tabMuted, 0.9), 1.7);
        painter.drawText(QRectF(startX + 21.0, tabRect.top(), textWidth + 4.0, tabRect.height()),
                         Qt::AlignLeft | Qt::AlignVCenter, tab.title);

        left += tab.width;
    }
}

void RibbonTabBar::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }
    const QRectF fileRect(8.0, 3.0, 64.0, height() - 3.0);
    if (fileRect.contains(event->position())) {
        emit fileClicked(mapToGlobal(QPoint(static_cast<int>(fileRect.left()),
                                            static_cast<int>(fileRect.bottom() + 2.0))));
        return;
    }
    const int index = tabAt(static_cast<int>(event->position().x()));
    if (index >= 0) {
        const QString id = m_tabs.at(index).id;
        if (id != m_currentId) {
            m_currentId = id;
            update();
            emit currentChanged(id);
        }
    }
}

void RibbonTabBar::mouseMoveEvent(QMouseEvent* event)
{
    const QRectF fileRect(8.0, 3.0, 64.0, height() - 3.0);
    const bool fileHovered = fileRect.contains(event->position());
    const int index = fileHovered ? -1 : tabAt(static_cast<int>(event->position().x()));
    if (fileHovered != m_fileHovered || index != m_hoverIndex) {
        m_fileHovered = fileHovered;
        m_hoverIndex = index;
        update();
    }
}

void RibbonTabBar::leaveEvent(QEvent* event)
{
    Q_UNUSED(event)
    m_hoverIndex = -1;
    m_fileHovered = false;
    update();
}

RibbonBar::RibbonBar(QWidget* parent)
    : QWidget(parent)
{
    setFixedHeight(kTabBarHeight + kContentHeight);
    setMouseTracking(true);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_tabs = new RibbonTabBar(this);
    layout->addWidget(m_tabs);

    m_scroll = new QScrollArea(this);
    m_scroll->setObjectName(QStringLiteral("RibbonScroll"));
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setWidgetResizable(false);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scroll->setFocusPolicy(Qt::NoFocus);
    m_scroll->viewport()->setAutoFillBackground(false);
    m_scroll->setFixedHeight(kContentHeight);

    m_stack = new QStackedWidget(m_scroll);
    m_stack->setFixedHeight(kContentHeight);
    m_scroll->setWidget(m_stack);
    layout->addWidget(m_scroll);
    m_scroll->installEventFilter(this);
    m_scroll->viewport()->installEventFilter(this);
    connect(m_scroll->horizontalScrollBar(), &QScrollBar::rangeChanged, this,
            [this] { QTimer::singleShot(0, this, &RibbonBar::updateOverflow); });

    const auto makeScrollButton = [this](icons::Icon icon, const QString& tooltip) {
        auto* button = new QToolButton(this);
        button->setAutoRaise(true);
        button->setCursor(Qt::PointingHandCursor);
        button->setToolTip(tooltip);
        button->setFocusPolicy(Qt::NoFocus);
        button->setIconSize(QSize(14, 14));
        button->setFixedSize(20, 34);
        button->setProperty("glyph", static_cast<int>(icon));
        button->hide();
        return button;
    };
    m_scrollLeft = makeScrollButton(icons::Icon::ChevronLeft, tr("Прокрутить ленту влево"));
    m_scrollRight = makeScrollButton(icons::Icon::ChevronRight, tr("Прокрутить ленту вправо"));
    connect(m_scrollLeft, &QToolButton::clicked, this, [this] {
        QScrollBar* bar = m_scroll->horizontalScrollBar();
        bar->setValue(bar->value() - std::max(80, m_scroll->viewport()->width() * 3 / 4));
    });
    connect(m_scrollRight, &QToolButton::clicked, this, [this] {
        QScrollBar* bar = m_scroll->horizontalScrollBar();
        bar->setValue(bar->value() + std::max(80, m_scroll->viewport()->width() * 3 / 4));
    });
    connect(m_scroll->horizontalScrollBar(), &QScrollBar::valueChanged, this,
            [this] { updateOverflow(); });

    connect(m_tabs, &RibbonTabBar::currentChanged, this, [this](const QString& id) {
        const int index = m_pageIds.indexOf(id);
        if (index >= 0) {
            m_stack->setCurrentIndex(index);
            m_scroll->horizontalScrollBar()->setValue(0);
        }
        if (m_collapsed) {
            setRibbonCollapsed(false);
        }
        QTimer::singleShot(0, this, &RibbonBar::updateOverflow);
        emit pageChanged(id);
    });
    connect(m_tabs, &RibbonTabBar::fileClicked, this, &RibbonBar::fileMenuRequested);
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this] {
        refreshOverflowIcons();
        updateContentWidths();
        update();
    });
    refreshOverflowIcons();
}

void RibbonBar::addPage(const QString& id, const QString& title, icons::Icon icon)
{
    auto* page = new RibbonPage(m_stack);
    m_stack->addWidget(page);
    m_pageIds.append(id);
    m_tabs->addTab(id, title, icon);
    QTimer::singleShot(0, this, &RibbonBar::updateOverflow);
}

RibbonGroup* RibbonBar::addGroup(const QString& pageId, const QString& title)
{
    const int index = m_pageIds.indexOf(pageId);
    if (index < 0) {
        return nullptr;
    }
    auto* page = static_cast<RibbonPage*>(m_stack->widget(index));
    return page->addGroup(title);
}

void RibbonBar::addSpacer(const QString& pageId)
{
    const int index = m_pageIds.indexOf(pageId);
    if (index < 0) {
        return;
    }
    auto* page = static_cast<RibbonPage*>(m_stack->widget(index));
    page->addSpacer();
}

void RibbonBar::setCurrentPage(const QString& id)
{
    const int index = m_pageIds.indexOf(id);
    if (index < 0) {
        return;
    }
    m_tabs->setCurrentId(id);
    m_stack->setCurrentIndex(index);
    m_scroll->horizontalScrollBar()->setValue(0);
    if (m_collapsed) {
        setRibbonCollapsed(false);
    }
    QTimer::singleShot(0, this, &RibbonBar::updateOverflow);
    emit pageChanged(id);
}

QString RibbonBar::currentPage() const
{
    return m_tabs->currentId();
}

void RibbonBar::setRibbonCollapsed(bool collapsed)
{
    if (m_collapsed == collapsed) {
        return;
    }
    m_collapsed = collapsed;
    updateContentVisibility();
}

void RibbonBar::updateContentWidths()
{
    int widest = 0;
    for (int index = 0; index < m_stack->count(); ++index) {
        auto* page = static_cast<RibbonPage*>(m_stack->widget(index));
        if (page == nullptr || page->layout() == nullptr) {
            continue;
        }
        QLayout* layout = page->layout();
        const QMargins margins = layout->contentsMargins();
        int width = margins.left() + margins.right();
        int items = 0;
        for (int itemIndex = 0; itemIndex < layout->count(); ++itemIndex) {
            const int itemWidth = layout->itemAt(itemIndex)->sizeHint().width();
            if (itemWidth <= 0) {
                continue;
            }
            width += itemWidth;
            ++items;
        }
        if (items > 1) {
            width += layout->spacing() * (items - 1);
        }
        page->setMinimumWidth(width);
        widest = std::max(widest, width);
    }
    m_contentMinWidth = widest;
    m_stack->setMinimumWidth(widest);
    relayoutScrollContent();
    QTimer::singleShot(0, this, &RibbonBar::updateOverflow);
}

void RibbonBar::relayoutScrollContent()
{
    if (m_scroll == nullptr || m_stack == nullptr) {
        return;
    }
    const int viewportWidth = m_scroll->viewport()->width();
    if (viewportWidth <= 0) {
        return;
    }
    m_stack->resize(std::max(viewportWidth, std::max(m_contentMinWidth, m_stack->minimumWidth())),
                    kContentHeight);
}

void RibbonBar::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    const ThemeColors& colors = ThemeManager::instance().current();
    QPainter painter(this);
    QLinearGradient gradient(0.0, 0.0, 0.0, height());
    gradient.setColorAt(0.0, colors.ribbonTop);
    gradient.setColorAt(0.55, colors.ribbonTop);
    gradient.setColorAt(1.0, colors.ribbonBottom);
    painter.fillRect(rect(), gradient);
    painter.setPen(QPen(QColor(255, 255, 255, colors.dark ? 15 : 120), 1.0));
    painter.drawLine(0, 0, width(), 0);
    if (m_collapsed) {
        painter.setPen(QPen(colors.ribbonBorder, 1.0));
        painter.drawLine(0, height() - 1, width(), height() - 1);
    }
    const QColor separator = colors.groupLine;
    painter.setPen(QPen(separator, 1.0));
}

void RibbonBar::updateContentVisibility()
{
    m_scroll->setVisible(!m_collapsed);
    setFixedHeight(kTabBarHeight + (m_collapsed ? 0 : kContentHeight));
    update();
    QTimer::singleShot(0, this, &RibbonBar::updateOverflow);
}

void RibbonBar::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    relayoutScrollContent();
    QTimer::singleShot(0, this, &RibbonBar::updateOverflow);
}

bool RibbonBar::eventFilter(QObject* watched, QEvent* event)
{
    if ((watched == m_scroll || watched == m_scroll->viewport())
        && event->type() == QEvent::Wheel) {
        auto* wheel = static_cast<QWheelEvent*>(event);
        QScrollBar* bar = m_scroll->horizontalScrollBar();
        if (bar->maximum() > 0) {
            const int delta = wheel->angleDelta().y() != 0 ? wheel->angleDelta().y()
                                                           : wheel->angleDelta().x();
            if (delta != 0) {
                bar->setValue(bar->value() - delta);
                return true;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

void RibbonBar::refreshOverflowIcons()
{
    const QColor color = ThemeManager::instance().current().tabMuted;
    if (m_scrollLeft != nullptr) {
        m_scrollLeft->setIcon(icons::qicon(icons::Icon::ChevronLeft, color));
    }
    if (m_scrollRight != nullptr) {
        m_scrollRight->setIcon(icons::qicon(icons::Icon::ChevronRight, color));
    }
}

void RibbonBar::updateOverflow()
{
    if (m_scroll == nullptr || m_scrollLeft == nullptr || m_scrollRight == nullptr) {
        return;
    }
    relayoutScrollContent();
    QScrollBar* bar = m_scroll->horizontalScrollBar();
    const bool overflow = !m_collapsed && bar->maximum() > 0;
    const int y = m_scroll->y() + std::max(0, (m_scroll->height() - m_scrollLeft->height()) / 2);
    m_scrollLeft->move(m_scroll->x() + 1, y);
    m_scrollRight->move(std::max(0, m_scroll->x() + m_scroll->width()
                                        - m_scrollRight->width() - 1), y);
    m_scrollLeft->setVisible(overflow && bar->value() > 0);
    m_scrollRight->setVisible(overflow && bar->value() < bar->maximum());
    if (m_scrollLeft->isVisible()) {
        m_scrollLeft->raise();
    }
    if (m_scrollRight->isVisible()) {
        m_scrollRight->raise();
    }
}

} // namespace pac::ui
