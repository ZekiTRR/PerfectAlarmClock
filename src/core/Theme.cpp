#include "Theme.h"

#include "Fonts.h"
#include "Settings.h"

#include <QApplication>
#include <QHash>
#include <QStyleFactory>

#include <algorithm>
#include <vector>

namespace pac {
namespace {

[[nodiscard]] QColor shade(const QColor& color, int lightnessDelta)
{
    return color.lighter(100 + lightnessDelta);
}

} // namespace

ThemeManager& ThemeManager::instance()
{
    static ThemeManager manager;
    return manager;
}

ThemeManager::ThemeManager()
    : m_themes(buildThemes())
    , m_currentId(QStringLiteral("office2007-blue"))
{
}

const ThemeColors& ThemeManager::current() const noexcept
{
    return theme(m_currentId);
}

const std::vector<ThemeColors>& ThemeManager::themes() const noexcept
{
    return m_themes;
}

const ThemeColors& ThemeManager::theme(const QString& id) const
{
    for (const auto& entry : m_themes) {
        if (entry.id == id) {
            return entry;
        }
    }
    return m_themes.front();
}

void ThemeManager::initialize()
{
    m_currentId = Settings::instance().themeId();
    bool found = false;
    for (const auto& entry : m_themes) {
        if (entry.id == m_currentId) {
            found = true;
            break;
        }
    }
    if (!found) {
        m_currentId = m_themes.front().id;
    }
    apply();
}

void ThemeManager::setTheme(const QString& id)
{
    if (id == m_currentId) {
        apply();
        return;
    }
    m_currentId = theme(id).id;
    Settings::instance().raw().setValue(QStringLiteral("appearance/theme"), m_currentId);
    apply();
}

void ThemeManager::apply()
{
    if (auto* app = qobject_cast<QApplication*>(QCoreApplication::instance())) {
        app->setPalette(buildPalette());
        app->setStyleSheet(styleSheet());
    }
    emit themeChanged();
}

QPalette ThemeManager::buildPalette() const
{
    const ThemeColors c = current();
    QPalette palette;
    palette.setColor(QPalette::Window, c.window);
    palette.setColor(QPalette::WindowText, c.text);
    palette.setColor(QPalette::Base, c.inputBg);
    palette.setColor(QPalette::AlternateBase, c.altRow);
    palette.setColor(QPalette::Text, c.text);
    palette.setColor(QPalette::Button, c.buttonTop);
    palette.setColor(QPalette::ButtonText, c.buttonText);
    palette.setColor(QPalette::BrightText, Qt::white);
    palette.setColor(QPalette::Highlight, c.accent);
    palette.setColor(QPalette::HighlightedText, c.selectionText);
    palette.setColor(QPalette::ToolTipBase, c.card);
    palette.setColor(QPalette::ToolTipText, c.text);
    palette.setColor(QPalette::Link, c.accent);
    palette.setColor(QPalette::LinkVisited, c.accentDeep);
    palette.setColor(QPalette::PlaceholderText, c.faint);
    palette.setColor(QPalette::Light, c.card);
    palette.setColor(QPalette::Midlight, c.panel);
    palette.setColor(QPalette::Mid, c.border);
    palette.setColor(QPalette::Dark, c.grid);
    palette.setColor(QPalette::Shadow, c.ribbonBorder);
    palette.setColor(QPalette::Disabled, QPalette::Text, c.faint);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, c.faint);
    palette.setColor(QPalette::Disabled, QPalette::WindowText, c.faint);
    return palette;
}

