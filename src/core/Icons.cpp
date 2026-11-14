#include "Icons.h"

#include <QApplication>
#include <QPainter>
#include <QPainterPath>
#include <QPixmapCache>
#include <QStringList>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace pac::icons {
namespace {

constexpr qreal kGrid = 24.0;

QPen makePen(const QColor& color, qreal width)
{
    QPen pen(color, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    pen.setJoinStyle(Qt::RoundJoin);
    return pen;
}

void drawArrowHead(QPainter& painter, const QPointF& tip, qreal angleDegrees, qreal length)
{
    const qreal radians = angleDegrees * (std::numbers::pi / 180.0);
    const QPointF back(std::cos(radians), std::sin(radians));
    const QPointF normal(-back.y(), back.x());
    QPainterPath path;
    path.moveTo(tip);
    path.lineTo(tip - back * length + normal * (length * 0.58));
    path.moveTo(tip);
    path.lineTo(tip - back * length - normal * (length * 0.58));
    painter.drawPath(path);
}

void drawGear(QPainter& painter)
{
    const QPointF center(12.0, 12.0);
    for (int i = 0; i < 8; ++i) {
        const qreal angle = i * 45.0 * (std::numbers::pi / 180.0);
        const QPointF offset(std::cos(angle), std::sin(angle));
        painter.drawLine(center + offset * 7.0, center + offset * 9.4);
    }
    painter.drawEllipse(center, 7.0, 7.0);
    painter.drawEllipse(center, 3.1, 3.1);
}

void drawBellBody(QPainter& painter)
{
    QPainterPath bell;
    bell.moveTo(6.2, 17.0);
    bell.cubicTo(7.3, 15.9, 7.5, 14.7, 7.5, 12.2);
    bell.cubicTo(7.5, 8.5, 9.4, 5.8, 12.0, 5.8);
    bell.cubicTo(14.6, 5.8, 16.5, 8.5, 16.5, 12.2);
    bell.cubicTo(16.5, 14.7, 16.7, 15.9, 17.8, 17.0);
    bell.closeSubpath();
    painter.drawPath(bell);
    painter.drawLine(QPointF(5.2, 17.2), QPointF(18.8, 17.2));
    painter.drawArc(QRectF(10.1, 18.0, 3.8, 3.2), 190 * 16, 160 * 16);
}

void paintPomodoro(QPainter& painter)
{
    painter.drawEllipse(QRectF(5.0, 8.0, 14.0, 12.5));
    QPainterPath leaf;
    leaf.moveTo(12.0, 8.0);
    leaf.cubicTo(9.0, 7.0, 8.2, 4.5, 10.4, 4.0);
    leaf.cubicTo(12.0, 3.6, 12.4, 5.6, 12.0, 8.0);
    leaf.cubicTo(12.8, 5.2, 15.2, 3.7, 16.5, 5.0);
    leaf.cubicTo(17.8, 6.2, 15.1, 7.9, 12.0, 8.0);
    painter.drawPath(leaf);
    painter.drawLine(QPointF(12.0, 4.2), QPointF(12.0, 2.8));
}

void paintCalendarBase(QPainter& painter)
{
    painter.drawRoundedRect(QRectF(3.7, 5.5, 16.6, 15.0), 2.0, 2.0);
    painter.drawLine(QPointF(3.9, 9.8), QPointF(20.1, 9.8));
    painter.drawLine(QPointF(8.0, 3.2), QPointF(8.0, 6.0));
    painter.drawLine(QPointF(16.0, 3.2), QPointF(16.0, 6.0));
    painter.drawLine(QPointF(7.2, 13.0), QPointF(9.2, 13.0));
    painter.drawLine(QPointF(11.0, 13.0), QPointF(13.0, 13.0));
    painter.drawLine(QPointF(7.2, 16.4), QPointF(9.2, 16.4));
}

} // namespace

void paint(QPainter& painter, const QRectF& rect, Icon icon, const QColor& color, qreal stroke)
{
    if (rect.isEmpty() || !color.isValid()) {
        return;
    }
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.translate(rect.topLeft());
    painter.scale(rect.width() / kGrid, rect.height() / kGrid);
    painter.setPen(makePen(color, stroke));
    painter.setBrush(Qt::NoBrush);

    switch (icon) {
    case Icon::Alarm:
    case Icon::AlarmPlus:
    case Icon::Clock: {
        const QRectF face = icon == Icon::Alarm ? QRectF(5.2, 6.8, 13.6, 13.6)
                                               : QRectF(4.6, 3.8, 14.8, 14.8);
        painter.drawEllipse(face);
        if (icon == Icon::Alarm) {
            painter.drawArc(QRectF(4.2, 2.4, 7.0, 7.0), 35 * 16, 110 * 16);
            painter.drawArc(QRectF(12.8, 2.4, 7.0, 7.0), 35 * 16, 110 * 16);
            painter.drawLine(QPointF(8.2, 20.2), QPointF(6.9, 21.8));
            painter.drawLine(QPointF(15.8, 20.2), QPointF(17.1, 21.8));
        }
        const QPointF center = face.center();
        painter.drawLine(center, center + QPointF(0.0, -face.height() * 0.28));
        painter.drawLine(center, center + QPointF(face.width() * 0.22, face.height() * 0.08));
        if (icon == Icon::AlarmPlus) {
            painter.drawLine(QPointF(18.0, 3.2), QPointF(18.0, 7.2));
            painter.drawLine(QPointF(16.0, 5.2), QPointF(20.0, 5.2));
        }
        break;
    }
    case Icon::Timer: {
        painter.drawLine(QPointF(6.0, 3.5), QPointF(18.0, 3.5));
        painter.drawLine(QPointF(6.0, 20.5), QPointF(18.0, 20.5));
        QPainterPath glass;
        glass.moveTo(7.0, 4.5);
        glass.lineTo(17.0, 4.5);
        glass.cubicTo(17.0, 9.0, 14.2, 10.2, 14.2, 12.0);
        glass.cubicTo(14.2, 13.8, 17.0, 15.0, 17.0, 19.5);
        glass.lineTo(7.0, 19.5);
        glass.cubicTo(7.0, 15.0, 9.8, 13.8, 9.8, 12.0);
        glass.cubicTo(9.8, 10.2, 7.0, 9.0, 7.0, 4.5);
        painter.drawPath(glass);
        painter.setBrush(color);
        QPainterPath sand;
        sand.moveTo(9.3, 17.7);
        sand.lineTo(14.7, 17.7);
        sand.cubicTo(14.0, 15.8, 12.9, 14.9, 12.0, 14.3);
        sand.cubicTo(11.1, 14.9, 10.0, 15.8, 9.3, 17.7);
        painter.drawPath(sand);
        break;
    }
    case Icon::Stopwatch: {
        painter.drawEllipse(QRectF(4.6, 6.0, 14.8, 14.8));
        painter.drawLine(QPointF(10.2, 2.8), QPointF(13.8, 2.8));
        painter.drawLine(QPointF(12.0, 2.8), QPointF(12.0, 5.8));
        painter.drawLine(QPointF(18.6, 7.2), QPointF(19.9, 5.9));
        const QPointF center(12.0, 13.4);
        painter.drawLine(center, QPointF(12.0, 9.6));
        painter.drawLine(center, QPointF(15.0, 14.5));
        break;
    }
    case Icon::Pomodoro:
        paintPomodoro(painter);
        break;
    case Icon::Calendar:
    case Icon::CalendarPlus:
        paintCalendarBase(painter);
        if (icon == Icon::CalendarPlus) {
            painter.drawLine(QPointF(16.2, 14.2), QPointF(16.2, 19.0));
            painter.drawLine(QPointF(13.8, 16.6), QPointF(18.6, 16.6));
        }
        break;
    case Icon::Globe: {
        painter.drawEllipse(QPointF(12.0, 12.0), 8.2, 8.2);
        painter.drawEllipse(QPointF(12.0, 12.0), 3.7, 8.2);
        painter.drawLine(QPointF(3.8, 12.0), QPointF(20.2, 12.0));
        QPainterPath latitude;
        latitude.moveTo(5.4, 7.6);
        latitude.cubicTo(9.0, 9.2, 15.0, 9.2, 18.6, 7.6);
        painter.drawPath(latitude);
        QPainterPath latitude2;
        latitude2.moveTo(5.4, 16.4);
        latitude2.cubicTo(9.0, 14.8, 15.0, 14.8, 18.6, 16.4);
        painter.drawPath(latitude2);
        break;
    }
    case Icon::Tasks:
    case Icon::TaskPlus: {
        painter.drawRoundedRect(QRectF(5.2, 3.4, 13.6, 17.2), 1.8, 1.8);
        painter.drawRoundedRect(QRectF(8.6, 2.0, 6.8, 3.4), 1.2, 1.2);
        for (int i = 0; i < 3; ++i) {
            const qreal y = 9.6 + i * 3.4;
            painter.drawRect(QRectF(8.0, y - 0.9, 1.8, 1.8));
            painter.drawLine(QPointF(11.2, y), QPointF(16.0, y));
        }
        if (icon == Icon::TaskPlus) {
            painter.drawLine(QPointF(16.6, 17.0), QPointF(16.6, 21.4));
            painter.drawLine(QPointF(14.4, 19.2), QPointF(18.8, 19.2));
        }
        break;
    }
    case Icon::Plus:
        painter.drawLine(QPointF(12.0, 4.0), QPointF(12.0, 20.0));
        painter.drawLine(QPointF(4.0, 12.0), QPointF(20.0, 12.0));
        break;
    case Icon::Edit: {
        QPainterPath pencil;
        pencil.moveTo(5.2, 18.8);
        pencil.lineTo(6.2, 14.6);
        pencil.lineTo(16.0, 4.8);
        pencil.lineTo(19.2, 8.0);
        pencil.lineTo(9.4, 17.8);
        pencil.closeSubpath();
        painter.drawPath(pencil);
        painter.drawLine(QPointF(14.6, 6.2), QPointF(17.8, 9.4));
        break;
    }
    case Icon::Copy:
        painter.drawRoundedRect(QRectF(8.0, 7.6, 11.0, 12.6), 1.6, 1.6);
        painter.drawRoundedRect(QRectF(5.0, 3.8, 11.0, 12.6), 1.6, 1.6);
        painter.drawLine(QPointF(8.0, 7.6), QPointF(8.0, 6.2));
        break;
    case Icon::Delete:
        painter.drawLine(QPointF(4.4, 6.6), QPointF(19.6, 6.6));
        painter.drawLine(QPointF(9.4, 3.6), QPointF(14.6, 3.6));
        painter.drawLine(QPointF(9.4, 3.6), QPointF(9.0, 6.4));
        painter.drawLine(QPointF(14.6, 3.6), QPointF(15.0, 6.4));
        painter.drawLine(QPointF(6.4, 7.4), QPointF(7.4, 20.2));
        painter.drawLine(QPointF(17.6, 7.4), QPointF(16.6, 20.2));
        painter.drawLine(QPointF(7.4, 20.2), QPointF(16.6, 20.2));
        painter.drawLine(QPointF(10.2, 10.0), QPointF(10.4, 17.6));
        painter.drawLine(QPointF(13.8, 10.0), QPointF(13.6, 17.6));
        break;
    case Icon::Settings:
        drawGear(painter);
        break;
    case Icon::Info:
        painter.drawEllipse(QPointF(12.0, 12.0), 8.5, 8.5);
        painter.drawLine(QPointF(12.0, 11.0), QPointF(12.0, 16.2));
        painter.drawPoint(QPointF(12.0, 8.1));
        break;
    case Icon::Help: {
        painter.drawEllipse(QPointF(12.0, 12.0), 8.5, 8.5);
        QPainterPath hook;
        hook.moveTo(9.4, 9.4);
        hook.cubicTo(9.4, 6.9, 14.6, 6.8, 14.6, 9.6);
        hook.cubicTo(14.6, 11.3, 12.0, 11.5, 12.0, 13.6);
        painter.drawPath(hook);
        painter.drawPoint(QPointF(12.0, 16.5));
        break;
    }
    case Icon::Theme:
        painter.drawEllipse(QPointF(12.0, 12.0), 8.4, 8.4);
        painter.save();
        painter.setPen(Qt::NoPen);
        painter.setBrush(color);
        painter.drawChord(QRectF(3.6, 3.6, 16.8, 16.8), 90 * 16, 180 * 16);
        painter.restore();
        break;
    case Icon::Search:
        painter.drawEllipse(QPointF(10.2, 10.2), 6.2, 6.2);
        painter.drawLine(QPointF(14.8, 14.8), QPointF(20.0, 20.0));
        break;
    case Icon::ChevronLeft:
        painter.drawLine(QPointF(14.6, 5.4), QPointF(8.2, 12.0));
        painter.drawLine(QPointF(8.2, 12.0), QPointF(14.6, 18.6));
        break;
    case Icon::ChevronRight:
        painter.drawLine(QPointF(9.4, 5.4), QPointF(15.8, 12.0));
        painter.drawLine(QPointF(15.8, 12.0), QPointF(9.4, 18.6));
        break;
    case Icon::ChevronDown:
        painter.drawLine(QPointF(5.4, 9.4), QPointF(12.0, 15.8));
        painter.drawLine(QPointF(12.0, 15.8), QPointF(18.6, 9.4));
        break;
    case Icon::ChevronUp:
        painter.drawLine(QPointF(5.4, 14.6), QPointF(12.0, 8.2));
        painter.drawLine(QPointF(12.0, 8.2), QPointF(18.6, 14.6));
        break;
    case Icon::Play: {
        painter.setBrush(color);
        painter.setPen(Qt::NoPen);
        QPainterPath triangle;
        triangle.moveTo(8.2, 4.6);
        triangle.lineTo(19.4, 12.0);
        triangle.lineTo(8.2, 19.4);
        triangle.closeSubpath();
        painter.drawPath(triangle);
        break;
    }
    case Icon::Pause:
        painter.setPen(Qt::NoPen);
        painter.setBrush(color);
        painter.drawRoundedRect(QRectF(7.0, 4.6, 3.5, 14.8), 1.2, 1.2);
        painter.drawRoundedRect(QRectF(13.5, 4.6, 3.5, 14.8), 1.2, 1.2);
        break;
    case Icon::Stop:
        painter.setPen(Qt::NoPen);
        painter.setBrush(color);
        painter.drawRoundedRect(QRectF(6.0, 6.0, 12.0, 12.0), 2.0, 2.0);
        break;
    case Icon::Reset: {
        painter.drawArc(QRectF(4.6, 4.6, 14.8, 14.8), 35 * 16, 300 * 16);
        drawArrowHead(painter, QPointF(17.4, 5.4), 125.0, 4.4);
        break;
    }
    case Icon::SkipForward: {
        painter.setBrush(color);
        painter.setPen(Qt::NoPen);
        QPainterPath triangle;
        triangle.moveTo(6.0, 5.8);
        triangle.lineTo(15.0, 12.0);
        triangle.lineTo(6.0, 18.2);
        triangle.closeSubpath();
        painter.drawPath(triangle);
        painter.drawRoundedRect(QRectF(16.2, 5.8, 2.4, 12.4), 1.0, 1.0);
        break;
    }
    case Icon::Flag: {
        painter.drawLine(QPointF(6.0, 3.2), QPointF(6.0, 21.0));
        painter.setBrush(color);
        QPainterPath flag;
        flag.moveTo(6.8, 4.2);
        flag.lineTo(19.0, 4.2);
        flag.lineTo(15.6, 8.6);
        flag.lineTo(19.0, 13.0);
        flag.lineTo(6.8, 13.0);
        flag.closeSubpath();
        painter.drawPath(flag);
        break;
    }
    case Icon::Bell:
        drawBellBody(painter);
        break;
    case Icon::Snooze:
        drawBellBody(painter);
        painter.drawLine(QPointF(16.0, 2.8), QPointF(19.8, 2.8));
        painter.drawLine(QPointF(19.8, 2.8), QPointF(16.0, 6.0));
        painter.drawLine(QPointF(16.0, 6.0), QPointF(19.8, 6.0));
        break;
    case Icon::Close:
        painter.drawLine(QPointF(6.0, 6.0), QPointF(18.0, 18.0));
        painter.drawLine(QPointF(18.0, 6.0), QPointF(6.0, 18.0));
        break;
    case Icon::Check:
        painter.drawLine(QPointF(4.6, 12.6), QPointF(9.8, 17.8));
        painter.drawLine(QPointF(9.8, 17.8), QPointF(19.6, 6.4));
        break;
    case Icon::Volume: {
        QPainterPath speaker;
        speaker.moveTo(4.0, 9.2);
        speaker.lineTo(7.6, 9.2);
        speaker.lineTo(11.6, 5.8);
        speaker.lineTo(11.6, 18.2);
        speaker.lineTo(7.6, 14.8);
        speaker.lineTo(4.0, 14.8);
        speaker.closeSubpath();
        painter.drawPath(speaker);
        painter.drawArc(QRectF(10.0, 8.0, 6.0, 8.0), -60 * 16, 120 * 16);
        painter.drawArc(QRectF(11.0, 5.4, 9.0, 13.2), -55 * 16, 110 * 16);
        break;
    }
    case Icon::Mute: {
        QPainterPath speaker;
        speaker.moveTo(4.0, 9.2);
        speaker.lineTo(7.6, 9.2);
        speaker.lineTo(11.6, 5.8);
        speaker.lineTo(11.6, 18.2);
        speaker.lineTo(7.6, 14.8);
        speaker.lineTo(4.0, 14.8);
        speaker.closeSubpath();
        painter.drawPath(speaker);
        painter.drawLine(QPointF(14.6, 9.4), QPointF(19.8, 14.6));
        painter.drawLine(QPointF(19.8, 9.4), QPointF(14.6, 14.6));
        break;
    }
    case Icon::Folder:
    case Icon::FolderPlus: {
        QPainterPath folder;
        folder.moveTo(3.4, 6.0);
        folder.lineTo(9.6, 6.0);
        folder.lineTo(11.4, 8.2);
        folder.lineTo(20.6, 8.2);
        folder.lineTo(20.6, 19.0);
        folder.lineTo(3.4, 19.0);
        folder.closeSubpath();
        painter.drawPath(folder);
        if (icon == Icon::FolderPlus) {
            painter.drawLine(QPointF(12.0, 11.0), QPointF(12.0, 16.4));
            painter.drawLine(QPointF(9.3, 13.7), QPointF(14.7, 13.7));
        }
        break;
    }
    case Icon::Save:
        painter.drawRoundedRect(QRectF(4.2, 3.8, 15.6, 16.4), 1.4, 1.4);
        painter.drawRect(QRectF(7.6, 3.8, 8.8, 5.2));
        painter.drawRect(QRectF(7.4, 13.2, 9.2, 7.0));
        painter.drawLine(QPointF(10.0, 5.8), QPointF(13.8, 5.8));
        break;
    case Icon::Import:
    case Icon::Export: {
        painter.drawPath([] {
            QPainterPath tray;
            tray.moveTo(4.0, 14.4);
            tray.lineTo(4.0, 19.6);
            tray.lineTo(20.0, 19.6);
            tray.lineTo(20.0, 14.4);
            return tray;
        }());
        if (icon == Icon::Import) {
            painter.drawLine(QPointF(12.0, 3.8), QPointF(12.0, 14.0));
            painter.drawLine(QPointF(7.8, 10.0), QPointF(12.0, 14.2));
            painter.drawLine(QPointF(16.2, 10.0), QPointF(12.0, 14.2));
        } else {
            painter.drawLine(QPointF(12.0, 14.0), QPointF(12.0, 3.8));
            painter.drawLine(QPointF(7.8, 7.8), QPointF(12.0, 3.6));
            painter.drawLine(QPointF(16.2, 7.8), QPointF(12.0, 3.6));
        }
        break;
    }
    case Icon::Moon: {
        QPainterPath crescent;
        crescent.setFillRule(Qt::OddEvenFill);
        crescent.addEllipse(QRectF(3.8, 2.4, 16.8, 19.2));
        crescent.addEllipse(QRectF(9.0, 0.4, 16.4, 17.6));
        painter.setPen(Qt::NoPen);
        painter.setBrush(color);
        painter.drawPath(crescent);
        break;
    }
    case Icon::Sun: {
        painter.drawEllipse(QPointF(12.0, 12.0), 4.2, 4.2);
        for (int i = 0; i < 8; ++i) {
            const qreal angle = i * 45.0 * (std::numbers::pi / 180.0);
            const QPointF dir(std::cos(angle), std::sin(angle));
            painter.drawLine(QPointF(12.0, 12.0) + dir * 6.4, QPointF(12.0, 12.0) + dir * 9.4);
        }
        break;
    }
    case Icon::Menu:
        painter.drawLine(QPointF(4.0, 6.5), QPointF(20.0, 6.5));
        painter.drawLine(QPointF(4.0, 12.0), QPointF(20.0, 12.0));
        painter.drawLine(QPointF(4.0, 17.5), QPointF(20.0, 17.5));
        break;
    case Icon::Grid:
        painter.drawRoundedRect(QRectF(4.0, 4.0, 6.6, 6.6), 1.0, 1.0);
        painter.drawRoundedRect(QRectF(13.4, 4.0, 6.6, 6.6), 1.0, 1.0);
        painter.drawRoundedRect(QRectF(4.0, 13.4, 6.6, 6.6), 1.0, 1.0);
        painter.drawRoundedRect(QRectF(13.4, 13.4, 6.6, 6.6), 1.0, 1.0);
        break;
    case Icon::List:
        for (int i = 0; i < 3; ++i) {
            const qreal y = 6.6 + i * 5.4;
            painter.drawEllipse(QPointF(5.2, y), 1.1, 1.1);
            painter.drawLine(QPointF(9.2, y), QPointF(19.8, y));
        }
        break;
    case Icon::Repeat: {
        QPainterPath top;
        top.moveTo(5.2, 10.4);
        top.lineTo(5.2, 8.2);
        top.cubicTo(5.2, 6.2, 6.8, 5.2, 8.6, 5.2);
        top.lineTo(16.2, 5.2);
        painter.drawPath(top);
        drawArrowHead(painter, QPointF(18.4, 10.0), 90.0, 4.0);
        painter.drawLine(QPointF(18.4, 5.4), QPointF(18.4, 10.2));
        QPainterPath bottom;
        bottom.moveTo(18.8, 13.6);
        bottom.lineTo(18.8, 15.8);
        bottom.cubicTo(18.8, 17.8, 17.2, 18.8, 15.4, 18.8);
        bottom.lineTo(7.8, 18.8);
        painter.drawPath(bottom);
        drawArrowHead(painter, QPointF(5.6, 14.0), 270.0, 4.0);
        painter.drawLine(QPointF(5.6, 18.6), QPointF(5.6, 13.8));
        break;
    }
    case Icon::More:
        painter.setPen(Qt::NoPen);
        painter.setBrush(color);
        painter.drawEllipse(QPointF(6.0, 12.0), 1.5, 1.5);
        painter.drawEllipse(QPointF(12.0, 12.0), 1.5, 1.5);
        painter.drawEllipse(QPointF(18.0, 12.0), 1.5, 1.5);
        break;
    case Icon::ArrowUp:
        painter.drawLine(QPointF(12.0, 20.0), QPointF(12.0, 5.0));
        painter.drawLine(QPointF(7.2, 9.4), QPointF(12.0, 4.6));
        painter.drawLine(QPointF(16.8, 9.4), QPointF(12.0, 4.6));
        break;
    case Icon::ArrowDown:
        painter.drawLine(QPointF(12.0, 4.0), QPointF(12.0, 19.0));
        painter.drawLine(QPointF(7.2, 14.6), QPointF(12.0, 19.4));
        painter.drawLine(QPointF(16.8, 14.6), QPointF(12.0, 19.4));
        break;
    case Icon::Sort:
        painter.drawLine(QPointF(4.2, 6.4), QPointF(13.4, 6.4));
        painter.drawLine(QPointF(4.2, 11.0), QPointF(10.8, 11.0));
        painter.drawLine(QPointF(4.2, 15.6), QPointF(8.2, 15.6));
        painter.drawLine(QPointF(17.0, 4.2), QPointF(17.0, 18.4));
        painter.drawLine(QPointF(13.8, 15.2), QPointF(17.0, 18.6));
        painter.drawLine(QPointF(20.2, 15.2), QPointF(17.0, 18.6));
        break;
    case Icon::Navigation:
        painter.drawRoundedRect(QRectF(3.6, 4.2, 16.8, 15.6), 1.6, 1.6);
        painter.drawLine(QPointF(9.4, 4.4), QPointF(9.4, 19.6));
        painter.drawLine(QPointF(5.8, 7.4), QPointF(7.2, 7.4));
        painter.drawLine(QPointF(5.8, 10.0), QPointF(7.2, 10.0));
        break;
    case Icon::StatusBar:
        painter.drawRoundedRect(QRectF(3.6, 4.2, 16.8, 15.6), 1.6, 1.6);
        painter.drawLine(QPointF(3.8, 16.2), QPointF(20.2, 16.2));
        painter.drawLine(QPointF(5.8, 18.2), QPointF(9.2, 18.2));
        break;
    case Icon::Density:
        for (int i = 0; i < 3; ++i) {
            const qreal y = 6.0 + i * 5.6;
            painter.drawRoundedRect(QRectF(4.0, y, 16.0, 3.2), 1.4, 1.4);
        }
        break;
    case Icon::Fullscreen:
        painter.drawLine(QPointF(4.4, 9.0), QPointF(4.4, 4.8));
        painter.drawLine(QPointF(4.4, 4.8), QPointF(8.8, 4.8));
        painter.drawLine(QPointF(19.6, 9.0), QPointF(19.6, 4.8));
        painter.drawLine(QPointF(19.6, 4.8), QPointF(15.2, 4.8));
        painter.drawLine(QPointF(4.4, 15.0), QPointF(4.4, 19.2));
        painter.drawLine(QPointF(4.4, 19.2), QPointF(8.8, 19.2));
        painter.drawLine(QPointF(19.6, 15.0), QPointF(19.6, 19.2));
        painter.drawLine(QPointF(19.6, 19.2), QPointF(15.2, 19.2));
        break;
    case Icon::Home: {
        painter.drawLine(QPointF(4.2, 11.4), QPointF(12.0, 4.2));
        painter.drawLine(QPointF(12.0, 4.2), QPointF(19.8, 11.4));
        painter.drawLine(QPointF(6.4, 10.0), QPointF(6.4, 20.0));
        painter.drawLine(QPointF(17.6, 10.0), QPointF(17.6, 20.0));
        painter.drawLine(QPointF(6.4, 20.0), QPointF(17.6, 20.0));
        painter.drawRect(QRectF(10.0, 14.6, 4.0, 5.4));
        break;
    }
    case Icon::View: {
        QPainterPath eye;
        eye.moveTo(3.0, 12.0);
        eye.cubicTo(6.0, 6.8, 18.0, 6.8, 21.0, 12.0);
        eye.cubicTo(18.0, 17.2, 6.0, 17.2, 3.0, 12.0);
        painter.drawPath(eye);
        painter.drawEllipse(QPointF(12.0, 12.0), 2.8, 2.8);
        break;
    }
    case Icon::Minimize:
        painter.drawLine(QPointF(5.5, 12.0), QPointF(18.5, 12.0));
        break;
    case Icon::Maximize:
        painter.drawRoundedRect(QRectF(6.0, 6.0, 12.0, 12.0), 1.2, 1.2);
        break;
    case Icon::Restore:
        painter.drawRoundedRect(QRectF(8.0, 5.5, 10.5, 10.5), 1.2, 1.2);
        painter.drawRoundedRect(QRectF(5.5, 8.0, 10.5, 10.5), 1.2, 1.2);
        break;
    case Icon::Pin: {
        QPainterPath pin;
        pin.moveTo(12.0, 20.6);
        pin.cubicTo(8.6, 16.2, 6.5, 13.6, 6.5, 10.4);
        pin.cubicTo(6.5, 7.0, 8.8, 4.6, 12.0, 4.6);
        pin.cubicTo(15.2, 4.6, 17.5, 7.0, 17.5, 10.4);
        pin.cubicTo(17.5, 13.6, 15.4, 16.2, 12.0, 20.6);
        pin.closeSubpath();
        painter.drawPath(pin);
        painter.drawEllipse(QPointF(12.0, 10.3), 2.3, 2.3);
        break;
    }
    case Icon::Star: {
        QPainterPath star;
        constexpr qreal pi = std::numbers::pi;
        for (int i = 0; i < 10; ++i) {
            const qreal radius = (i % 2 == 0) ? 8.6 : 3.8;
            const qreal angle = -pi / 2.0 + i * pi / 5.0;
            const QPointF point(12.0 + std::cos(angle) * radius, 12.0 + std::sin(angle) * radius);
            if (i == 0) {
                star.moveTo(point);
            } else {
                star.lineTo(point);
            }
        }
        star.closeSubpath();
        painter.drawPath(star);
        break;
    }
    }
    painter.restore();
}

void paintClockFace(QPainter& painter, const QRectF& rect, const QTime& time, const QColor& rim,
                    const QColor& face, const QColor& ticks, const QColor& hands,
                    bool showSeconds, const QColor& secondHand)
{
    if (rect.isEmpty() || !time.isValid()) {
        return;
    }
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    const qreal diameter = std::min(rect.width(), rect.height());
    const QRectF circle(rect.center().x() - diameter / 2.0, rect.center().y() - diameter / 2.0,
                        diameter, diameter);
    painter.setPen(QPen(rim, std::max<qreal>(1.0, diameter * 0.045)));
    painter.setBrush(face);
    painter.drawEllipse(circle.adjusted(1.0, 1.0, -1.0, -1.0));

    const QPointF center = circle.center();
    const qreal radius = diameter / 2.0;
    painter.setPen(Qt::NoPen);
    painter.setBrush(ticks);
    for (int minute = 0; minute < 60; minute += 5) {
        const qreal angle = (minute / 60.0) * 2.0 * std::numbers::pi - std::numbers::pi / 2.0;
        const bool hourMark = minute % 15 == 0;
        const qreal inner = radius - (hourMark ? 0.28 : 0.20) * radius;
        const qreal outer = radius - 0.10 * radius;
        const QPointF p1(center.x() + std::cos(angle) * inner, center.y() + std::sin(angle) * inner);
        const QPointF p2(center.x() + std::cos(angle) * outer, center.y() + std::sin(angle) * outer);
        painter.setPen(QPen(ticks, std::max<qreal>(0.8, diameter * (hourMark ? 0.026 : 0.014)),
                            Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(p1, p2);
    }

    const int hour = time.hour() % 12;
    const qreal minuteAngle = (time.minute() / 60.0) * 2.0 * std::numbers::pi - std::numbers::pi / 2.0;
    const qreal hourAngle = ((hour + time.minute() / 60.0) / 12.0) * 2.0 * std::numbers::pi
                            - std::numbers::pi / 2.0;

    painter.setPen(QPen(hands, std::max<qreal>(1.4, diameter * 0.065), Qt::SolidLine, Qt::RoundCap));
    QPointF hourTip(center.x() + std::cos(hourAngle) * radius * 0.46,
                    center.y() + std::sin(hourAngle) * radius * 0.46);
    painter.drawLine(center, hourTip);
    painter.setPen(QPen(hands, std::max<qreal>(1.0, diameter * 0.045), Qt::SolidLine, Qt::RoundCap));
    QPointF minuteTip(center.x() + std::cos(minuteAngle) * radius * 0.70,
                      center.y() + std::sin(minuteAngle) * radius * 0.70);
    painter.drawLine(center, minuteTip);

    if (showSeconds) {
        const qreal secondAngle = (time.second() / 60.0) * 2.0 * std::numbers::pi - std::numbers::pi / 2.0;
        const QColor color = secondHand.isValid() ? secondHand : hands;
        painter.setPen(QPen(color, std::max<qreal>(0.8, diameter * 0.022), Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(center, QPointF(center.x() + std::cos(secondAngle) * radius * 0.78,
                                         center.y() + std::sin(secondAngle) * radius * 0.78));
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(hands);
    painter.drawEllipse(center, std::max<qreal>(1.0, diameter * 0.05), std::max<qreal>(1.0, diameter * 0.05));
    painter.restore();
}

QPixmap pixmap(Icon icon, int size, const QColor& color, qreal devicePixelRatio)
{
    if (size <= 0) {
        return {};
    }
    qreal dpr = devicePixelRatio;
    if (dpr <= 0.0) {
        dpr = qApp != nullptr ? qApp->devicePixelRatio() : 1.0;
    }
    const QString key = QStringLiteral("pac-icon-%1-%2-%3-%4")
                            .arg(static_cast<int>(icon))
                            .arg(size)
                            .arg(color.rgba())
                            .arg(dpr, 0, 'f', 2);
    QPixmap cached;
    if (QPixmapCache::find(key, &cached)) {
        return cached;
    }
    QPixmap result(qRound(size * dpr), qRound(size * dpr));
    result.fill(Qt::transparent);
    QPainter painter(&result);
    painter.scale(dpr, dpr);
    paint(painter, QRectF(0, 0, size, size), icon, color);
    painter.end();
    result.setDevicePixelRatio(dpr);
    QPixmapCache::insert(key, result);
    return result;
}

QIcon qicon(Icon icon, const QColor& color)
{
    QIcon result;
    for (const int size : {16, 20, 24, 32}) {
        result.addPixmap(pixmap(icon, size, color));
    }
    return result;
}

} // namespace pac::icons
