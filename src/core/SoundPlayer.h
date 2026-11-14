#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QString>
#include <QTimer>

namespace pac {

class SoundPlayer final : public QObject {
    Q_OBJECT

public:
    static SoundPlayer& instance();

    SoundPlayer(const SoundPlayer&) = delete;
    SoundPlayer& operator=(const SoundPlayer&) = delete;
    SoundPlayer(SoundPlayer&&) = delete;
    SoundPlayer& operator=(SoundPlayer&&) = delete;

    void play(const QString& soundId, int volumePercent, bool loop, int maxMinutes = 0,
              bool fadeIn = false, int fadeInSeconds = 6);
    void stop();
    void stopWithFade(int seconds);
    [[nodiscard]] bool isPlaying() const noexcept;

    [[nodiscard]] static QString resolveSoundPath(const QString& soundId);

private:
    SoundPlayer();
    ~SoundPlayer() override;

    bool openAlias(const QString& filePath, bool fallbackPlaySound, bool loop);
    void closeAlias();
    void startAliasPlayback();
    void restartLoop();
    void applyVolume(int percent);
    void updateFade();
    void scheduleStop(int maxMinutes);

    QString m_alias;
    int m_aliasCounter{0};
    bool m_playing{false};
    bool m_fallback{false};
    int m_targetVolume{80};
    int m_fadeVolume{80};
    QTimer m_maxTimer;
    QTimer m_fadeTimer;
    QTimer m_loopTimer;
    QElapsedTimer m_fadeClock;
    qint64 m_loopLengthMs{0};
    bool m_loop{false};
    bool m_fading{false};
    bool m_fadingOut{false};
    qint64 m_fadeInDurationMs{6000};
    qint64 m_fadeOutDurationMs{0};
    int m_currentVolume{0};
    int m_fadeOutStartVolume{0};
};

} // namespace pac
