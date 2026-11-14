#include "SoundPlayer.h"

#include "Catalog.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QStandardPaths>

#include <algorithm>
#include <iterator>
#include <limits>

#ifdef Q_OS_WIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mmsystem.h>
#endif

Q_LOGGING_CATEGORY(lcSound, "pac.sound")

namespace pac {
namespace {

#ifdef Q_OS_WIN
void sendMci(const QString& command)
{
    wchar_t buffer[256]{};
    const MCIERROR error = mciSendStringW(reinterpret_cast<LPCWSTR>(command.utf16()), buffer,
                                          static_cast<UINT>(std::size(buffer)), nullptr);
    if (error != 0) {
        wchar_t message[256]{};
        mciGetErrorStringW(error, message, static_cast<UINT>(std::size(message)));
        qCWarning(lcSound) << "MCI error:" << QString::fromWCharArray(message) << "in" << command;
    }
}

[[nodiscard]] QString queryMci(const QString& command)
{
    wchar_t buffer[256]{};
    const MCIERROR error = mciSendStringW(reinterpret_cast<LPCWSTR>(command.utf16()), buffer,
                                          static_cast<UINT>(std::size(buffer)), nullptr);
    return error == 0 ? QString::fromWCharArray(buffer) : QString();
}
#endif

} // namespace

SoundPlayer& SoundPlayer::instance()
{
    static SoundPlayer player;
    return player;
}

SoundPlayer::SoundPlayer()
{
    m_maxTimer.setSingleShot(true);
    connect(&m_maxTimer, &QTimer::timeout, this, &SoundPlayer::stop);
    m_fadeTimer.setInterval(200);
    connect(&m_fadeTimer, &QTimer::timeout, this, &SoundPlayer::updateFade);
    m_loopTimer.setSingleShot(true);
    connect(&m_loopTimer, &QTimer::timeout, this, &SoundPlayer::restartLoop);
}

SoundPlayer::~SoundPlayer()
{
    stop();
}

QString SoundPlayer::resolveSoundPath(const QString& soundId)
{
    QString sourcePath;
    if (soundId.startsWith(QStringLiteral("file:"))) {
        sourcePath = soundId.mid(5);
    } else {
        QString resource = catalog::soundResource(soundId);
        if (resource.isEmpty()) {
            resource = catalog::soundResource(QStringLiteral("builtin:chime"));
        }
        if (resource.isEmpty()) {
            return {};
        }
        const QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
                                 + QStringLiteral("/sounds");
        QDir().mkpath(cacheDir);
        const QString target = cacheDir + QLatin1Char('/') + QFileInfo(resource).fileName();
        const QFileInfo sourceInfo(resource);
        const QFileInfo targetInfo(target);
        if (!targetInfo.exists() || targetInfo.size() != sourceInfo.size()) {
            QFile::remove(target);
            if (!QFile::copy(resource, target)) {
                return {};
            }
        }
        return target;
    }

    const QFileInfo info(sourcePath);
    return info.exists() && info.isFile() ? info.absoluteFilePath() : QString();
}

void SoundPlayer::play(const QString& soundId, int volumePercent, bool loop, int maxMinutes,
                       bool fadeIn, int fadeInSeconds)
{
    stop();
    m_fadingOut = false;
    m_fadeInDurationMs = std::max<qint64>(1, static_cast<qint64>(fadeInSeconds) * 1000);
    const QString path = resolveSoundPath(soundId);
    if (path.isEmpty()) {
        qCWarning(lcSound) << "Sound file was not found for" << soundId;
        return;
    }

    m_targetVolume = std::clamp(volumePercent, 0, 100);
    m_fadeVolume = fadeIn ? std::max(1, m_targetVolume / 8) : m_targetVolume;
    m_currentVolume = m_fadeVolume;
    m_fading = fadeIn && m_targetVolume > 0;
    m_loop = loop;

    if (!openAlias(path, true, loop)) {
        return;
    }
    m_playing = true;

#ifdef Q_OS_WIN
    if (!m_fallback) {
        sendMci(QStringLiteral("set %1 time format milliseconds").arg(m_alias));
        sendMci(QStringLiteral("setaudio %1 volume to %2").arg(m_alias).arg(m_fadeVolume * 10));
        m_loopLengthMs = queryMci(QStringLiteral("status %1 length").arg(m_alias)).toLongLong();
        startAliasPlayback();
    }
#endif

    if (m_fading) {
        m_fadeClock.start();
        m_fadeTimer.start();
    }
    scheduleStop(maxMinutes);
}

void SoundPlayer::stopWithFade(int seconds)
{
    if (!m_playing || seconds <= 0) {
        stop();
        return;
    }
    m_maxTimer.stop();
    m_fading = false;
    m_fadingOut = true;
    m_fadeOutDurationMs = std::max<qint64>(1, static_cast<qint64>(seconds) * 1000);
    m_fadeOutStartVolume = std::max(1, m_currentVolume);
    m_loopTimer.stop();
    m_fadeClock.restart();
    m_fadeTimer.start();
}

void SoundPlayer::stop()
{
    m_fadeTimer.stop();
    m_loopTimer.stop();
    m_loop = false;
    m_loopLengthMs = 0;
    m_fading = false;
    m_fadingOut = false;
    if (!m_playing) {
        closeAlias();
        return;
    }
#ifdef Q_OS_WIN
    if (m_fallback) {
        PlaySoundW(nullptr, nullptr, 0);
    } else {
        closeAlias();
    }
#else
    closeAlias();
#endif
    m_playing = false;
    m_fallback = false;
    m_maxTimer.stop();
}

bool SoundPlayer::isPlaying() const noexcept
{
    return m_playing;
}

bool SoundPlayer::openAlias(const QString& filePath, bool fallbackPlaySound, bool loop)
{
#ifdef Q_OS_WIN
    closeAlias();
    m_alias = QStringLiteral("pac_sound_%1").arg(++m_aliasCounter);
    const QString native = QDir::toNativeSeparators(filePath);
    const QString type = QFileInfo(filePath).suffix().compare(QStringLiteral("wav"), Qt::CaseInsensitive) == 0
                             ? QStringLiteral("waveaudio")
                             : QStringLiteral("mpegvideo");
    wchar_t buffer[256]{};
    const QString openCommand = QStringLiteral("open \"%1\" type %2 alias %3")
                                    .arg(native, type, m_alias);
    const MCIERROR error = mciSendStringW(reinterpret_cast<LPCWSTR>(openCommand.utf16()), buffer,
                                          static_cast<UINT>(std::size(buffer)), nullptr);
    if (error == 0) {
        m_fallback = false;
        return true;
    }
    m_alias.clear();
    if (fallbackPlaySound) {
        DWORD flags = SND_FILENAME | SND_ASYNC | SND_NODEFAULT;
        if (loop) {
            flags |= SND_LOOP;
        }
        if (PlaySoundW(reinterpret_cast<LPCWSTR>(native.utf16()), nullptr, flags)) {
            m_fallback = true;
            qCWarning(lcSound) << "MCI unavailable, used PlaySound fallback for" << native;
            return true;
        }
    }
    qCWarning(lcSound) << "Unable to open sound" << native;
    return false;
#else
    Q_UNUSED(filePath)
    Q_UNUSED(fallbackPlaySound)
    Q_UNUSED(loop)
    return false;
#endif
}

void SoundPlayer::closeAlias()
{
#ifdef Q_OS_WIN
    if (!m_alias.isEmpty()) {
        sendMci(QStringLiteral("stop %1").arg(m_alias));
        sendMci(QStringLiteral("close %1").arg(m_alias));
        m_alias.clear();
    }
#endif
}

void SoundPlayer::startAliasPlayback()
{
#ifdef Q_OS_WIN
    if (m_alias.isEmpty() || m_fallback) {
        return;
    }
    if (m_loop && m_loopLengthMs > 0) {
        sendMci(QStringLiteral("play %1 from 0").arg(m_alias));
        const qint64 interval = std::clamp<qint64>(
            m_loopLengthMs, 250, std::numeric_limits<int>::max());
        m_loopTimer.start(static_cast<int>(interval));
        return;
    }
    if (m_loop) {
        sendMci(QStringLiteral("play %1 repeat").arg(m_alias));
        return;
    }
    sendMci(QStringLiteral("play %1 from 0").arg(m_alias));
#endif
}

void SoundPlayer::restartLoop()
{
#ifdef Q_OS_WIN
    if (!m_playing || m_fadingOut || m_fallback || m_alias.isEmpty() || !m_loop) {
        return;
    }
    const QString mode = queryMci(QStringLiteral("status %1 mode").arg(m_alias));
    if (mode.compare(QStringLiteral("playing"), Qt::CaseInsensitive) == 0) {
        m_loopTimer.start(80);
        return;
    }
    startAliasPlayback();
#endif
}

void SoundPlayer::applyVolume(int percent)
{
    m_currentVolume = std::clamp(percent, 0, 100);
#ifdef Q_OS_WIN
    if (!m_fallback && !m_alias.isEmpty()) {
        sendMci(QStringLiteral("setaudio %1 volume to %2").arg(m_alias).arg(std::clamp(percent, 0, 100) * 10));
    }
#else
    Q_UNUSED(percent)
#endif
}

void SoundPlayer::updateFade()
{
    const qint64 elapsed = m_fadeClock.elapsed();
    if (m_fadingOut) {
        const qreal progress =
            std::min<qreal>(1.0, static_cast<qreal>(elapsed) / m_fadeOutDurationMs);
        const int volume = static_cast<int>(m_fadeOutStartVolume * (1.0 - progress));
        applyVolume(volume);
        if (progress >= 1.0) {
            m_fadeTimer.stop();
            m_fadingOut = false;
            stop();
        }
        return;
    }
    if (!m_fading) {
        return;
    }
    const qreal progress =
        std::min<qreal>(1.0, static_cast<qreal>(elapsed) / m_fadeInDurationMs);
    const int volume = std::max(1, m_targetVolume / 8)
                       + static_cast<int>((m_targetVolume - std::max(1, m_targetVolume / 8))
                                          * progress);
    applyVolume(volume);
    if (progress >= 1.0) {
        m_fading = false;
        m_fadeTimer.stop();
    }
}

void SoundPlayer::scheduleStop(int maxMinutes)
{
    if (maxMinutes > 0) {
        m_maxTimer.start(maxMinutes * 60 * 1000);
    }
}

} // namespace pac