std::vector<ThemeColors> ThemeManager::buildThemes()
{
    ThemeColors blue2007;
    blue2007.id = QStringLiteral("office2007-blue");
    blue2007.name = QStringLiteral("2007 · синяя");
    blue2007.flavor = ThemeFlavor::Office2007;
    blue2007.accent = QColor(QStringLiteral("#2a66a5"));
    blue2007.accentDeep = QColor(QStringLiteral("#1d4c80"));
    blue2007.window = QColor(QStringLiteral("#dce9f7"));
    blue2007.pageTop = QColor(QStringLiteral("#f7fafd"));
    blue2007.pageBottom = QColor(QStringLiteral("#eaf2fa"));
    blue2007.panel = QColor(QStringLiteral("#eef5fc"));
    blue2007.card = QColor(QStringLiteral("#ffffff"));
    blue2007.border = QColor(QStringLiteral("#b9cde2"));
    blue2007.grid = QColor(QStringLiteral("#dce7f2"));
    blue2007.text = QColor(QStringLiteral("#1b2b3a"));
    blue2007.dim = QColor(QStringLiteral("#5b7186"));
    blue2007.faint = QColor(QStringLiteral("#8fa1b3"));
    blue2007.selection = QColor(QStringLiteral("#cfe3f7"));
    blue2007.selectionText = QColor(QStringLiteral("#142b42"));
    blue2007.altRow = QColor(QStringLiteral("#f7fafd"));
    blue2007.hoverRow = QColor(QStringLiteral("#eaf4fe"));
    blue2007.ribbonTop = QColor(QStringLiteral("#e9f3ff"));
    blue2007.ribbonBottom = QColor(QStringLiteral("#bfd8f0"));
    blue2007.ribbonBorder = QColor(QStringLiteral("#7f9dc0"));
    blue2007.ribbonHighlight = QColor(QStringLiteral("#ffffff"));
    blue2007.tabActiveTop = QColor(QStringLiteral("#ffffff"));
    blue2007.tabActiveBottom = QColor(QStringLiteral("#e3f0fd"));
    blue2007.tabText = QColor(QStringLiteral("#173a5e"));
    blue2007.tabMuted = QColor(QStringLiteral("#33597f"));
    blue2007.tabHover = QColor(QStringLiteral("#d5e7fa"));
    blue2007.groupLine = QColor(QStringLiteral("#a9c3dd"));
    blue2007.groupText = QColor(QStringLiteral("#3d6288"));
    blue2007.navTop = QColor(QStringLiteral("#f6faff"));
    blue2007.navBottom = QColor(QStringLiteral("#dfeaf7"));
    blue2007.navBorder = QColor(QStringLiteral("#aec4da"));
    blue2007.navText = QColor(QStringLiteral("#173a5e"));
    blue2007.navDim = QColor(QStringLiteral("#5b7186"));
    blue2007.navSelTop = QColor(QStringLiteral("#ffffff"));
    blue2007.navSelBottom = QColor(QStringLiteral("#eaf5ff"));
    blue2007.navSelText = QColor(QStringLiteral("#0f2c49"));
    blue2007.navHover = QColor(QStringLiteral("#e8f3fe"));
    blue2007.headerTop = QColor(QStringLiteral("#fbfdff"));
    blue2007.headerBottom = QColor(QStringLiteral("#e6f0fa"));
    blue2007.headerText = QColor(QStringLiteral("#3d5e7e"));
    blue2007.inputBg = QColor(QStringLiteral("#ffffff"));
    blue2007.inputBorder = QColor(QStringLiteral("#9fb8d1"));
    blue2007.buttonTop = QColor(QStringLiteral("#fdfefe"));
    blue2007.buttonBottom = QColor(QStringLiteral("#e6f0fa"));
    blue2007.buttonHover = QColor(QStringLiteral("#f3f9ff"));
    blue2007.buttonDown = QColor(QStringLiteral("#d7e8f8"));
    blue2007.buttonBorder = QColor(QStringLiteral("#9fb8d1"));
    blue2007.buttonText = QColor(QStringLiteral("#1b2b3a"));
    blue2007.menuBg = QColor(QStringLiteral("#fbfdff"));
    blue2007.menuBorder = QColor(QStringLiteral("#8eaac7"));
    blue2007.statusTop = QColor(QStringLiteral("#e3eefa"));
    blue2007.statusBottom = QColor(QStringLiteral("#cfe0f1"));
    blue2007.statusText = QColor(QStringLiteral("#37536f"));
    blue2007.danger = QColor(QStringLiteral("#c0392b"));
    blue2007.success = QColor(QStringLiteral("#2e8b57"));
    blue2007.warning = QColor(QStringLiteral("#d68910"));

    ThemeColors silver2007 = blue2007;
    silver2007.id = QStringLiteral("office2007-silver");
    silver2007.name = QStringLiteral("2007 · серебро");
    silver2007.window = QColor(QStringLiteral("#e6e6e6"));
    silver2007.pageTop = QColor(QStringLiteral("#fafafa"));
    silver2007.pageBottom = QColor(QStringLiteral("#f0f0f0"));
    silver2007.panel = QColor(QStringLiteral("#f2f2f2"));
    silver2007.border = QColor(QStringLiteral("#c4c4c4"));
    silver2007.grid = QColor(QStringLiteral("#e2e2e2"));
    silver2007.text = QColor(QStringLiteral("#222222"));
    silver2007.dim = QColor(QStringLiteral("#5f5f5f"));
    silver2007.faint = QColor(QStringLiteral("#8f8f8f"));
    silver2007.selection = QColor(QStringLiteral("#dce8f5"));
    silver2007.selectionText = QColor(QStringLiteral("#1a2b3c"));
    silver2007.altRow = QColor(QStringLiteral("#f8f8f8"));
    silver2007.hoverRow = QColor(QStringLiteral("#eff5fa"));
    silver2007.ribbonTop = QColor(QStringLiteral("#fdfdfd"));
    silver2007.ribbonBottom = QColor(QStringLiteral("#d5d5d5"));
    silver2007.ribbonBorder = QColor(QStringLiteral("#9d9d9d"));
    silver2007.ribbonHighlight = QColor(QStringLiteral("#ffffff"));
    silver2007.tabActiveTop = QColor(QStringLiteral("#ffffff"));
    silver2007.tabActiveBottom = QColor(QStringLiteral("#ededed"));
    silver2007.tabText = QColor(QStringLiteral("#2b2b2b"));
    silver2007.tabMuted = QColor(QStringLiteral("#525252"));
    silver2007.tabHover = QColor(QStringLiteral("#e7e7e7"));
    silver2007.groupLine = QColor(QStringLiteral("#b9b9b9"));
    silver2007.groupText = QColor(QStringLiteral("#565656"));
    silver2007.navTop = QColor(QStringLiteral("#fafafa"));
    silver2007.navBottom = QColor(QStringLiteral("#e6e6e6"));
    silver2007.navBorder = QColor(QStringLiteral("#bdbdbd"));
    silver2007.navText = QColor(QStringLiteral("#2b2b2b"));
    silver2007.navDim = QColor(QStringLiteral("#5f5f5f"));
    silver2007.navSelTop = QColor(QStringLiteral("#ffffff"));
    silver2007.navSelBottom = QColor(QStringLiteral("#eef4fa"));
    silver2007.navSelText = QColor(QStringLiteral("#1c2c3d"));
    silver2007.navHover = QColor(QStringLiteral("#eeeeee"));
    silver2007.headerTop = QColor(QStringLiteral("#fbfbfb"));
    silver2007.headerBottom = QColor(QStringLiteral("#e6e6e6"));
    silver2007.headerText = QColor(QStringLiteral("#4a4a4a"));
    silver2007.inputBg = QColor(QStringLiteral("#ffffff"));
    silver2007.inputBorder = QColor(QStringLiteral("#ababab"));
    silver2007.buttonTop = QColor(QStringLiteral("#fdfdfd"));
    silver2007.buttonBottom = QColor(QStringLiteral("#e8e8e8"));
    silver2007.buttonHover = QColor(QStringLiteral("#f5f8fb"));
    silver2007.buttonDown = QColor(QStringLiteral("#d9e5f1"));
    silver2007.buttonBorder = QColor(QStringLiteral("#ababab"));
    silver2007.buttonText = QColor(QStringLiteral("#222222"));
    silver2007.menuBg = QColor(QStringLiteral("#fbfbfb"));
    silver2007.menuBorder = QColor(QStringLiteral("#9d9d9d"));
    silver2007.statusTop = QColor(QStringLiteral("#f0f0f0"));
    silver2007.statusBottom = QColor(QStringLiteral("#dedede"));
    silver2007.statusText = QColor(QStringLiteral("#3f3f3f"));

    ThemeColors black2007;
    black2007.id = QStringLiteral("office2007-black");
    black2007.name = QStringLiteral("2007 · чёрная");
    black2007.flavor = ThemeFlavor::Office2007;
    black2007.dark = true;
    black2007.accent = QColor(QStringLiteral("#6fb3e8"));
    black2007.accentDeep = QColor(QStringLiteral("#2b6d9e"));
    black2007.window = QColor(QStringLiteral("#232426"));
    black2007.pageTop = QColor(QStringLiteral("#27282b"));
    black2007.pageBottom = QColor(QStringLiteral("#202124"));
    black2007.panel = QColor(QStringLiteral("#2a2b2e"));
    black2007.card = QColor(QStringLiteral("#2e2f32"));
    black2007.border = QColor(QStringLiteral("#44464a"));
    black2007.grid = QColor(QStringLiteral("#3c3e42"));
    black2007.text = QColor(QStringLiteral("#eef1f4"));
    black2007.dim = QColor(QStringLiteral("#b1b8c0"));
    black2007.faint = QColor(QStringLiteral("#818892"));
    black2007.selection = QColor(QStringLiteral("#3d5c78"));
    black2007.selectionText = QColor(QStringLiteral("#ffffff"));
    black2007.altRow = QColor(QStringLiteral("#313236"));
    black2007.hoverRow = QColor(QStringLiteral("#3a444e"));
    black2007.ribbonTop = QColor(QStringLiteral("#3b3d41"));
    black2007.ribbonBottom = QColor(QStringLiteral("#202124"));
    black2007.ribbonBorder = QColor(QStringLiteral("#111214"));
    black2007.ribbonHighlight = QColor(QStringLiteral("#6b6f75"));
    black2007.tabActiveTop = QColor(QStringLiteral("#4b4e53"));
    black2007.tabActiveBottom = QColor(QStringLiteral("#323438"));
    black2007.tabText = QColor(QStringLiteral("#f2f5f7"));
    black2007.tabMuted = QColor(QStringLiteral("#b6bcc4"));
    black2007.tabHover = QColor(QStringLiteral("#35373b"));
    black2007.groupLine = QColor(QStringLiteral("#4d5055"));
    black2007.groupText = QColor(QStringLiteral("#c4ccd5"));
    black2007.navTop = QColor(QStringLiteral("#333438"));
    black2007.navBottom = QColor(QStringLiteral("#242528"));
    black2007.navBorder = QColor(QStringLiteral("#4a4c50"));
    black2007.navText = QColor(QStringLiteral("#eef1f4"));
    black2007.navDim = QColor(QStringLiteral("#a2a9b1"));
    black2007.navSelTop = QColor(QStringLiteral("#4a5d70"));
    black2007.navSelBottom = QColor(QStringLiteral("#3d4d5d"));
    black2007.navSelText = QColor(QStringLiteral("#ffffff"));
    black2007.navHover = QColor(QStringLiteral("#3b3d40"));
    black2007.headerTop = QColor(QStringLiteral("#3d3f43"));
    black2007.headerBottom = QColor(QStringLiteral("#2d2e31"));
    black2007.headerText = QColor(QStringLiteral("#ccd5df"));
    black2007.inputBg = QColor(QStringLiteral("#1d1e20"));
    black2007.inputBorder = QColor(QStringLiteral("#56595e"));
    black2007.buttonTop = QColor(QStringLiteral("#3a3c40"));
    black2007.buttonBottom = QColor(QStringLiteral("#2b2c2f"));
    black2007.buttonHover = QColor(QStringLiteral("#45484d"));
    black2007.buttonDown = QColor(QStringLiteral("#27282b"));
    black2007.buttonBorder = QColor(QStringLiteral("#585b60"));
    black2007.buttonText = QColor(QStringLiteral("#eef1f4"));
    black2007.menuBg = QColor(QStringLiteral("#303134"));
    black2007.menuBorder = QColor(QStringLiteral("#5a5c60"));
    black2007.statusTop = QColor(QStringLiteral("#2a2b2e"));
    black2007.statusBottom = QColor(QStringLiteral("#202124"));
    black2007.statusText = QColor(QStringLiteral("#c4ccd5"));
    black2007.danger = QColor(QStringLiteral("#ff7b72"));
    black2007.success = QColor(QStringLiteral("#5cc47d"));
    black2007.warning = QColor(QStringLiteral("#e6b35c"));

    ThemeColors blue2010 = blue2007;
    blue2010.id = QStringLiteral("office2010-blue");
    blue2010.name = QStringLiteral("2010 · синяя");
    blue2010.flavor = ThemeFlavor::Office2010;
    blue2010.accent = QColor(QStringLiteral("#1f5c99"));
    blue2010.accentDeep = QColor(QStringLiteral("#15436f"));
    blue2010.window = QColor(QStringLiteral("#d5e4f3"));
    blue2010.pageTop = QColor(QStringLiteral("#f8fbfe"));
    blue2010.pageBottom = QColor(QStringLiteral("#edf4fb"));
    blue2010.panel = QColor(QStringLiteral("#e9f2fa"));
    blue2010.ribbonTop = QColor(QStringLiteral("#e2edf8"));
    blue2010.ribbonBottom = QColor(QStringLiteral("#b8d0e8"));
    blue2010.ribbonBorder = QColor(QStringLiteral("#86a3c1"));
    blue2010.tabActiveTop = QColor(QStringLiteral("#ffffff"));
    blue2010.tabActiveBottom = QColor(QStringLiteral("#e9f2fb"));
    blue2010.tabHover = QColor(QStringLiteral("#cadff3"));
    blue2010.groupLine = QColor(QStringLiteral("#9db9d5"));
    blue2010.navTop = QColor(QStringLiteral("#eef5fc"));
    blue2010.navBottom = QColor(QStringLiteral("#d4e3f2"));
    blue2010.navBorder = QColor(QStringLiteral("#a3bcd4"));
    blue2010.navHover = QColor(QStringLiteral("#e2eef9"));
    blue2010.headerTop = QColor(QStringLiteral("#fcfdff"));
    blue2010.headerBottom = QColor(QStringLiteral("#e9f2fb"));
    blue2010.statusTop = QColor(QStringLiteral("#dfeaf6"));
    blue2010.statusBottom = QColor(QStringLiteral("#c8dced"));

    ThemeColors silver2010 = silver2007;
    silver2010.id = QStringLiteral("office2010-silver");
    silver2010.name = QStringLiteral("2010 · серебро");
    silver2010.flavor = ThemeFlavor::Office2010;
    silver2010.window = QColor(QStringLiteral("#dfdfdf"));
    silver2010.pageTop = QColor(QStringLiteral("#fbfbfb"));
    silver2010.pageBottom = QColor(QStringLiteral("#f1f1f1"));
    silver2010.panel = QColor(QStringLiteral("#ececec"));
    silver2010.ribbonTop = QColor(QStringLiteral("#f4f4f4"));
    silver2010.ribbonBottom = QColor(QStringLiteral("#d4d4d4"));
    silver2010.ribbonBorder = QColor(QStringLiteral("#9b9b9b"));
    silver2010.tabActiveTop = QColor(QStringLiteral("#ffffff"));
    silver2010.tabActiveBottom = QColor(QStringLiteral("#efefef"));
    silver2010.tabHover = QColor(QStringLiteral("#e2e2e2"));
    silver2010.navTop = QColor(QStringLiteral("#f6f6f6"));
    silver2010.navBottom = QColor(QStringLiteral("#e3e3e3"));
    silver2010.navBorder = QColor(QStringLiteral("#b4b4b4"));
    silver2010.navHover = QColor(QStringLiteral("#ededed"));
    silver2010.headerTop = QColor(QStringLiteral("#fbfbfb"));
    silver2010.headerBottom = QColor(QStringLiteral("#e8e8e8"));
    silver2010.statusTop = QColor(QStringLiteral("#ededed"));
    silver2010.statusBottom = QColor(QStringLiteral("#dadada"));

    ThemeColors black2010 = black2007;
    black2010.id = QStringLiteral("office2010-black");
    black2010.name = QStringLiteral("2010 · чёрная");
    black2010.flavor = ThemeFlavor::Office2010;
    black2010.accent = QColor(QStringLiteral("#7ab8e8"));
    black2010.accentDeep = QColor(QStringLiteral("#2f6fa3"));
    black2010.window = QColor(QStringLiteral("#1f2022"));
    black2010.pageTop = QColor(QStringLiteral("#232426"));
    black2010.pageBottom = QColor(QStringLiteral("#1a1b1d"));
    black2010.panel = QColor(QStringLiteral("#252629"));
    black2010.card = QColor(QStringLiteral("#28292c"));
    black2010.border = QColor(QStringLiteral("#3d3f43"));
    black2010.grid = QColor(QStringLiteral("#36383b"));
    black2010.altRow = QColor(QStringLiteral("#2c2d30"));
    black2010.hoverRow = QColor(QStringLiteral("#343b42"));
    black2010.ribbonTop = QColor(QStringLiteral("#2c2d2f"));
    black2010.ribbonBottom = QColor(QStringLiteral("#18191b"));
    black2010.ribbonBorder = QColor(QStringLiteral("#0c0d0e"));
    black2010.ribbonHighlight = QColor(QStringLiteral("#55585d"));
    black2010.tabActiveTop = QColor(QStringLiteral("#3a3b3e"));
    black2010.tabActiveBottom = QColor(QStringLiteral("#2b2c2e"));
    black2010.tabHover = QColor(QStringLiteral("#292a2d"));
    black2010.groupLine = QColor(QStringLiteral("#44464a"));
    black2010.navTop = QColor(QStringLiteral("#28292c"));
    black2010.navBottom = QColor(QStringLiteral("#1d1e20"));
    black2010.navBorder = QColor(QStringLiteral("#3d3f43"));
    black2010.navHover = QColor(QStringLiteral("#303134"));
    black2010.headerTop = QColor(QStringLiteral("#333437"));
    black2010.headerBottom = QColor(QStringLiteral("#26272a"));
    black2010.inputBg = QColor(QStringLiteral("#1b1c1e"));
    black2010.buttonTop = QColor(QStringLiteral("#333437"));
    black2010.buttonBottom = QColor(QStringLiteral("#252629"));
    black2010.buttonHover = QColor(QStringLiteral("#3b3d41"));
    black2010.menuBg = QColor(QStringLiteral("#2a2b2e"));

    ThemeColors windowsLight = blue2010;
    windowsLight.id = QStringLiteral("windows-light");
    windowsLight.name = QStringLiteral("Windows 8.1/10 · светлая");
    windowsLight.flavor = ThemeFlavor::Office2010;
    windowsLight.dark = false;
    windowsLight.accent = QColor(QStringLiteral("#0078d7"));
    windowsLight.accentDeep = QColor(QStringLiteral("#005a9e"));
    windowsLight.window = QColor(QStringLiteral("#f2f2f2"));
    windowsLight.pageTop = QColor(QStringLiteral("#fafafa"));
    windowsLight.pageBottom = QColor(QStringLiteral("#f3f3f3"));
    windowsLight.panel = QColor(QStringLiteral("#f7f7f7"));
    windowsLight.card = QColor(QStringLiteral("#ffffff"));
    windowsLight.border = QColor(QStringLiteral("#e1e1e1"));
    windowsLight.grid = QColor(QStringLiteral("#ededed"));
    windowsLight.text = QColor(QStringLiteral("#1b1b1b"));
    windowsLight.dim = QColor(QStringLiteral("#5f5f5f"));
    windowsLight.faint = QColor(QStringLiteral("#969696"));
    windowsLight.selection = QColor(QStringLiteral("#0078d7"));
    windowsLight.selectionText = QColor(QStringLiteral("#ffffff"));
    windowsLight.altRow = QColor(QStringLiteral("#f7f9fb"));
    windowsLight.hoverRow = QColor(QStringLiteral("#eaf3fb"));
    windowsLight.ribbonTop = QColor(QStringLiteral("#ffffff"));
    windowsLight.ribbonBottom = QColor(QStringLiteral("#f2f2f2"));
    windowsLight.ribbonBorder = QColor(QStringLiteral("#e0e0e0"));
    windowsLight.ribbonHighlight = QColor(QStringLiteral("#ffffff"));
    windowsLight.tabActiveTop = QColor(QStringLiteral("#ffffff"));
    windowsLight.tabActiveBottom = QColor(QStringLiteral("#ffffff"));
    windowsLight.tabText = QColor(QStringLiteral("#0078d7"));
    windowsLight.tabMuted = QColor(QStringLiteral("#4a4a4a"));
    windowsLight.tabHover = QColor(QStringLiteral("#eaf3fb"));
    windowsLight.groupLine = QColor(QStringLiteral("#e3e3e3"));
    windowsLight.groupText = QColor(QStringLiteral("#6b6b6b"));
    windowsLight.navTop = QColor(QStringLiteral("#ffffff"));
    windowsLight.navBottom = QColor(QStringLiteral("#f7f7f7"));
    windowsLight.navBorder = QColor(QStringLiteral("#e1e1e1"));
    windowsLight.navText = QColor(QStringLiteral("#1f1f1f"));
    windowsLight.navDim = QColor(QStringLiteral("#666666"));
    windowsLight.navSelTop = QColor(QStringLiteral("#0078d7"));
    windowsLight.navSelBottom = QColor(QStringLiteral("#0078d7"));
    windowsLight.navSelText = QColor(QStringLiteral("#ffffff"));
    windowsLight.navHover = QColor(QStringLiteral("#eaf3fb"));
    windowsLight.headerTop = QColor(QStringLiteral("#ffffff"));
    windowsLight.headerBottom = QColor(QStringLiteral("#f5f5f5"));
    windowsLight.headerText = QColor(QStringLiteral("#5a5a5a"));
    windowsLight.inputBg = QColor(QStringLiteral("#ffffff"));
    windowsLight.inputBorder = QColor(QStringLiteral("#c8c8c8"));
    windowsLight.buttonTop = QColor(QStringLiteral("#ffffff"));
    windowsLight.buttonBottom = QColor(QStringLiteral("#f3f3f3"));
    windowsLight.buttonHover = QColor(QStringLiteral("#eaf3fb"));
    windowsLight.buttonDown = QColor(QStringLiteral("#ddeefa"));
    windowsLight.buttonBorder = QColor(QStringLiteral("#c8c8c8"));
    windowsLight.buttonText = QColor(QStringLiteral("#1b1b1b"));
    windowsLight.menuBg = QColor(QStringLiteral("#ffffff"));
    windowsLight.menuBorder = QColor(QStringLiteral("#c8c8c8"));
    windowsLight.statusTop = QColor(QStringLiteral("#f5f5f5"));
    windowsLight.statusBottom = QColor(QStringLiteral("#e8e8e8"));
    windowsLight.statusText = QColor(QStringLiteral("#333333"));
    windowsLight.danger = QColor(QStringLiteral("#c42b1c"));
    windowsLight.success = QColor(QStringLiteral("#107c10"));
    windowsLight.warning = QColor(QStringLiteral("#9d5d00"));

    ThemeColors windowsDark = black2010;
    windowsDark.id = QStringLiteral("windows-dark");
    windowsDark.name = QStringLiteral("Windows 8.1/10 · тёмная");
    windowsDark.flavor = ThemeFlavor::Office2010;
    windowsDark.dark = true;
    windowsDark.accent = QColor(QStringLiteral("#4cc2ff"));
    windowsDark.accentDeep = QColor(QStringLiteral("#1a7fb5"));
    windowsDark.window = QColor(QStringLiteral("#202020"));
    windowsDark.pageTop = QColor(QStringLiteral("#1f1f1f"));
    windowsDark.pageBottom = QColor(QStringLiteral("#1a1a1a"));
    windowsDark.panel = QColor(QStringLiteral("#272727"));
    windowsDark.card = QColor(QStringLiteral("#2b2b2b"));
    windowsDark.border = QColor(QStringLiteral("#3d3d3d"));
    windowsDark.grid = QColor(QStringLiteral("#353535"));
    windowsDark.text = QColor(QStringLiteral("#f3f3f3"));
    windowsDark.dim = QColor(QStringLiteral("#c0c0c0"));
    windowsDark.faint = QColor(QStringLiteral("#8a8a8a"));
    windowsDark.selection = QColor(QStringLiteral("#0f6cbd"));
    windowsDark.selectionText = QColor(QStringLiteral("#ffffff"));
    windowsDark.altRow = QColor(QStringLiteral("#262626"));
    windowsDark.hoverRow = QColor(QStringLiteral("#333b43"));
    windowsDark.ribbonTop = QColor(QStringLiteral("#2d2d2d"));
    windowsDark.ribbonBottom = QColor(QStringLiteral("#202020"));
    windowsDark.ribbonBorder = QColor(QStringLiteral("#171717"));
    windowsDark.ribbonHighlight = QColor(QStringLiteral("#4cc2ff"));
    windowsDark.tabActiveTop = QColor(QStringLiteral("#3a3a3a"));
    windowsDark.tabActiveBottom = QColor(QStringLiteral("#3a3a3a"));
    windowsDark.tabText = QColor(QStringLiteral("#ffffff"));
    windowsDark.tabMuted = QColor(QStringLiteral("#c5c5c5"));
    windowsDark.tabHover = QColor(QStringLiteral("#353535"));
    windowsDark.groupLine = QColor(QStringLiteral("#444444"));
    windowsDark.groupText = QColor(QStringLiteral("#bdbdbd"));
    windowsDark.navTop = QColor(QStringLiteral("#292929"));
    windowsDark.navBottom = QColor(QStringLiteral("#202020"));
    windowsDark.navBorder = QColor(QStringLiteral("#3c3c3c"));
    windowsDark.navText = QColor(QStringLiteral("#f3f3f3"));
    windowsDark.navDim = QColor(QStringLiteral("#b0b0b0"));
    windowsDark.navSelTop = QColor(QStringLiteral("#0f6cbd"));
    windowsDark.navSelBottom = QColor(QStringLiteral("#0f6cbd"));
    windowsDark.navSelText = QColor(QStringLiteral("#ffffff"));
    windowsDark.navHover = QColor(QStringLiteral("#353535"));
    windowsDark.headerTop = QColor(QStringLiteral("#303030"));
    windowsDark.headerBottom = QColor(QStringLiteral("#262626"));
    windowsDark.headerText = QColor(QStringLiteral("#d0d0d0"));
    windowsDark.inputBg = QColor(QStringLiteral("#1b1b1b"));
    windowsDark.inputBorder = QColor(QStringLiteral("#5a5a5a"));
    windowsDark.buttonTop = QColor(QStringLiteral("#323232"));
    windowsDark.buttonBottom = QColor(QStringLiteral("#272727"));
    windowsDark.buttonHover = QColor(QStringLiteral("#3a3a3a"));
    windowsDark.buttonDown = QColor(QStringLiteral("#1f1f1f"));
    windowsDark.buttonBorder = QColor(QStringLiteral("#5a5a5a"));
    windowsDark.buttonText = QColor(QStringLiteral("#f3f3f3"));
    windowsDark.menuBg = QColor(QStringLiteral("#2c2c2c"));
    windowsDark.menuBorder = QColor(QStringLiteral("#5a5a5a"));
    windowsDark.statusTop = QColor(QStringLiteral("#2a2a2a"));
    windowsDark.statusBottom = QColor(QStringLiteral("#202020"));
    windowsDark.statusText = QColor(QStringLiteral("#d0d0d0"));
    windowsDark.danger = QColor(QStringLiteral("#ff99a4"));
    windowsDark.success = QColor(QStringLiteral("#6ccb5f"));
    windowsDark.warning = QColor(QStringLiteral("#fce100"));

    ThemeColors halloween = black2010;
    halloween.id = QStringLiteral("halloween");
    halloween.name = QStringLiteral("Хэллоуин");
    halloween.flavor = ThemeFlavor::Office2010;
    halloween.dark = true;
    halloween.accent = QColor(QStringLiteral("#ff7a18"));
    halloween.accentDeep = QColor(QStringLiteral("#a63d00"));
    halloween.window = QColor(QStringLiteral("#140f18"));
    halloween.pageTop = QColor(QStringLiteral("#1b121f"));
    halloween.pageBottom = QColor(QStringLiteral("#100b15"));
    halloween.panel = QColor(QStringLiteral("#211827"));
    halloween.card = QColor(QStringLiteral("#2a1e32"));
    halloween.border = QColor(QStringLiteral("#422d4d"));
    halloween.grid = QColor(QStringLiteral("#35253e"));
    halloween.text = QColor(QStringLiteral("#fff4e6"));
    halloween.dim = QColor(QStringLiteral("#d3bfae"));
    halloween.faint = QColor(QStringLiteral("#9a8290"));
    halloween.selection = QColor(QStringLiteral("#ff7a18"));
    halloween.selectionText = QColor(QStringLiteral("#1b1020"));
    halloween.altRow = QColor(QStringLiteral("#271b2f"));
    halloween.hoverRow = QColor(QStringLiteral("#3a2444"));
    halloween.ribbonTop = QColor(QStringLiteral("#321d39"));
    halloween.ribbonBottom = QColor(QStringLiteral("#180e1e"));
    halloween.ribbonBorder = QColor(QStringLiteral("#0b0610"));
    halloween.ribbonHighlight = QColor(QStringLiteral("#ff9c4a"));
    halloween.tabActiveTop = QColor(QStringLiteral("#472548"));
    halloween.tabActiveBottom = QColor(QStringLiteral("#321b39"));
    halloween.tabText = QColor(QStringLiteral("#ffb066"));
    halloween.tabMuted = QColor(QStringLiteral("#d3bfae"));
    halloween.tabHover = QColor(QStringLiteral("#3b2243"));
    halloween.groupLine = QColor(QStringLiteral("#54325b"));
    halloween.groupText = QColor(QStringLiteral("#c8a8b8"));
    halloween.navTop = QColor(QStringLiteral("#281932"));
    halloween.navBottom = QColor(QStringLiteral("#170f1d"));
    halloween.navBorder = QColor(QStringLiteral("#422d4d"));
    halloween.navText = QColor(QStringLiteral("#fff4e6"));
    halloween.navDim = QColor(QStringLiteral("#c3adac"));
    halloween.navSelTop = QColor(QStringLiteral("#ff7a18"));
    halloween.navSelBottom = QColor(QStringLiteral("#d95400"));
    halloween.navSelText = QColor(QStringLiteral("#160d1b"));
    halloween.navHover = QColor(QStringLiteral("#3a2444"));
    halloween.headerTop = QColor(QStringLiteral("#301d37"));
    halloween.headerBottom = QColor(QStringLiteral("#211527"));
    halloween.headerText = QColor(QStringLiteral("#d8b9c4"));
    halloween.inputBg = QColor(QStringLiteral("#150e1a"));
    halloween.inputBorder = QColor(QStringLiteral("#684476"));
    halloween.buttonTop = QColor(QStringLiteral("#382442"));
    halloween.buttonBottom = QColor(QStringLiteral("#27182e"));
    halloween.buttonHover = QColor(QStringLiteral("#48294f"));
    halloween.buttonDown = QColor(QStringLiteral("#1b1121"));
    halloween.buttonBorder = QColor(QStringLiteral("#684476"));
    halloween.buttonText = QColor(QStringLiteral("#fff4e6"));
    halloween.menuBg = QColor(QStringLiteral("#24172b"));
    halloween.menuBorder = QColor(QStringLiteral("#684476"));
    halloween.statusTop = QColor(QStringLiteral("#2a1b31"));
    halloween.statusBottom = QColor(QStringLiteral("#160e1b"));
    halloween.statusText = QColor(QStringLiteral("#d3bfae"));
    halloween.danger = QColor(QStringLiteral("#ff5252"));
    halloween.success = QColor(QStringLiteral("#8ecf6d"));
    halloween.warning = QColor(QStringLiteral("#ffc14d"));

    ThemeColors newYear = windowsLight;
    newYear.id = QStringLiteral("new-year");
    newYear.name = QStringLiteral("Новый год");
    newYear.flavor = ThemeFlavor::Office2010;
    newYear.dark = false;
    newYear.accent = QColor(QStringLiteral("#c62828"));
    newYear.accentDeep = QColor(QStringLiteral("#8e1c17"));
    newYear.window = QColor(QStringLiteral("#edf7fc"));
    newYear.pageTop = QColor(QStringLiteral("#fbfdff"));
    newYear.pageBottom = QColor(QStringLiteral("#eef7fc"));
    newYear.panel = QColor(QStringLiteral("#f4fbfe"));
    newYear.card = QColor(QStringLiteral("#ffffff"));
    newYear.border = QColor(QStringLiteral("#cfe3ee"));
    newYear.grid = QColor(QStringLiteral("#e4eff5"));
    newYear.text = QColor(QStringLiteral("#17313d"));
    newYear.dim = QColor(QStringLiteral("#5b7482"));
    newYear.faint = QColor(QStringLiteral("#8fa8b4"));
    newYear.selection = QColor(QStringLiteral("#d8ecf8"));
    newYear.selectionText = QColor(QStringLiteral("#17313d"));
    newYear.altRow = QColor(QStringLiteral("#f7fbfe"));
    newYear.hoverRow = QColor(QStringLiteral("#e8f4fb"));
    newYear.ribbonTop = QColor(QStringLiteral("#ffffff"));
    newYear.ribbonBottom = QColor(QStringLiteral("#e8f3fa"));
    newYear.ribbonBorder = QColor(QStringLiteral("#b7d0de"));
    newYear.ribbonHighlight = QColor(QStringLiteral("#ffffff"));
    newYear.tabActiveTop = QColor(QStringLiteral("#ffffff"));
    newYear.tabActiveBottom = QColor(QStringLiteral("#ffffff"));
    newYear.tabText = QColor(QStringLiteral("#b3261e"));
    newYear.tabMuted = QColor(QStringLiteral("#516b79"));
    newYear.tabHover = QColor(QStringLiteral("#e8f4fb"));
    newYear.groupLine = QColor(QStringLiteral("#d9e8f0"));
    newYear.groupText = QColor(QStringLiteral("#6b8592"));
    newYear.navTop = QColor(QStringLiteral("#ffffff"));
    newYear.navBottom = QColor(QStringLiteral("#f0f9fd"));
    newYear.navBorder = QColor(QStringLiteral("#c6dce8"));
    newYear.navText = QColor(QStringLiteral("#17313d"));
    newYear.navDim = QColor(QStringLiteral("#6b8490"));
    newYear.navSelTop = QColor(QStringLiteral("#c62828"));
    newYear.navSelBottom = QColor(QStringLiteral("#a91f1f"));
    newYear.navSelText = QColor(QStringLiteral("#ffffff"));
    newYear.navHover = QColor(QStringLiteral("#e8f4fb"));
    newYear.headerTop = QColor(QStringLiteral("#ffffff"));
    newYear.headerBottom = QColor(QStringLiteral("#f4fafd"));
    newYear.headerText = QColor(QStringLiteral("#5b7482"));
    newYear.inputBg = QColor(QStringLiteral("#ffffff"));
    newYear.inputBorder = QColor(QStringLiteral("#a9c4d2"));
    newYear.buttonTop = QColor(QStringLiteral("#ffffff"));
    newYear.buttonBottom = QColor(QStringLiteral("#f4fafd"));
    newYear.buttonHover = QColor(QStringLiteral("#e8f4fb"));
    newYear.buttonDown = QColor(QStringLiteral("#d9ecf7"));
    newYear.buttonBorder = QColor(QStringLiteral("#a9c4d2"));
    newYear.buttonText = QColor(QStringLiteral("#17313d"));
    newYear.menuBg = QColor(QStringLiteral("#ffffff"));
    newYear.menuBorder = QColor(QStringLiteral("#a9c4d2"));
    newYear.statusTop = QColor(QStringLiteral("#eef7fc"));
    newYear.statusBottom = QColor(QStringLiteral("#e2eff6"));
    newYear.statusText = QColor(QStringLiteral("#435f6c"));
    newYear.danger = QColor(QStringLiteral("#b3261e"));
    newYear.success = QColor(QStringLiteral("#2e7d32"));
    newYear.warning = QColor(QStringLiteral("#9a6700"));

    ThemeColors clockLight = windowsLight;
    clockLight.id = QStringLiteral("win11-clock-light");
    clockLight.name = QStringLiteral("Windows Clock · светлая");
    clockLight.flavor = ThemeFlavor::WindowsClock;
    clockLight.dark = false;
    clockLight.accent = QColor(QStringLiteral("#0067c0"));
    clockLight.accentDeep = QColor(QStringLiteral("#005a9e"));
    clockLight.window = QColor(QStringLiteral("#f3f3f3"));
    clockLight.pageTop = QColor(QStringLiteral("#f3f3f3"));
    clockLight.pageBottom = QColor(QStringLiteral("#f3f3f3"));
    clockLight.panel = QColor(QStringLiteral("#ededed"));
    clockLight.card = QColor(QStringLiteral("#fbfbfb"));
    clockLight.border = QColor(QStringLiteral("#e5e5e5"));
    clockLight.grid = QColor(QStringLiteral("#ededed"));
    clockLight.text = QColor(QStringLiteral("#1b1b1b"));
    clockLight.dim = QColor(QStringLiteral("#616161"));
    clockLight.faint = QColor(QStringLiteral("#8b8b8b"));
    clockLight.selection = QColor(QStringLiteral("#0067c0"));
    clockLight.selectionText = QColor(QStringLiteral("#ffffff"));
    clockLight.altRow = QColor(QStringLiteral("#f7f7f7"));
    clockLight.hoverRow = QColor(QStringLiteral("#ececec"));
    clockLight.ribbonTop = QColor(QStringLiteral("#f3f3f3"));
    clockLight.ribbonBottom = QColor(QStringLiteral("#ededed"));
    clockLight.ribbonBorder = QColor(QStringLiteral("#e5e5e5"));
    clockLight.ribbonHighlight = QColor(QStringLiteral("#ffffff"));
    clockLight.tabActiveTop = QColor(QStringLiteral("#fbfbfb"));
    clockLight.tabActiveBottom = QColor(QStringLiteral("#fbfbfb"));
    clockLight.tabText = QColor(QStringLiteral("#0067c0"));
    clockLight.tabMuted = QColor(QStringLiteral("#616161"));
    clockLight.tabHover = QColor(QStringLiteral("#ececec"));
    clockLight.groupLine = QColor(QStringLiteral("#e5e5e5"));
    clockLight.groupText = QColor(QStringLiteral("#616161"));
    clockLight.navTop = QColor(QStringLiteral("#f3f3f3"));
    clockLight.navBottom = QColor(QStringLiteral("#f3f3f3"));
    clockLight.navBorder = QColor(QStringLiteral("#e5e5e5"));
    clockLight.navText = QColor(QStringLiteral("#1b1b1b"));
    clockLight.navDim = QColor(QStringLiteral("#616161"));
    clockLight.navSelTop = QColor(QStringLiteral("#e7f0f8"));
    clockLight.navSelBottom = QColor(QStringLiteral("#e7f0f8"));
    clockLight.navSelText = QColor(QStringLiteral("#0b4c7a"));
    clockLight.navHover = QColor(QStringLiteral("#ebebeb"));
    clockLight.headerTop = QColor(QStringLiteral("#fbfbfb"));
    clockLight.headerBottom = QColor(QStringLiteral("#f5f5f5"));
    clockLight.headerText = QColor(QStringLiteral("#616161"));
    clockLight.inputBg = QColor(QStringLiteral("#fdfdfd"));
    clockLight.inputBorder = QColor(QStringLiteral("#d5d5d5"));
    clockLight.buttonTop = QColor(QStringLiteral("#fbfbfb"));
    clockLight.buttonBottom = QColor(QStringLiteral("#f5f5f5"));
    clockLight.buttonHover = QColor(QStringLiteral("#f0f0f0"));
    clockLight.buttonDown = QColor(QStringLiteral("#e8e8e8"));
    clockLight.buttonBorder = QColor(QStringLiteral("#d5d5d5"));
    clockLight.buttonText = QColor(QStringLiteral("#1b1b1b"));
    clockLight.menuBg = QColor(QStringLiteral("#f9f9f9"));
    clockLight.menuBorder = QColor(QStringLiteral("#d5d5d5"));
    clockLight.statusTop = QColor(QStringLiteral("#f3f3f3"));
    clockLight.statusBottom = QColor(QStringLiteral("#eaeaea"));
    clockLight.statusText = QColor(QStringLiteral("#616161"));
    clockLight.danger = QColor(QStringLiteral("#c42b1c"));
    clockLight.success = QColor(QStringLiteral("#0f7b0f"));
    clockLight.warning = QColor(QStringLiteral("#9d5d00"));

    ThemeColors clockDark = windowsDark;
    clockDark.id = QStringLiteral("win11-clock-dark");
    clockDark.name = QStringLiteral("Windows Clock · тёмная");
    clockDark.flavor = ThemeFlavor::WindowsClock;
    clockDark.dark = true;
    clockDark.accent = QColor(QStringLiteral("#60cdff"));
    clockDark.accentDeep = QColor(QStringLiteral("#1a7fb5"));
    clockDark.window = QColor(QStringLiteral("#202020"));
    clockDark.pageTop = QColor(QStringLiteral("#202020"));
    clockDark.pageBottom = QColor(QStringLiteral("#202020"));
    clockDark.panel = QColor(QStringLiteral("#2b2b2b"));
    clockDark.card = QColor(QStringLiteral("#272727"));
    clockDark.border = QColor(QStringLiteral("#383838"));
    clockDark.grid = QColor(QStringLiteral("#333333"));
    clockDark.text = QColor(QStringLiteral("#ffffff"));
    clockDark.dim = QColor(QStringLiteral("#c5c5c5"));
    clockDark.faint = QColor(QStringLiteral("#8a8a8a"));
    clockDark.selection = QColor(QStringLiteral("#60cdff"));
    clockDark.selectionText = QColor(QStringLiteral("#000000"));
    clockDark.altRow = QColor(QStringLiteral("#252525"));
    clockDark.hoverRow = QColor(QStringLiteral("#2f2f2f"));
    clockDark.ribbonTop = QColor(QStringLiteral("#272727"));
    clockDark.ribbonBottom = QColor(QStringLiteral("#202020"));
    clockDark.ribbonBorder = QColor(QStringLiteral("#383838"));
    clockDark.ribbonHighlight = QColor(QStringLiteral("#60cdff"));
    clockDark.tabActiveTop = QColor(QStringLiteral("#2b2b2b"));
    clockDark.tabActiveBottom = QColor(QStringLiteral("#2b2b2b"));
    clockDark.tabText = QColor(QStringLiteral("#ffffff"));
    clockDark.tabMuted = QColor(QStringLiteral("#c5c5c5"));
    clockDark.tabHover = QColor(QStringLiteral("#2f2f2f"));
    clockDark.groupLine = QColor(QStringLiteral("#383838"));
    clockDark.groupText = QColor(QStringLiteral("#c5c5c5"));
    clockDark.navTop = QColor(QStringLiteral("#202020"));
    clockDark.navBottom = QColor(QStringLiteral("#202020"));
    clockDark.navBorder = QColor(QStringLiteral("#383838"));
    clockDark.navText = QColor(QStringLiteral("#ffffff"));
    clockDark.navDim = QColor(QStringLiteral("#c5c5c5"));
    clockDark.navSelTop = QColor(QStringLiteral("#2d3a44"));
    clockDark.navSelBottom = QColor(QStringLiteral("#2d3a44"));
    clockDark.navSelText = QColor(QStringLiteral("#60cdff"));
    clockDark.navHover = QColor(QStringLiteral("#2a2a2a"));
    clockDark.headerTop = QColor(QStringLiteral("#2b2b2b"));
    clockDark.headerBottom = QColor(QStringLiteral("#252525"));
    clockDark.headerText = QColor(QStringLiteral("#c5c5c5"));
    clockDark.inputBg = QColor(QStringLiteral("#272727"));
    clockDark.inputBorder = QColor(QStringLiteral("#3d3d3d"));
    clockDark.buttonTop = QColor(QStringLiteral("#2f2f2f"));
    clockDark.buttonBottom = QColor(QStringLiteral("#2a2a2a"));
    clockDark.buttonHover = QColor(QStringLiteral("#353535"));
    clockDark.buttonDown = QColor(QStringLiteral("#252525"));
    clockDark.buttonBorder = QColor(QStringLiteral("#3d3d3d"));
    clockDark.buttonText = QColor(QStringLiteral("#ffffff"));
    clockDark.menuBg = QColor(QStringLiteral("#2c2c2c"));
    clockDark.menuBorder = QColor(QStringLiteral("#454545"));
    clockDark.statusTop = QColor(QStringLiteral("#202020"));
    clockDark.statusBottom = QColor(QStringLiteral("#202020"));
    clockDark.statusText = QColor(QStringLiteral("#c5c5c5"));
    clockDark.danger = QColor(QStringLiteral("#ff99a4"));
    clockDark.success = QColor(QStringLiteral("#6ccb5f"));
    clockDark.warning = QColor(QStringLiteral("#fce100"));

    return {blue2007, silver2007, black2007, blue2010, silver2010, black2010,
            windowsLight, windowsDark, halloween, newYear, clockLight, clockDark};
}

