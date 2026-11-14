#include "Fonts.h"

#include "Settings.h"

#include <QApplication>
#include <QFontDatabase>
#include <QSet>

namespace pac::fonts {
namespace {

QString g_uiFamily = QStringLiteral("Segoe UI");
QString g_digitFamily = QStringLiteral("Cascadia Mono");
QString g_uiFile;
QString g_digitFile;
QSet<QString> g_loadedFiles;

QString loadFile(const QString& path)
{
    if (path.isEmpty() || g_loadedFiles.contains(path)) {
        return {};
    }
    const int id = QFontDatabase::addApplicationFont(path);
    if (id < 0) {
        return {};
    }
    g_loadedFiles.insert(path);
    const QStringList families = QFontDatabase::applicationFontFamilies(id);
    return families.isEmpty() ? QString() : families.first();
}

[[nodiscard]] QString resolveFamily(const QString& requested, const QString& fallback)
{
    const QStringList available = QFontDatabase::families();
    if (requested.isEmpty()) {
        return fallback;
    }
    for (const QString& family : available) {
        if (family.compare(requested, Qt::CaseInsensitive) == 0) {
            return family;
        }
    }
    return fallback;
}

void applyApplicationFont()
{
    if (auto* app = qobject_cast<QApplication*>(QCoreApplication::instance())) {
        QFont font = app->font();
        font.setFamily(g_uiFamily);
        font.setPointSizeF(9.0);
        app->setFont(font);
    }
}

} // namespace

QString uiFamily()
{
    return g_uiFamily;
}

QString digitFamily()
{
    return g_digitFamily;
}

QFont uiFont(qreal pointSize, QFont::Weight weight)
{
    QFont font(g_uiFamily);
    font.setPointSizeF(pointSize);
    font.setWeight(weight);
    return font;
}

QFont digitFont(qreal pointSize, QFont::Weight weight)
{
    QFont font(g_digitFamily);
    font.setPointSizeF(pointSize);
    font.setWeight(weight);
    return font;
}

QString loadFontFile(const QString& path)
{
    return loadFile(path);
}

void initialize()
{
    QSettings& settings = Settings::instance().raw();
    g_uiFile = settings.value(QStringLiteral("appearance/uiFontFile")).toString();
    g_digitFile = settings.value(QStringLiteral("appearance/digitFontFile")).toString();
    const QString uiFromFile = loadFile(g_uiFile);
    const QString digitFromFile = loadFile(g_digitFile);
    const QString requestedUi = !uiFromFile.isEmpty()
                                    ? uiFromFile
                                    : settings.value(QStringLiteral("appearance/uiFontFamily"),
                                                     QStringLiteral("Segoe UI"))
                                          .toString();
    const QString requestedDigit = !digitFromFile.isEmpty()
                                       ? digitFromFile
                                       : settings.value(QStringLiteral("appearance/digitFontFamily"),
                                                        QStringLiteral("Cascadia Mono"))
                                             .toString();
    g_uiFamily = resolveFamily(requestedUi, QStringLiteral("Segoe UI"));
    g_digitFamily = resolveFamily(requestedDigit, QStringLiteral("Consolas"));
    if (g_digitFamily == QStringLiteral("Segoe UI")) {
        g_digitFamily = QStringLiteral("Consolas");
    }
    applyApplicationFont();
}

void apply()
{
    initialize();
}

} // namespace pac::fonts
