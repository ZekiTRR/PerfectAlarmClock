#pragma once

#include "core/Icons.h"

#include <QAbstractButton>
#include <QColor>
#include <QDateTime>
#include <QFrame>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QToolButton>
#include <QWidget>

namespace pac::ui {

class PageWidget : public QWidget {
    Q_OBJECT

public:
    explicit PageWidget(QWidget* parent = nullptr);

    void activate();
    void deactivate();
    virtual void refreshClock(const QDateTime& now);

    [[nodiscard]] bool isActive() const noexcept;
    virtual void applyThemeMode();

protected:
    virtual void onActivated();
    virtual void onDeactivated();
    virtual void themeUpdated();

private:
    bool m_active{false};
};

class ThemedButton final : public QPushButton {
public:
    ThemedButton(const QString& text, icons::Icon icon, QWidget* parent = nullptr, bool accent = false);

    void setGlyph(icons::Icon icon);
    void setGlyphColor(const QColor& color);
    void setIconSizePx(int size);

private:
    void refreshIcon();

    icons::Icon m_glyph;
    bool m_accent{false};
    QColor m_customColor;
    int m_iconSize{16};
};

class SwitchButton final : public QAbstractButton {
    Q_OBJECT

public:
    explicit SwitchButton(QWidget* parent = nullptr);
    [[nodiscard]] QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
};

class ThemedToolButton final : public QToolButton {
public:
    ThemedToolButton(icons::Icon icon, const QString& tooltip, QWidget* parent = nullptr);
    ThemedToolButton(icons::Icon icon, const QString& text, const QString& tooltip,
                     QWidget* parent = nullptr);

    void setGlyph(icons::Icon icon);
    void setGlyphColor(const QColor& color);
    void setIconSizePx(int size);

private:
    void refreshIcon();

    icons::Icon m_glyph;
    QColor m_customColor;
    int m_iconSize{18};
};

[[nodiscard]] QFrame* makeCard(QWidget* parent = nullptr);
[[nodiscard]] QFrame* makeInsetPanel(QWidget* parent = nullptr);
[[nodiscard]] QWidget* makeHorizontalRule(QWidget* parent = nullptr);
[[nodiscard]] QLabel* makeLabel(const QString& text, const QString& objectName, QWidget* parent = nullptr);
[[nodiscard]] QPixmap emojiPixmap(const QString& emoji, int size, qreal devicePixelRatio = 0.0);
[[nodiscard]] QColor tinted(const QColor& color, qreal alpha);
[[nodiscard]] QIcon appIcon();

} // namespace pac::ui
