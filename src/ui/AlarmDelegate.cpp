#include "AlarmDelegate.h"

#include "AlarmTableModel.h"
#include "UiKit.h"
#include "core/Icons.h"
#include "core/Settings.h"
#include "core/Theme.h"

#include <QEvent>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

#include <algorithm>

namespace pac::ui {
namespace {

QFont fontWith(const QFont& base, qreal pointSize, QFont::Weight weight = QFont::Normal,
               bool tabular = false)
{
    QFont font = base;
    font.setPointSizeF(pointSize);
    font.setWeight(weight);
    if (tabular) {
        font.setFeature(QFont::Tag("tnum"), 1);
    }
    return font;
}

QRectF checkboxRect(const QRect& cell)
{
    constexpr qreal size = 17.0;
    return QRectF(cell.center().x() - size / 2.0, cell.center().y() - size / 2.0, size, size);
}

} // namespace

AlarmDelegate::AlarmDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

int AlarmDelegate::alarmRowHeight()
{
    switch (Settings::instance().rowDensity()) {
    case 0:
        return 34;
    case 2:
        return 50;
    default:
        return 42;
    }
}

QSize AlarmDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if (index.data(AlarmTableModel::IsGroupRole).toBool()) {
        return QSize(option.rect.width(), 30);
    }
    return QSize(option.rect.width(), alarmRowHeight());
}

void AlarmDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                          const QModelIndex& index) const
{
    const ThemeColors& colors = ThemeManager::instance().current();
    const bool group = index.data(AlarmTableModel::IsGroupRole).toBool();
    const bool selected = option.state.testFlag(QStyle::State_Selected);
    const bool hovered = option.state.testFlag(QStyle::State_MouseOver);
    const bool showGrid = Settings::instance().showGrid();
    const bool alternate = Settings::instance().alternateRows();
    const QRect cell = option.rect;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    if (group) {
        QLinearGradient gradient(cell.topLeft(), cell.bottomLeft());
        gradient.setColorAt(0.0, tinted(colors.headerTop, colors.dark ? 0.9 : 0.95));
        gradient.setColorAt(1.0, tinted(colors.panel, colors.dark ? 0.8 : 0.75));
        painter->fillRect(cell, gradient);
        if (hovered) {
            painter->fillRect(cell, tinted(colors.hoverRow, 0.45));
        }
        painter->setPen(QPen(colors.border, 1.0));
        painter->drawLine(cell.left(), cell.bottom(), cell.right(), cell.bottom());
        if (selected) {
            painter->fillRect(QRect(cell.left(), cell.top(), cell.width(), 1), colors.accent);
        }

        const int count = index.data(AlarmTableModel::CountRole).toInt();
        const bool collapsed = index.data(AlarmTableModel::CollapsedRole).toBool();
        icons::paint(*painter, QRectF(cell.left() + 12.0, cell.top() + 8.5, 13.0, 13.0),
                     collapsed ? icons::Icon::ChevronRight : icons::Icon::ChevronDown,
                     colors.dim, 1.7);
        icons::paint(*painter, QRectF(cell.left() + 31.0, cell.top() + 7.5, 15.0, 15.0),
                     icons::Icon::Folder, tinted(colors.headerText, 0.9), 1.65);

        QFont nameFont = fontWith(option.font, 9.0, QFont::DemiBold);
        painter->setFont(nameFont);
        painter->setPen(colors.headerText);
        const QString name = index.data(Qt::DisplayRole).toString();
        const QFontMetrics nameMetrics(nameFont);
        const int nameWidth = nameMetrics.horizontalAdvance(name);
        painter->drawText(QPointF(cell.left() + 53.0, cell.center().y() + nameMetrics.ascent() / 2.0 - 1.0),
                          name);

        QFont countFont = fontWith(option.font, 7.5, QFont::DemiBold);
        const QFontMetrics countMetrics(countFont);
        const QString countText = QString::number(count);
        const int pillWidth = std::max(18, countMetrics.horizontalAdvance(countText) + 12);
        const QRectF pill(cell.left() + 60.0 + nameWidth, cell.center().y() - 8.0, pillWidth, 16.0);
        painter->setPen(Qt::NoPen);
        painter->setBrush(tinted(colors.accent, 0.13));
        painter->drawRoundedRect(pill, 8.0, 8.0);
        painter->setFont(countFont);
        painter->setPen(colors.accent);
        painter->drawText(pill, Qt::AlignCenter, countText);

        if (count == 0) {
            painter->setFont(fontWith(option.font, 8.0));
            painter->setPen(colors.faint);
            painter->drawText(QRectF(cell.left() + 63.0, cell.top(), 200.0, cell.height()),
                              Qt::AlignLeft | Qt::AlignVCenter, tr("нет будильников"));
        }
        painter->restore();
        return;
    }

    const bool enabled = index.data(AlarmTableModel::EnabledRole).toBool();
    const bool snoozed = index.data(AlarmTableModel::SnoozedRole).toBool();
    const QColor alarmColor = index.data(AlarmTableModel::AlarmColorRole).value<QColor>();
    const QTime alarmTime = index.data(AlarmTableModel::AlarmTimeRole).value<QTime>();
    const int alternateIndex = index.data(AlarmTableModel::AlternateRole).toInt();

    QColor background = colors.card;
    if (selected) {
        background = colors.selection;
    } else if (hovered) {
        background = colors.hoverRow;
    } else if (alternate && alternateIndex % 2 == 1) {
        background = colors.altRow;
    }
    painter->fillRect(cell, background);

    if (selected && index.column() == 0) {
        painter->fillRect(QRect(cell.left(), cell.top(), 3, cell.height()), colors.accent);
    }

    if (showGrid) {
        painter->setPen(QPen(tinted(colors.grid, selected ? 0.6 : 0.9), 1.0));
        painter->drawLine(cell.left(), cell.bottom(), cell.right(), cell.bottom());
        if (index.column() < AlarmTableModel::ColumnCount - 1) {
            painter->drawLine(cell.right(), cell.top(), cell.right(), cell.bottom());
        }
    } else {
        painter->setPen(QPen(tinted(colors.grid, 0.45), 1.0));
        painter->drawLine(cell.left(), cell.bottom(), cell.right(), cell.bottom());
    }

    const QColor primary = enabled ? colors.text : colors.dim;
    const QColor secondary = enabled ? colors.dim : colors.faint;
    const int padding = 10;

    switch (index.column()) {
    case AlarmTableModel::Enabled: {
        const QRectF box = checkboxRect(cell);
        if (enabled) {
            painter->setPen(QPen(colors.success.darker(colors.dark ? 100 : 120), 1.0));
            painter->setBrush(colors.success);
        } else {
            painter->setPen(QPen(colors.inputBorder, 1.0));
            painter->setBrush(colors.dark ? colors.inputBg : colors.card);
        }
        painter->drawRoundedRect(box, 3.5, 3.5);
        if (enabled) {
            icons::paint(*painter, box.adjusted(3.2, 3.2, -3.2, -3.2), icons::Icon::Check,
                         QColor(255, 255, 255, 235), 2.1);
        }
        break;
    }
    case AlarmTableModel::Dial: {
        const qreal diameter = std::clamp<qreal>(cell.height() - 12.0, 22.0, 30.0);
        const QRectF dial(cell.center().x() - diameter / 2.0, cell.center().y() - diameter / 2.0,
                          diameter, diameter);
        QColor rimColor = enabled ? alarmColor : colors.faint;
        if (!enabled) {
            rimColor.setAlpha(160);
        }
        const QColor faceColor = enabled ? colors.card : colors.panel;
        const QColor tickColor = tinted(rimColor, 0.7);
        const QColor handColor = enabled ? colors.text : colors.faint;
        icons::paintClockFace(*painter, dial, alarmTime, rimColor, faceColor, tickColor, handColor,
                              false);
        break;
    }
    case AlarmTableModel::Countdown: {
        QFont font = fontWith(option.font, 8.8, QFont::Medium, true);
        painter->setFont(font);
        painter->setPen(snoozed && enabled ? colors.warning : (enabled ? colors.text : colors.faint));
        const QString text = index.data(Qt::DisplayRole).toString();
        const QFontMetrics metrics(font);
        painter->drawText(QRect(cell.left() + padding, cell.top(),
                                cell.width() - padding - 6, cell.height()),
                          Qt::AlignLeft | Qt::AlignVCenter,
                          metrics.elidedText(text, Qt::ElideRight, cell.width() - padding - 8));
        break;
    }
    case AlarmTableModel::Time: {
        QFont font = fontWith(option.font, 10.5, QFont::DemiBold);
        painter->setFont(font);
        painter->setPen(primary);
        const QString text = index.data(Qt::DisplayRole).toString();
        painter->drawText(QRect(cell.left() + padding, cell.top(), cell.width() - padding - 4,
                                cell.height()),
                          Qt::AlignLeft | Qt::AlignVCenter, text);
        break;
    }
    case AlarmTableModel::Repeat: {
        QFont font = fontWith(option.font, 8.3);
        painter->setFont(font);
        painter->setPen(secondary);
        const QFontMetrics metrics(font);
        const QString text = metrics.elidedText(index.data(Qt::DisplayRole).toString(),
                                                Qt::ElideRight, cell.width() - padding - 8);
        painter->drawText(QRect(cell.left() + padding, cell.top(), cell.width() - padding - 8,
                                cell.height()),
                          Qt::AlignLeft | Qt::AlignVCenter, text);
        break;
    }
    case AlarmTableModel::Emoji: {
        const qreal size = std::clamp<qreal>(cell.height() - 16.0, 22.0, 28.0);
        const QRectF chip(cell.center().x() - size / 2.0, cell.center().y() - size / 2.0, size, size);
        painter->setPen(Qt::NoPen);
        painter->setBrush(tinted(alarmColor, enabled ? 0.16 : 0.08));
        painter->drawRoundedRect(chip, size * 0.28, size * 0.28);
        const QString emoji = index.data(AlarmTableModel::AlarmIconRole).toString();
        if (!emoji.isEmpty()) {
            const QPixmap emojiImage = emojiPixmap(emoji, static_cast<int>(size - 6));
            painter->drawPixmap(QRectF(chip.center().x() - (size - 6) / 2.0,
                                       chip.center().y() - (size - 6) / 2.0, size - 6, size - 6)
                                    .toRect(),
                                emojiImage);
        } else {
            icons::paint(*painter, chip.adjusted(4, 4, -4, -4), icons::Icon::Bell, alarmColor, 1.8);
        }
        break;
    }
    case AlarmTableModel::Label: {
        QFont font = fontWith(option.font, 9.3, QFont::DemiBold);
        painter->setFont(font);
        painter->setPen(primary);
        qreal textLeft = cell.left() + padding;
        if (snoozed && enabled) {
            icons::paint(*painter, QRectF(textLeft, cell.center().y() - 6.5, 13.0, 13.0),
                         icons::Icon::Snooze, colors.warning, 1.7);
            textLeft += 18.0;
        }
        const QFontMetrics metrics(font);
        const QString text = metrics.elidedText(index.data(Qt::DisplayRole).toString(),
                                                Qt::ElideRight,
                                                cell.width() - static_cast<int>(textLeft - cell.left()) - 8);
        painter->drawText(QRectF(textLeft, cell.top(), cell.width() - (textLeft - cell.left()) - 8,
                                 cell.height()),
                          Qt::AlignLeft | Qt::AlignVCenter, text);
        break;
    }
    default:
        break;
    }

    painter->restore();
}

bool AlarmDelegate::editorEvent(QEvent* event, QAbstractItemModel* model,
                                const QStyleOptionViewItem& option, const QModelIndex& index)
{
    if (event->type() == QEvent::MouseButtonRelease
        && index.column() == AlarmTableModel::Enabled
        && !index.data(AlarmTableModel::IsGroupRole).toBool()) {
        auto* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton
            && checkboxRect(option.rect).contains(mouseEvent->position())) {
            const Qt::CheckState current = static_cast<Qt::CheckState>(
                index.data(Qt::CheckStateRole).toInt());
            model->setData(index, current == Qt::Checked ? Qt::Unchecked : Qt::Checked,
                           Qt::CheckStateRole);
            return true;
        }
    }
    return QStyledItemDelegate::editorEvent(event, model, option, index);
}

} // namespace pac::ui
