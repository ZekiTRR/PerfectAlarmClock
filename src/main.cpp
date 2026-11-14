#include "core/Fonts.h"
#include "core/Platform.h"
#include "core/Settings.h"
#include "core/Theme.h"
#include "ui/MainWindow.h"

#include <QApplication>
#include <QFont>
#include <QLibraryInfo>
#include <QLocale>
#include <QPixmapCache>
#include <QStyleFactory>
#include <QTranslator>
#include <QLockFile>
#include <QMessageBox>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("PerfectAlarmClock"));
    QApplication::setApplicationDisplayName(QStringLiteral("Perfect Alarm Clock"));
    QApplication::setApplicationVersion(QStringLiteral("1.0.0"));
    QApplication::setOrganizationName(QStringLiteral("PerfectAlarmClock"));
    QApplication::setQuitOnLastWindowClosed(false);
    QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    QLocale::setDefault(QLocale(QLocale::Russian, QLocale::Russia));

    QTranslator qtTranslator;
    if (qtTranslator.load(QStringLiteral("qt_ru"),
                          QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
        QApplication::installTranslator(&qtTranslator);
    }

    pac::fonts::initialize();
    QPixmapCache::setCacheLimit(4 * 1024);

    QLockFile instanceLock(pac::platform::dataDirectory()
                           + QStringLiteral("/.perfectalarmclock.lock"));
    instanceLock.setStaleLockTime(0);
    if (!instanceLock.tryLock(100)) {
        QMessageBox::information(
            nullptr, QStringLiteral("Perfect Alarm Clock"),
            QStringLiteral("Perfect Alarm Clock уже запущен.\n"
                           "Проверьте значок приложения в области уведомлений."));
        return 0;
    }

    pac::ThemeManager::instance().initialize();

    pac::ui::MainWindow window;
    window.show();
    return QApplication::exec();
}
