#pragma once

#include <QElapsedTimer>
#include <QTimer>
#include <QVector>
#include <QWidget>

namespace pac::ui {

class ThemeEffects final : public QWidget {
    Q_OBJECT

public:
    explicit ThemeEffects(QWidget* parent = nullptr);

    void setSuspended(bool suspended);
    void restartEffect();

protected:
    void paintEvent(QPaintEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private:
    enum class Mode {
        None,
        Halloween,
        NewYear
    };

    struct Particle {
        qreal x{0.0};
        qreal y{0.0};
        qreal vx{0.0};
        qreal vy{0.0};
        qreal size{1.0};
        qreal phase{0.0};
        int kind{0};
    };

    void updateMode();
    void seedParticles();
    void advance();

    Mode m_mode{Mode::None};
    bool m_suspended{false};
    QTimer m_timer;
    QElapsedTimer m_clock;
    QVector<Particle> m_particles;
    qreal m_elapsed{0.0};
};

} // namespace pac::ui
