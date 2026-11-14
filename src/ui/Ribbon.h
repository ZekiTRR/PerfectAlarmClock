#pragma once

#include "UiKit.h"

#include <QAbstractButton>
#include <QColor>
#include <QVector>
#include <QWidget>

class QEvent;
class QHBoxLayout;
class QResizeEvent;
class QScrollArea;
class QStackedWidget;
class QToolButton;
class QVBoxLayout;

namespace pac::ui {

class RibbonButton final : public QAbstractButton {
public:
    enum class Size {
        Large,
        Small
    };

    RibbonButton(Size size, icons::Icon icon, QString text, QString tooltip, QColor accent,
                 QWidget* parent = nullptr);

    [[nodiscard]] QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;

private:
    Size m_size;
    icons::Icon m_icon;
    QColor m_accent;
    bool m_hovered{false};
};

class RibbonGroup final : public QWidget {
public:
    explicit RibbonGroup(const QString& title, QWidget* parent = nullptr);

    RibbonButton* addLarge(icons::Icon icon, const QString& text, const QString& tooltip,
                           const QColor& accent);
    RibbonButton* addSmall(icons::Icon icon, const QString& text, const QString& tooltip,
                           const QColor& accent);
    void addWidget(QWidget* widget);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void closeSmallColumn();

    QString m_title;
    QHBoxLayout* m_row;
    QVBoxLayout* m_smallColumn{nullptr};
    int m_smallCount{0};
};

class RibbonPage final : public QWidget {
public:
    explicit RibbonPage(QWidget* parent = nullptr);

    RibbonGroup* addGroup(const QString& title);
    void addSpacer();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QHBoxLayout* m_layout;
};

class RibbonTabBar final : public QWidget {
    Q_OBJECT

public:
    explicit RibbonTabBar(QWidget* parent = nullptr);

    void addTab(const QString& id, const QString& title, icons::Icon icon);
    void setCurrentId(const QString& id);
    [[nodiscard]] QString currentId() const;

signals:
    void currentChanged(const QString& id);
    void fileClicked(const QPoint& globalPosition);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    struct Tab {
        QString id;
        QString title;
        icons::Icon icon;
        int width{0};
    };

    void updateWidths();
    [[nodiscard]] int tabAt(int x) const;

    QVector<Tab> m_tabs;
    QString m_currentId;
    int m_hoverIndex{-1};
    bool m_fileHovered{false};
};

class RibbonBar final : public QWidget {
    Q_OBJECT

public:
    explicit RibbonBar(QWidget* parent = nullptr);

    void addPage(const QString& id, const QString& title, icons::Icon icon);
    RibbonGroup* addGroup(const QString& pageId, const QString& title);
    void addSpacer(const QString& pageId);
    void setCurrentPage(const QString& id);
    [[nodiscard]] QString currentPage() const;
    void setRibbonCollapsed(bool collapsed);
    void updateContentWidths();

signals:
    void newAlarmRequested();
    void settingsRequested();
    void helpRequested();
    void fileMenuRequested(const QPoint& globalPosition);
    void pageChanged(const QString& id);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void updateContentVisibility();
    void updateOverflow();
    void refreshOverflowIcons();
    void relayoutScrollContent();

    RibbonTabBar* m_tabs;
    QScrollArea* m_scroll;
    QStackedWidget* m_stack;
    QToolButton* m_scrollLeft{nullptr};
    QToolButton* m_scrollRight{nullptr};
    QVector<QString> m_pageIds;
    int m_contentMinWidth{0};
    bool m_collapsed{false};
};

} // namespace pac::ui
