#pragma once

#include <QColor>
#include <QIcon>
#include <QPixmap>
#include <QTime>

class QPainter;
class QRectF;
class QString;

namespace pac::icons {

enum class Icon {
    Alarm,
    AlarmPlus,
    Timer,
    Stopwatch,
    Pomodoro,
    Calendar,
    CalendarPlus,
    Globe,
    Tasks,
    TaskPlus,
    Plus,
    Edit,
    Copy,
    Delete,
    Settings,
    Info,
    Help,
    Theme,
    Search,
    ChevronLeft,
    ChevronRight,
    ChevronDown,
    ChevronUp,
    Play,
    Pause,
    Stop,
    Reset,
    SkipForward,
    Flag,
    Bell,
    Snooze,
    Close,
    Check,
    Volume,
    Mute,
    Folder,
    FolderPlus,
    Save,
    Import,
    Export,
    Moon,
    Sun,
    Menu,
    Grid,
    List,
    Repeat,
    More,
    ArrowUp,
    ArrowDown,
    Sort,
    Navigation,
    StatusBar,
    Density,
    Fullscreen,
    Clock,
    Star,
    Home,
    View,
    Minimize,
    Maximize,
    Restore,
    Pin
};

void paint(QPainter& painter, const QRectF& rect, Icon icon, const QColor& color, qreal stroke = 1.9);
void paintClockFace(QPainter& painter, const QRectF& rect, const QTime& time, const QColor& rim,
                    const QColor& face, const QColor& ticks, const QColor& hands,
                    bool showSeconds = false, const QColor& secondHand = QColor());
[[nodiscard]] QPixmap pixmap(Icon icon, int size, const QColor& color, qreal devicePixelRatio = 0.0);
[[nodiscard]] QIcon qicon(Icon icon, const QColor& color);

} // namespace pac::icons
