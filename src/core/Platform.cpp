#include "Platform.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>

#ifdef Q_OS_WIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace pac::platform {

void setAutostart(bool enabled)
{
#ifdef Q_OS_WIN
    QSettings registry(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
                       QSettings::NativeFormat);
    if (enabled) {
        const QString path = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
        registry.setValue(QStringLiteral("PerfectAlarmClock"), QStringLiteral("\"%1\"").arg(path));
    } else {
        registry.remove(QStringLiteral("PerfectAlarmClock"));
    }
#else
    Q_UNUSED(enabled)
#endif
}

bool autostartEnabled()
{
#ifdef Q_OS_WIN
    QSettings registry(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
                       QSettings::NativeFormat);
    return registry.contains(QStringLiteral("PerfectAlarmClock"));
#else
    return false;
#endif
}

QString dataDirectory()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    if (path.isEmpty()) {
        path = QDir::homePath() + QStringLiteral("/.local/share");
    }
    path += QStringLiteral("/PerfectAlarmClock");
    QDir().mkpath(path);
    return path;
}

void revealInExplorer(const QString& path)
{
    const QFileInfo info(path);
    if (info.exists()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(info.isDir() ? info.absoluteFilePath()
                                                                   : info.absolutePath()));
    }
}

void flashTaskbar()
{
#ifdef Q_OS_WIN
    FLASHWINFO info{};
    info.cbSize = sizeof(info);
    info.hwnd = GetActiveWindow();
    info.dwFlags = FLASHW_TRAY | FLASHW_TIMERNOFG;
    info.uCount = 3;
    info.dwTimeout = 0;
    FlashWindowEx(&info);
#endif
}

} // namespace pac::platform
