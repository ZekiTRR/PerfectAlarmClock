#pragma once

#include <QColor>
#include <QObject>
#include <QPalette>
#include <QString>

#include <vector>

namespace pac {

enum class ThemeFlavor {
    Office2007,
    Office2010,
    WindowsClock
};

struct ThemeColors {
    QString id;
    QString name;
    ThemeFlavor flavor{ThemeFlavor::Office2007};
    bool dark{false};

    QColor accent;
    QColor accentDeep;
    QColor window;
    QColor pageTop;
    QColor pageBottom;
    QColor panel;
    QColor card;
    QColor border;
    QColor grid;
    QColor text;
    QColor dim;
    QColor faint;
    QColor selection;
    QColor selectionText;
    QColor altRow;
    QColor hoverRow;

    QColor ribbonTop;
    QColor ribbonBottom;
    QColor ribbonBorder;
    QColor ribbonHighlight;
    QColor tabActiveTop;
    QColor tabActiveBottom;
    QColor tabText;
    QColor tabMuted;
    QColor tabHover;
    QColor groupLine;
    QColor groupText;

    QColor navTop;
    QColor navBottom;
    QColor navBorder;
    QColor navText;
    QColor navDim;
    QColor navSelTop;
    QColor navSelBottom;
    QColor navSelText;
    QColor navHover;

    QColor headerTop;
    QColor headerBottom;
    QColor headerText;
    QColor inputBg;
    QColor inputBorder;
    QColor buttonTop;
    QColor buttonBottom;
    QColor buttonHover;
    QColor buttonDown;
    QColor buttonBorder;
    QColor buttonText;
    QColor menuBg;
    QColor menuBorder;
    QColor statusTop;
    QColor statusBottom;
    QColor statusText;

    QColor danger;
    QColor success;
    QColor warning;
};

class ThemeManager final : public QObject {
    Q_OBJECT

public:
    static ThemeManager& instance();

    ThemeManager(const ThemeManager&) = delete;
    ThemeManager& operator=(const ThemeManager&) = delete;
    ThemeManager(ThemeManager&&) = delete;
    ThemeManager& operator=(ThemeManager&&) = delete;

    [[nodiscard]] const ThemeColors& current() const noexcept;
    [[nodiscard]] const std::vector<ThemeColors>& themes() const noexcept;
    [[nodiscard]] const ThemeColors& theme(const QString& id) const;
    [[nodiscard]] QString styleSheet() const;

    void initialize();
    void setTheme(const QString& id);
    void apply();

signals:
    void themeChanged();

private:
    ThemeManager();
    [[nodiscard]] static std::vector<ThemeColors> buildThemes();
    [[nodiscard]] QPalette buildPalette() const;

    std::vector<ThemeColors> m_themes;
    QString m_currentId;
};

} // namespace pac
