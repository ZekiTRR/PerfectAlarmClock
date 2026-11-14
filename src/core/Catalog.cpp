#include "Catalog.h"

#include "Platform.h"

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QSet>
#include <QStringList>

#include <algorithm>

namespace pac::catalog {

const std::vector<AlarmIconInfo>& alarmIcons()
{
    static const std::vector<AlarmIconInfo> items{
        {QStringLiteral("⏰"), QStringLiteral("Будильник")},
        {QStringLiteral("🔔"), QStringLiteral("Колокольчик")},
        {QStringLiteral("🌅"), QStringLiteral("Рассвет")},
        {QStringLiteral("☀️"), QStringLiteral("Солнце")},
        {QStringLiteral("☕"), QStringLiteral("Кофе")},
        {QStringLiteral("🍅"), QStringLiteral("Помидор")},
        {QStringLiteral("🌱"), QStringLiteral("Растения")},
        {QStringLiteral("🚗"), QStringLiteral("Машина")},
        {QStringLiteral("✈️"), QStringLiteral("Самолёт")},
        {QStringLiteral("💊"), QStringLiteral("Лекарство")},
        {QStringLiteral("💼"), QStringLiteral("Работа")},
        {QStringLiteral("📚"), QStringLiteral("Учёба")},
        {QStringLiteral("🏃"), QStringLiteral("Тренировка")},
        {QStringLiteral("🏠"), QStringLiteral("Дом")},
        {QStringLiteral("❤️"), QStringLiteral("Важное")},
        {QStringLiteral("⭐"), QStringLiteral("Звезда")},
        {QStringLiteral("🌙"), QStringLiteral("Ночь")},
        {QStringLiteral("🍽️"), QStringLiteral("Еда")},
    };
    return items;
}

const std::vector<QString>& alarmColors()
{
    static const std::vector<QString> colors{
        QStringLiteral("#e2842b"), QStringLiteral("#d4534b"), QStringLiteral("#c9a227"),
        QStringLiteral("#4c9a62"), QStringLiteral("#2e8b8b"), QStringLiteral("#3f7fd6"),
        QStringLiteral("#6a63b5"), QStringLiteral("#a8588f"), QStringLiteral("#6c7a89"),
    };
    return colors;
}

const std::vector<BuiltinSound>& builtinSounds()
{
    static const std::vector<BuiltinSound> sounds{
        {QStringLiteral("builtin:chime"), QStringLiteral("Мягкие колокольчики"), QStringLiteral(":/sounds/chime.wav")},
        {QStringLiteral("builtin:gentle"), QStringLiteral("Рассвет"), QStringLiteral(":/sounds/gentle.wav")},
        {QStringLiteral("builtin:classic"), QStringLiteral("Классический будильник"), QStringLiteral(":/sounds/classic.wav")},
        {QStringLiteral("builtin:digital"), QStringLiteral("Цифровой сигнал"), QStringLiteral(":/sounds/digital.wav")},
    };
    return sounds;
}

std::vector<BuiltinSound> availableSounds()
{
    std::vector<BuiltinSound> result = builtinSounds();
    const std::size_t builtinCount = result.size();
    QSet<QString> builtinBaseNames;
    QSet<QString> usedNames;
    for (const auto& sound : result) {
        builtinBaseNames.insert(QFileInfo(sound.resource).completeBaseName().toLower());
        usedNames.insert(sound.name.toLower());
    }

    QStringList roots{
        QCoreApplication::applicationDirPath() + QStringLiteral("/sounds"),
        QDir::currentPath() + QStringLiteral("/sounds"),
        platform::dataDirectory() + QStringLiteral("/sounds"),
        QCoreApplication::applicationDirPath() + QStringLiteral("/assets/sounds"),
        QDir::currentPath() + QStringLiteral("/assets/sounds"),
    };
    roots.removeDuplicates();
    QSet<QString> seenPaths;
    for (const QString& root : roots) {
        if (!QFileInfo(root).isDir()) {
            continue;
        }
        QDirIterator iterator(root, QDir::Files, QDirIterator::Subdirectories);
        while (iterator.hasNext()) {
            const QString path = iterator.next();
            const QFileInfo info(path);
            const QString suffix = info.suffix().toLower();
            if (suffix != QStringLiteral("wav") && suffix != QStringLiteral("mp3")
                && suffix != QStringLiteral("wma") && suffix != QStringLiteral("ogg")
                && suffix != QStringLiteral("flac") && suffix != QStringLiteral("m4a")) {
                continue;
            }
            QString baseName = info.completeBaseName().trimmed();
            if (baseName.isEmpty() || builtinBaseNames.contains(baseName.toLower())) {
                continue;
            }
            const QString canonical = info.canonicalFilePath();
            if (canonical.isEmpty() || seenPaths.contains(canonical)) {
                continue;
            }
            seenPaths.insert(canonical);
            QString displayName = baseName;
            if (usedNames.contains(displayName.toLower())) {
                displayName += QStringLiteral(" · ") + info.dir().dirName();
            }
            usedNames.insert(displayName.toLower());
            result.push_back({QStringLiteral("file:") + info.absoluteFilePath(),
                              displayName, info.absoluteFilePath()});
        }
    }
    std::sort(result.begin() + static_cast<std::ptrdiff_t>(builtinCount), result.end(),
              [](const BuiltinSound& left, const BuiltinSound& right) {
                  return left.name.localeAwareCompare(right.name) < 0;
              });
    return result;
}

const std::vector<CityInfo>& cities()
{
    static const std::vector<CityInfo> items{
        {QStringLiteral("Москва"), QStringLiteral("Россия"), QStringLiteral("Europe/Moscow")},
        {QStringLiteral("Санкт-Петербург"), QStringLiteral("Россия"), QStringLiteral("Europe/Moscow")},
        {QStringLiteral("Калининград"), QStringLiteral("Россия"), QStringLiteral("Europe/Kaliningrad")},
        {QStringLiteral("Самара"), QStringLiteral("Россия"), QStringLiteral("Europe/Samara")},
        {QStringLiteral("Екатеринбург"), QStringLiteral("Россия"), QStringLiteral("Asia/Yekaterinburg")},
        {QStringLiteral("Омск"), QStringLiteral("Россия"), QStringLiteral("Asia/Omsk")},
        {QStringLiteral("Новосибирск"), QStringLiteral("Россия"), QStringLiteral("Asia/Novosibirsk")},
        {QStringLiteral("Красноярск"), QStringLiteral("Россия"), QStringLiteral("Asia/Krasnoyarsk")},
        {QStringLiteral("Иркутск"), QStringLiteral("Россия"), QStringLiteral("Asia/Irkutsk")},
        {QStringLiteral("Якутск"), QStringLiteral("Россия"), QStringLiteral("Asia/Yakutsk")},
        {QStringLiteral("Владивосток"), QStringLiteral("Россия"), QStringLiteral("Asia/Vladivostok")},
        {QStringLiteral("Магадан"), QStringLiteral("Россия"), QStringLiteral("Asia/Magadan")},
        {QStringLiteral("Петропавловск-Камчатский"), QStringLiteral("Россия"), QStringLiteral("Asia/Kamchatka")},
        {QStringLiteral("Минск"), QStringLiteral("Беларусь"), QStringLiteral("Europe/Minsk")},
        {QStringLiteral("Киев"), QStringLiteral("Украина"), QStringLiteral("Europe/Kyiv")},
        {QStringLiteral("Алматы"), QStringLiteral("Казахстан"), QStringLiteral("Asia/Almaty")},
        {QStringLiteral("Астана"), QStringLiteral("Казахстан"), QStringLiteral("Asia/Almaty")},
        {QStringLiteral("Ташкент"), QStringLiteral("Узбекистан"), QStringLiteral("Asia/Tashkent")},
        {QStringLiteral("Тбилиси"), QStringLiteral("Грузия"), QStringLiteral("Asia/Tbilisi")},
        {QStringLiteral("Ереван"), QStringLiteral("Армения"), QStringLiteral("Asia/Yerevan")},
        {QStringLiteral("Баку"), QStringLiteral("Азербайджан"), QStringLiteral("Asia/Baku")},
        {QStringLiteral("Лондон"), QStringLiteral("Великобритания"), QStringLiteral("Europe/London")},
        {QStringLiteral("Париж"), QStringLiteral("Франция"), QStringLiteral("Europe/Paris")},
        {QStringLiteral("Берлин"), QStringLiteral("Германия"), QStringLiteral("Europe/Berlin")},
        {QStringLiteral("Мадрид"), QStringLiteral("Испания"), QStringLiteral("Europe/Madrid")},
        {QStringLiteral("Рим"), QStringLiteral("Италия"), QStringLiteral("Europe/Rome")},
        {QStringLiteral("Амстердам"), QStringLiteral("Нидерланды"), QStringLiteral("Europe/Amsterdam")},
        {QStringLiteral("Брюссель"), QStringLiteral("Бельгия"), QStringLiteral("Europe/Brussels")},
        {QStringLiteral("Вена"), QStringLiteral("Австрия"), QStringLiteral("Europe/Vienna")},
        {QStringLiteral("Прага"), QStringLiteral("Чехия"), QStringLiteral("Europe/Prague")},
        {QStringLiteral("Варшава"), QStringLiteral("Польша"), QStringLiteral("Europe/Warsaw")},
        {QStringLiteral("Стокгольм"), QStringLiteral("Швеция"), QStringLiteral("Europe/Stockholm")},
        {QStringLiteral("Осло"), QStringLiteral("Норвегия"), QStringLiteral("Europe/Oslo")},
        {QStringLiteral("Хельсинки"), QStringLiteral("Финляндия"), QStringLiteral("Europe/Helsinki")},
        {QStringLiteral("Копенгаген"), QStringLiteral("Дания"), QStringLiteral("Europe/Copenhagen")},
        {QStringLiteral("Цюрих"), QStringLiteral("Швейцария"), QStringLiteral("Europe/Zurich")},
        {QStringLiteral("Лиссабон"), QStringLiteral("Португалия"), QStringLiteral("Europe/Lisbon")},
        {QStringLiteral("Афины"), QStringLiteral("Греция"), QStringLiteral("Europe/Athens")},
        {QStringLiteral("Стамбул"), QStringLiteral("Турция"), QStringLiteral("Europe/Istanbul")},
        {QStringLiteral("Будапешт"), QStringLiteral("Венгрия"), QStringLiteral("Europe/Budapest")},
        {QStringLiteral("Бухарест"), QStringLiteral("Румыния"), QStringLiteral("Europe/Bucharest")},
        {QStringLiteral("Рейкьявик"), QStringLiteral("Исландия"), QStringLiteral("Atlantic/Reykjavik")},
        {QStringLiteral("Нью-Йорк"), QStringLiteral("США"), QStringLiteral("America/New_York")},
        {QStringLiteral("Лос-Анджелес"), QStringLiteral("США"), QStringLiteral("America/Los_Angeles")},
        {QStringLiteral("Чикаго"), QStringLiteral("США"), QStringLiteral("America/Chicago")},
        {QStringLiteral("Денвер"), QStringLiteral("США"), QStringLiteral("America/Denver")},
        {QStringLiteral("Сиэтл"), QStringLiteral("США"), QStringLiteral("America/Los_Angeles")},
        {QStringLiteral("Майами"), QStringLiteral("США"), QStringLiteral("America/New_York")},
        {QStringLiteral("Гонолулу"), QStringLiteral("США"), QStringLiteral("Pacific/Honolulu")},
        {QStringLiteral("Торонто"), QStringLiteral("Канада"), QStringLiteral("America/Toronto")},
        {QStringLiteral("Ванкувер"), QStringLiteral("Канада"), QStringLiteral("America/Vancouver")},
        {QStringLiteral("Мехико"), QStringLiteral("Мексика"), QStringLiteral("America/Mexico_City")},
        {QStringLiteral("Сан-Паулу"), QStringLiteral("Бразилия"), QStringLiteral("America/Sao_Paulo")},
        {QStringLiteral("Буэнос-Айрес"), QStringLiteral("Аргентина"), QStringLiteral("America/Argentina/Buenos_Aires")},
        {QStringLiteral("Богота"), QStringLiteral("Колумбия"), QStringLiteral("America/Bogota")},
        {QStringLiteral("Лима"), QStringLiteral("Перу"), QStringLiteral("America/Lima")},
        {QStringLiteral("Сантьяго"), QStringLiteral("Чили"), QStringLiteral("America/Santiago")},
        {QStringLiteral("Гавана"), QStringLiteral("Куба"), QStringLiteral("America/Havana")},
        {QStringLiteral("Токио"), QStringLiteral("Япония"), QStringLiteral("Asia/Tokyo")},
        {QStringLiteral("Пекин"), QStringLiteral("Китай"), QStringLiteral("Asia/Shanghai")},
        {QStringLiteral("Шанхай"), QStringLiteral("Китай"), QStringLiteral("Asia/Shanghai")},
        {QStringLiteral("Гонконг"), QStringLiteral("Китай"), QStringLiteral("Asia/Hong_Kong")},
        {QStringLiteral("Сеул"), QStringLiteral("Южная Корея"), QStringLiteral("Asia/Seoul")},
        {QStringLiteral("Сингапур"), QStringLiteral("Сингапур"), QStringLiteral("Asia/Singapore")},
        {QStringLiteral("Бангкок"), QStringLiteral("Таиланд"), QStringLiteral("Asia/Bangkok")},
        {QStringLiteral("Джакарта"), QStringLiteral("Индонезия"), QStringLiteral("Asia/Jakarta")},
        {QStringLiteral("Манила"), QStringLiteral("Филиппины"), QStringLiteral("Asia/Manila")},
        {QStringLiteral("Куала-Лумпур"), QStringLiteral("Малайзия"), QStringLiteral("Asia/Kuala_Lumpur")},
        {QStringLiteral("Хошимин"), QStringLiteral("Вьетнам"), QStringLiteral("Asia/Ho_Chi_Minh")},
        {QStringLiteral("Дели"), QStringLiteral("Индия"), QStringLiteral("Asia/Kolkata")},
        {QStringLiteral("Мумбаи"), QStringLiteral("Индия"), QStringLiteral("Asia/Kolkata")},
        {QStringLiteral("Дубай"), QStringLiteral("ОАЭ"), QStringLiteral("Asia/Dubai")},
        {QStringLiteral("Доха"), QStringLiteral("Катар"), QStringLiteral("Asia/Qatar")},
        {QStringLiteral("Рияд"), QStringLiteral("Саудовская Аравия"), QStringLiteral("Asia/Riyadh")},
        {QStringLiteral("Тегеран"), QStringLiteral("Иран"), QStringLiteral("Asia/Tehran")},
        {QStringLiteral("Иерусалим"), QStringLiteral("Израиль"), QStringLiteral("Asia/Jerusalem")},
        {QStringLiteral("Каир"), QStringLiteral("Египет"), QStringLiteral("Africa/Cairo")},
        {QStringLiteral("Кейптаун"), QStringLiteral("ЮАР"), QStringLiteral("Africa/Johannesburg")},
        {QStringLiteral("Найроби"), QStringLiteral("Кения"), QStringLiteral("Africa/Nairobi")},
        {QStringLiteral("Лагос"), QStringLiteral("Нигерия"), QStringLiteral("Africa/Lagos")},
        {QStringLiteral("Касабланка"), QStringLiteral("Марокко"), QStringLiteral("Africa/Casablanca")},
        {QStringLiteral("Сидней"), QStringLiteral("Австралия"), QStringLiteral("Australia/Sydney")},
        {QStringLiteral("Мельбурн"), QStringLiteral("Австралия"), QStringLiteral("Australia/Melbourne")},
        {QStringLiteral("Брисбен"), QStringLiteral("Австралия"), QStringLiteral("Australia/Brisbane")},
        {QStringLiteral("Перт"), QStringLiteral("Австралия"), QStringLiteral("Australia/Perth")},
        {QStringLiteral("Окленд"), QStringLiteral("Новая Зеландия"), QStringLiteral("Pacific/Auckland")},
        {QStringLiteral("Сува"), QStringLiteral("Фиджи"), QStringLiteral("Pacific/Fiji")},
    };
    return items;
}

QString soundName(const QString& soundId)
{
    for (const auto& sound : builtinSounds()) {
        if (sound.id == soundId) {
            return sound.name;
        }
    }
    if (soundId.startsWith(QStringLiteral("file:"))) {
        const QString path = soundId.mid(5);
        return QFileInfo(path).completeBaseName();
    }
    return QStringLiteral("Без звука");
}

QString soundResource(const QString& soundId)
{
    for (const auto& sound : builtinSounds()) {
        if (sound.id == soundId) {
            return sound.resource;
        }
    }
    return {};
}

QString priorityName(int priority)
{
    switch (priority) {
    case 0:
        return QStringLiteral("Низкий");
    case 2:
        return QStringLiteral("Высокий");
    default:
        return QStringLiteral("Обычный");
    }
}

QString dayShort(Qt::DayOfWeek day)
{
    switch (day) {
    case Qt::Monday:
        return QStringLiteral("Пн");
    case Qt::Tuesday:
        return QStringLiteral("Вт");
    case Qt::Wednesday:
        return QStringLiteral("Ср");
    case Qt::Thursday:
        return QStringLiteral("Чт");
    case Qt::Friday:
        return QStringLiteral("Пт");
    case Qt::Saturday:
        return QStringLiteral("Сб");
    case Qt::Sunday:
        return QStringLiteral("Вс");
    }
    return {};
}

QString dayLong(Qt::DayOfWeek day)
{
    switch (day) {
    case Qt::Monday:
        return QStringLiteral("Понедельник");
    case Qt::Tuesday:
        return QStringLiteral("Вторник");
    case Qt::Wednesday:
        return QStringLiteral("Среда");
    case Qt::Thursday:
        return QStringLiteral("Четверг");
    case Qt::Friday:
        return QStringLiteral("Пятница");
    case Qt::Saturday:
        return QStringLiteral("Суббота");
    case Qt::Sunday:
        return QStringLiteral("Воскресенье");
    }
    return {};
}

QString monthName(int month)
{
    static const QStringList names{
        QStringLiteral("Январь"), QStringLiteral("Февраль"), QStringLiteral("Март"),
        QStringLiteral("Апрель"), QStringLiteral("Май"), QStringLiteral("Июнь"),
        QStringLiteral("Июль"), QStringLiteral("Август"), QStringLiteral("Сентябрь"),
        QStringLiteral("Октябрь"), QStringLiteral("Ноябрь"), QStringLiteral("Декабрь")};
    return (month >= 1 && month <= 12) ? names.at(month - 1) : QString();
}

QString monthNameGenitive(int month)
{
    static const QStringList names{
        QStringLiteral("января"), QStringLiteral("февраля"), QStringLiteral("марта"),
        QStringLiteral("апреля"), QStringLiteral("мая"), QStringLiteral("июня"),
        QStringLiteral("июля"), QStringLiteral("августа"), QStringLiteral("сентября"),
        QStringLiteral("октября"), QStringLiteral("ноября"), QStringLiteral("декабря")};
    return (month >= 1 && month <= 12) ? names.at(month - 1) : QString();
}

} // namespace pac::catalog