QString ThemeManager::styleSheet() const
{
    const ThemeColors c = current();
    QHash<QString, QString> tokens;
    tokens.insert(QStringLiteral("@uiFont"), fonts::uiFamily());
    tokens.insert(QStringLiteral("@digitFont"), fonts::digitFamily());
    tokens.insert(QStringLiteral("@window"), c.window.name());
    tokens.insert(QStringLiteral("@pageTop"), c.pageTop.name());
    tokens.insert(QStringLiteral("@pageBottom"), c.pageBottom.name());
    tokens.insert(QStringLiteral("@panel"), c.panel.name());
    tokens.insert(QStringLiteral("@card"), c.card.name());
    tokens.insert(QStringLiteral("@border"), c.border.name());
    tokens.insert(QStringLiteral("@grid"), c.grid.name());
    tokens.insert(QStringLiteral("@text"), c.text.name());
    tokens.insert(QStringLiteral("@dim"), c.dim.name());
    tokens.insert(QStringLiteral("@faint"), c.faint.name());
    tokens.insert(QStringLiteral("@selection"), c.selection.name());
    tokens.insert(QStringLiteral("@selectionText"), c.selectionText.name());
    tokens.insert(QStringLiteral("@altRow"), c.altRow.name());
    tokens.insert(QStringLiteral("@hoverRow"), c.hoverRow.name());
    tokens.insert(QStringLiteral("@headerTop"), c.headerTop.name());
    tokens.insert(QStringLiteral("@headerBottom"), c.headerBottom.name());
    tokens.insert(QStringLiteral("@headerText"), c.headerText.name());
    tokens.insert(QStringLiteral("@inputBg"), c.inputBg.name());
    tokens.insert(QStringLiteral("@inputBorder"), c.inputBorder.name());
    tokens.insert(QStringLiteral("@buttonTop"), c.buttonTop.name());
    tokens.insert(QStringLiteral("@buttonBottom"), c.buttonBottom.name());
    tokens.insert(QStringLiteral("@buttonHover"), c.buttonHover.name());
    tokens.insert(QStringLiteral("@buttonDown"), c.buttonDown.name());
    tokens.insert(QStringLiteral("@buttonBorder"), c.buttonBorder.name());
    tokens.insert(QStringLiteral("@buttonText"), c.buttonText.name());
    tokens.insert(QStringLiteral("@menuBg"), c.menuBg.name());
    tokens.insert(QStringLiteral("@menuBorder"), c.menuBorder.name());
    tokens.insert(QStringLiteral("@statusTop"), c.statusTop.name());
    tokens.insert(QStringLiteral("@statusBottom"), c.statusBottom.name());
    tokens.insert(QStringLiteral("@statusText"), c.statusText.name());
    tokens.insert(QStringLiteral("@accent"), c.accent.name());
    tokens.insert(QStringLiteral("@accentDeep"), c.accentDeep.name());
    tokens.insert(QStringLiteral("@accentHover"), shade(c.accent, 12).name());
    tokens.insert(QStringLiteral("@accentSoft"), shade(c.accent, 65).name());
    tokens.insert(QStringLiteral("@accentText"),
                  c.accent.lightnessF() > 0.58 ? QStringLiteral("#000000")
                                                : QStringLiteral("#ffffff"));
    tokens.insert(QStringLiteral("@danger"), c.danger.name());
    tokens.insert(QStringLiteral("@success"), c.success.name());
    tokens.insert(QStringLiteral("@warning"), c.warning.name());
    tokens.insert(QStringLiteral("@scrollTrack"), c.panel.name());
    tokens.insert(QStringLiteral("@scrollHandle"), c.border.name());
    tokens.insert(QStringLiteral("@scrollHover"), c.dim.name());
    tokens.insert(QStringLiteral("@arrowUp"),
                  c.dark ? QStringLiteral(":/icons/arrow_up_dark.png")
                         : QStringLiteral(":/icons/arrow_up_light.png"));
    tokens.insert(QStringLiteral("@arrowDown"),
                  c.dark ? QStringLiteral(":/icons/arrow_down_dark.png")
                         : QStringLiteral(":/icons/arrow_down_light.png"));

    QString css = QStringLiteral(R"CSS(
* { outline: none; }
QWidget { color: @text; font-family: "@uiFont"; }
QMainWindow, QDialog { background: @window; }
QWidget#PageHost { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 @pageTop, stop:1 @pageBottom); }
QWidget#Page, QWidget#Transparent, QScrollArea, QScrollArea > QWidget > QWidget { background: transparent; border: none; }
QFrame#Card { background: @card; border: 1px solid @border; border-radius: 6px; }
QFrame#InsetPanel { background: @panel; border: 1px solid @border; border-radius: 5px; }
QFrame#ToolsHero { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 @card, stop:0.62 @panel, stop:1 @card); border: 1px solid @border; border-radius: 8px; }
QFrame#ToolsSidebar { background: @panel; border: 1px solid @border; border-radius: 8px; }
QFrame#ToolCard { background: @card; border: 1px solid @border; border-radius: 8px; }
QFrame#ToolZone { background: @panel; border: 1px solid @border; border-radius: 6px; }
QFrame#ToolResult { background: @panel; border: 1px solid @border; border-left: 3px solid @accent; border-radius: 6px; }
QLabel#ToolIconChip { background: @panel; border: 1px solid @border; border-radius: 10px; }
QListWidget#ToolsNav { background: transparent; border: none; padding: 2px; }
QListWidget#ToolsNav::item { padding: 7px 10px; margin: 1px 0; border-radius: 5px; color: @text; }
QListWidget#ToolsNav::item:hover { background: @hoverRow; }
QListWidget#ToolsNav::item:selected { background: @hoverRow; color: @text; border-left: 3px solid @accent; }
QListWidget#ToolsNav::item:disabled { background: transparent; color: @faint; padding: 12px 8px 4px 10px; }
QLabel { background: transparent; }
QLabel#PageTitle { font-size: 17pt; font-weight: 600; }
QLabel#PageSubtitle { color: @dim; font-size: 9pt; }
QLabel#CardTitle { font-size: 11pt; font-weight: 600; }
QLabel#SectionLabel { font-size: 8pt; font-weight: 600; color: @dim; }
QLabel#Muted { color: @dim; }
QLabel#Faint { color: @faint; }
QLabel#HeroTime { font-size: 30pt; font-weight: 300; font-family: "@digitFont"; }
QLabel#Mono { font-family: "@digitFont"; }
QLabel#Danger { color: @danger; }
QLabel#Success { color: @success; }
QToolTip { background: @menuBg; color: @text; border: 1px solid @menuBorder; padding: 5px 8px; }
QPushButton { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 @buttonTop, stop:1 @buttonBottom); border: 1px solid @buttonBorder; border-radius: 3px; color: @buttonText; padding: 4px 14px; min-height: 20px; }
QPushButton:hover { background: @buttonHover; border-color: @accent; }
QPushButton:pressed { background: @buttonDown; }
QPushButton:checked { background: @selection; border-color: @accent; color: @selectionText; }
QPushButton:disabled { color: @faint; background: @panel; border-color: @border; }
QPushButton[accent="true"] { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 @accentHover, stop:1 @accentDeep); color: #ffffff; border-color: @accentDeep; font-weight: 600; }
QPushButton[accent="true"]:hover { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 @accent, stop:1 @accent); }
QPushButton[accent="true"]:pressed { background: @accentDeep; }
QPushButton[flat="true"] { background: transparent; border-color: transparent; }
QPushButton[flat="true"]:hover { background: @buttonHover; border-color: @border; }
QPushButton[danger="true"] { color: @danger; }
QToolButton { background: transparent; border: 1px solid transparent; border-radius: 3px; padding: 3px; color: @text; }
QToolButton:hover { background: @buttonHover; border-color: @border; }
QToolButton:pressed { background: @buttonDown; }
QToolButton:disabled { color: @faint; }
QLineEdit, QTextEdit, QPlainTextEdit, QAbstractSpinBox, QComboBox {
    background: @inputBg; border: 1px solid @inputBorder; border-radius: 3px;
    color: @text; padding: 3px 6px; min-height: 20px;
    selection-background-color: @accent; selection-color: #ffffff;
}
QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus, QAbstractSpinBox:focus, QComboBox:focus { border-color: @accent; }
QLineEdit:disabled, QTextEdit:disabled, QPlainTextEdit:disabled, QAbstractSpinBox:disabled, QComboBox:disabled { background: @panel; color: @faint; border-color: @border; }
QLineEdit[search="true"] { padding-left: 24px; border-radius: 4px; }
QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: center right; width: 20px; border-left: 1px solid @border; }
QComboBox::down-arrow { image: url(@arrowDown); width: 9px; height: 6px; }
QAbstractSpinBox::up-button { subcontrol-origin: border; subcontrol-position: top right; width: 18px; background: @panel; border-left: 1px solid @border; border-top-right-radius: 3px; }
QAbstractSpinBox::down-button { subcontrol-origin: border; subcontrol-position: bottom right; width: 18px; background: @panel; border-left: 1px solid @border; border-bottom-right-radius: 3px; }
QAbstractSpinBox::up-button:hover, QAbstractSpinBox::down-button:hover { background: @buttonHover; }
QAbstractSpinBox::up-arrow { image: url(@arrowUp); width: 9px; height: 6px; }
QAbstractSpinBox::down-arrow { image: url(@arrowDown); width: 9px; height: 6px; }
QComboBox QAbstractItemView { background: @card; border: 1px solid @border; selection-background-color: @selection; selection-color: @selectionText; padding: 2px; outline: 0; }
QComboBox QAbstractItemView::item { min-height: 22px; padding: 2px 8px; }
QTimeEdit, QDateEdit { font-family: "@digitFont"; }
QTimeEdit#HeroTime { font-size: 26pt; font-weight: 300; padding: 2px 8px; min-height: 48px; background: @inputBg; border: 1px solid @inputBorder; border-radius: 4px; }
QTableView { background: @card; alternate-background-color: @altRow; border: 1px solid @border; border-radius: 4px; gridline-color: @grid; selection-background-color: @selection; selection-color: @selectionText; }
QTableView::item { padding: 0px; }
QHeaderView { background: transparent; }
QHeaderView::section { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 @headerTop, stop:1 @headerBottom); color: @headerText; border: none; border-right: 1px solid @border; border-bottom: 1px solid @border; padding: 5px 8px; font-size: 8pt; font-weight: 600; }
QTableCornerButton::section { background: @headerTop; border: none; }
QScrollBar:vertical { background: @scrollTrack; width: 12px; margin: 0; border: none; }
QScrollBar::handle:vertical { background: @scrollHandle; min-height: 28px; border-radius: 5px; margin: 2px; }
QScrollBar::handle:vertical:hover { background: @scrollHover; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }
QScrollBar:horizontal { background: @scrollTrack; height: 12px; border: none; }
QScrollBar::handle:horizontal { background: @scrollHandle; min-width: 28px; border-radius: 5px; margin: 2px; }
QScrollBar::handle:horizontal:hover { background: @scrollHover; }
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }
QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: transparent; }
QMenu { background: @menuBg; border: 1px solid @menuBorder; padding: 4px; }
QMenu::item { padding: 6px 28px 6px 26px; border-radius: 2px; }
QMenu::item:selected { background: @selection; color: @selectionText; }
QMenu::item:disabled { color: @faint; }
QMenu::separator { height: 1px; background: @border; margin: 4px 8px; }
QMenu::icon { padding-left: 6px; }
QStatusBar { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 @statusTop, stop:1 @statusBottom); border-top: 1px solid @border; color: @statusText; }
QStatusBar::item { border: none; }
QStatusBar QLabel { color: @statusText; background: transparent; }
QGroupBox { background: @card; border: 1px solid @border; border-radius: 5px; margin-top: 15px; padding: 12px 12px 10px 12px; font-weight: 600; }
QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 10px; padding: 0 5px; color: @text; background: transparent; }
QTabWidget::pane { border: 1px solid @border; background: @card; top: -1px; }
QTabBar::tab { background: @panel; color: @dim; border: 1px solid @border; padding: 5px 14px; margin-right: 2px; border-top-left-radius: 3px; border-top-right-radius: 3px; }
QTabBar::tab:selected { background: @card; color: @text; border-bottom-color: @card; }
QTabBar::tab:hover:!selected { background: @hoverRow; }
QListWidget, QListView { background: @card; border: 1px solid @border; outline: 0; }
QListWidget::item, QListView::item { padding: 5px 8px; }
QListWidget::item:hover, QListView::item:hover { background: @hoverRow; }
QListWidget::item:selected, QListView::item:selected { background: @selection; color: @selectionText; }
QListWidget#SettingsNav { background: @panel; border: 1px solid @border; border-radius: 4px; padding: 5px; }
QListWidget#SettingsNav::item { padding: 8px 10px; margin: 1px 0; border-radius: 3px; }
QListWidget#SettingsNav::item:selected { background: @selection; color: @selectionText; }
QSlider::groove:horizontal { height: 4px; background: @border; border-radius: 2px; }
QSlider::sub-page:horizontal { background: @accent; border-radius: 2px; }
QSlider::add-page:horizontal { background: @border; border-radius: 2px; }
QSlider::handle:horizontal { background: qradialgradient(cx:0.4, cy:0.35, radius:0.8, fx:0.4, fy:0.35, stop:0 #ffffff, stop:1 @buttonBottom); border: 1px solid @accentDeep; width: 14px; margin: -6px 0; border-radius: 7px; }
QSlider::handle:horizontal:hover { border-color: @accent; }
QCheckBox { spacing: 7px; background: transparent; }
QCheckBox::indicator { width: 15px; height: 15px; border: 1px solid @inputBorder; border-radius: 3px; background: @inputBg; }
QCheckBox::indicator:hover { border-color: @accent; }
QCheckBox::indicator:checked { background: @accent; border-color: @accentDeep; image: url(:/icons/check_white.png); }
QCheckBox::indicator:checked:hover { background: @accentHover; }
QCheckBox::indicator:disabled { background: @panel; border-color: @border; }
QCheckBox::indicator:checked:disabled { background: @border; border-color: @border; image: url(:/icons/check_white.png); }
QRadioButton { spacing: 7px; background: transparent; }
QRadioButton::indicator { width: 15px; height: 15px; border: 1px solid @inputBorder; border-radius: 8px; background: @inputBg; }
QRadioButton::indicator:hover { border-color: @accent; }
QRadioButton::indicator:checked { border: 4px solid @accent; background: @inputBg; }
QProgressBar { background: @panel; border: 1px solid @border; border-radius: 3px; text-align: center; min-height: 8px; color: @text; }
QProgressBar::chunk { background: @accent; border-radius: 2px; }
QDialogButtonBox QPushButton { min-width: 82px; }
QSplitter::handle { background: transparent; }
QSizeGrip { background: transparent; }
QMessageBox { background: @window; }
QCalendarWidget QWidget { background: @card; color: @text; }
)CSS");

    if (c.flavor == ThemeFlavor::WindowsClock) {
        css += QStringLiteral(R"CSS(
QWidget#PageHost { background: @window; }
QMainWindow, QDialog { background: @window; }
QFrame#FluentCard { background: @card; border: 1px solid @border; border-radius: 12px; }
QLabel#FluentTime { font-family: "@digitFont"; font-size: 29pt; font-weight: 600; color: @text; }
QLabel#FluentLabel { color: @dim; font-size: 10pt; }
QLabel#FluentMeta { color: @dim; font-size: 9pt; }
QLabel[fluentChip="true"] { background: @panel; color: @dim; border-radius: 14px; padding: 0; font-size: 9pt; }
QLabel[fluentChip="true"][selected="true"] { background: @accent; color: @accentText; }
QPushButton { background: @buttonTop; border: 1px solid @buttonBorder; border-radius: 6px; color: @buttonText; padding: 5px 14px; min-height: 22px; }
QPushButton:hover { background: @buttonHover; border-color: @accent; }
QPushButton:pressed { background: @buttonDown; }
QPushButton:checked { background: @selection; border-color: @accent; color: @selectionText; }
QPushButton[accent="true"] { background: @accent; border-color: @accentDeep; color: @accentText; font-weight: 600; }
QPushButton[accent="true"]:hover { background: @accentHover; }
QPushButton[accent="true"]:pressed { background: @accentDeep; }
QToolButton { border-radius: 6px; padding: 4px; }
QLineEdit, QComboBox, QAbstractSpinBox { border-radius: 6px; padding: 5px 9px; min-height: 22px; }
QTableView, QListWidget, QListView { border-radius: 8px; }
QMenu { background: @menuBg; border: 1px solid @menuBorder; border-radius: 8px; padding: 5px; }
QMenu::item { padding: 7px 28px 7px 24px; border-radius: 5px; }
QGroupBox { border-radius: 8px; }
QScrollBar:vertical { width: 12px; background: transparent; }
QScrollBar:horizontal { height: 12px; background: transparent; }
QScrollBar::handle:vertical { background: @scrollHandle; border-radius: 4px; margin: 3px; min-height: 32px; }
QScrollBar::handle:horizontal { background: @scrollHandle; border-radius: 4px; margin: 3px; min-width: 32px; }
)CSS");
    }

    QStringList keys = tokens.keys();
    std::sort(keys.begin(), keys.end(), [](const QString& left, const QString& right) {
        return left.size() > right.size();
    });
    for (const auto& key : keys) {
        css.replace(key, tokens.value(key));
    }
    return css;
}

} // namespace pac
