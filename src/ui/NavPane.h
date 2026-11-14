#pragma once

#include "core/Icons.h"

#include <QAbstractButton>
#include <QColor>
#include <QVector>
#include <QWidget>

class QVBoxLayout;

namespace pac::ui {

class NavItem final : public QAbstractButton {
public:
    NavItem(QString id, QString title, icons::Icon icon, QColor accent, QWidget* parent = nullptr);

    [[nodiscard]] QString id() const;
    void setActive(bool active);
    void setBadge(const QString& text);
    void setCompact(bool compact);
    void setFluentMode(bool fluent);

protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    void applyDensity();

    QString m_id;
    icons::Icon m_icon;
    QColor m_accent;
    bool m_active{false};
    bool m_hovered{false};
    bool m_compact{false};
    bool m_fluent{false};
    QString m_badge;
};

class NextAlarmCard final : public QWidget {
    Q_OBJECT

public:
    explicit NextAlarmCard(QWidget* parent = nullptr);

    void setAlarm(const QString& timeLine, const QString& label, const QString& icon, bool active);

signals:
    void clicked();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    void applyDensity();

    QString m_timeLine;
    QString m_label;
    QString m_icon;
    bool m_active{false};
    bool m_hovered{false};
};

class NavPane final : public QWidget {
    Q_OBJECT

public:
    explicit NavPane(QWidget* parent = nullptr);

    void addItem(const QString& id, const QString& title, icons::Icon icon, const QColor& accent);
    void setCurrent(const QString& id);
    [[nodiscard]] QString currentId() const;
    void setBadge(const QString& id, const QString& badge);
    void setNextAlarm(const QString& timeLine, const QString& label, const QString& icon, bool active);
    void setCompact(bool compact);
    void setFluentMode(bool fluent);

signals:
    void currentChanged(const QString& id);
    void nextAlarmClicked();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void applyLayoutMetrics();

    QVBoxLayout* m_rootLayout{nullptr};
    QVBoxLayout* m_itemsLayout{nullptr};
    QVector<NavItem*> m_items;
    NextAlarmCard* m_nextCard{nullptr};
    QString m_currentId;
    bool m_compact{false};
    bool m_fluent{false};
};

} // namespace pac::ui
