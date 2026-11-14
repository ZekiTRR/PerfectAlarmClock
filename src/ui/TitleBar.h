#pragma once

#include "core/Icons.h"

#include <QPoint>
#include <QWidget>

class QLabel;

namespace pac::ui {

class WindowButton;

class TitleBar final : public QWidget {
    Q_OBJECT

public:
    explicit TitleBar(QWidget* parent = nullptr);

    void setClockText(const QString& text);
    void setMaximized(bool maximized);
    void setClockVisible(bool visible);
    void setFluentMode(bool fluent);

signals:
    void minimizeRequested();
    void maximizeRestoreRequested();
    void closeRequested();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    void applyTheme();

    QLabel* m_icon{nullptr};
    QLabel* m_title{nullptr};
    QLabel* m_clock{nullptr};
    QPoint m_pressPosition;
    bool m_maybeDrag{false};
    bool m_fluent{false};
    WindowButton* m_minimize{nullptr};
    WindowButton* m_maximize{nullptr};
    WindowButton* m_close{nullptr};
    bool m_maximized{false};
};

} // namespace pac::ui
