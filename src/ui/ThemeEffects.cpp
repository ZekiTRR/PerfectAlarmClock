#include "ThemeEffects.h"

#include "core/Theme.h"

#include <QPainter>
#include <QPainterPath>
#include <QShowEvent>
#include <QRandomGenerator>
#include <QRadialGradient>

#include <algorithm>
#include <cmath>

namespace pac::ui {
namespace {

constexpr qreal kPi = 3.14159265358979323846;

[[nodiscard]] qreal randomBetween(qreal minimum, qreal maximum)
{
    return minimum + QRandomGenerator::global()->generateDouble() * (maximum - minimum);
}

} // namespace

ThemeEffects::ThemeEffects(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setFocusPolicy(Qt::NoFocus);
    setObjectName(QStringLiteral("Transparent"));

    m_timer.setInterval(50);
    connect(&m_timer, &QTimer::timeout, this, [this] {
        advance();
        update();
    });
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this,
            [this] { updateMode(); });
    updateMode();
}

void ThemeEffects::setSuspended(bool suspended)
{
    m_suspended = suspended;
    updateMode();
}

void ThemeEffects::restartEffect()
{
    updateMode();
}

void ThemeEffects::updateMode()
{
    const QString id = ThemeManager::instance().current().id;
    Mode mode = Mode::None;
    if (id == QStringLiteral("halloween")) {
        mode = Mode::Halloween;
    } else if (id == QStringLiteral("new-year")) {
        mode = Mode::NewYear;
    }
    if (mode == m_mode) {
        if (mode == Mode::None || m_suspended || !isVisible()) {
            m_timer.stop();
        } else {
            if (m_particles.isEmpty()) {
                seedParticles();
                m_clock.restart();
            }
            if (!m_timer.isActive()) {
                m_timer.start();
            }
        }
        update();
        return;
    }
    m_mode = mode;
    m_particles.clear();
    m_elapsed = 0.0;
    if (m_mode == Mode::None || m_suspended || !isVisible()) {
        m_timer.stop();
        update();
        return;
    }
    seedParticles();
    m_clock.restart();
    m_timer.start();
    update();
}

void ThemeEffects::seedParticles()
{
    m_particles.clear();
    const int count = m_mode == Mode::Halloween ? 24 : 44;
    m_particles.reserve(count);
    for (int index = 0; index < count; ++index) {
        Particle particle;
        particle.x = randomBetween(0.02, 0.98);
        particle.y = randomBetween(0.02, 0.98);
        particle.size = randomBetween(1.4, 4.2);
        particle.phase = randomBetween(0.0, kPi * 2.0);
        if (m_mode == Mode::Halloween) {
            particle.kind = index % 4 == 0 ? 1 : 0;
            particle.vx = randomBetween(-0.010, 0.010);
            particle.vy = randomBetween(-0.028, -0.008);
            if (particle.kind == 1) {
                particle.size = randomBetween(5.0, 10.0);
                particle.vx *= 2.4;
            }
        } else {
            particle.kind = index % 13 == 0 ? 2 : (index % 5 == 0 ? 1 : 0);
            particle.vx = randomBetween(-0.010, 0.010);
            particle.vy = randomBetween(0.012, 0.045);
            if (particle.kind == 1) {
                particle.size = randomBetween(3.5, 7.5);
            } else if (particle.kind == 2) {
                particle.size = randomBetween(10.0, 22.0);
                particle.vy *= 0.35;
            }
        }
        m_particles.append(particle);
    }
}

void ThemeEffects::advance()
{
    if (m_mode == Mode::None || m_suspended) {
        return;
    }
    const qreal dt = std::clamp(m_elapsed == 0.0 ? 0.05 : 0.05, 0.01, 0.12);
    m_elapsed += dt;
    for (auto& particle : m_particles) {
        particle.x += particle.vx * dt + std::sin(m_elapsed * 1.7 + particle.phase) * 0.0009;
        particle.y += particle.vy * dt;
        if (m_mode == Mode::Halloween) {
            if (particle.y < -0.05) {
                particle.y = 1.04;
                particle.x = randomBetween(0.02, 0.98);
            }
        } else {
            if (particle.y > 1.05) {
                particle.y = -0.04;
                particle.x = randomBetween(0.02, 0.98);
            }
        }
        if (particle.x < -0.05) {
            particle.x = 1.04;
        } else if (particle.x > 1.05) {
            particle.x = -0.04;
        }
    }
}

void ThemeEffects::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    updateMode();
}

void ThemeEffects::hideEvent(QHideEvent* event)
{
    QWidget::hideEvent(event);
    m_timer.stop();
}

void ThemeEffects::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    if (m_mode == Mode::None) {
        return;
    }
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    const QRectF area = rect();
    const qreal width = area.width();
    const qreal height = area.height();
    if (width <= 0.0 || height <= 0.0) {
        return;
    }

    if (m_mode == Mode::Halloween) {
        QRadialGradient moon(width * 0.88, height * 0.06, width * 0.32);
        moon.setColorAt(0.0, QColor(255, 145, 60, 30));
        moon.setColorAt(1.0, QColor(255, 145, 60, 0));
        painter.fillRect(area, moon);
        for (const auto& particle : m_particles) {
            const QPointF point(particle.x * width, particle.y * height);
            if (particle.kind == 1) {
                const qreal size = particle.size;
                QPainterPath bat;
                bat.moveTo(point.x(), point.y());
                bat.cubicTo(point.x() - size * 0.8, point.y() - size * 0.8,
                            point.x() - size * 1.9, point.y() - size * 0.5,
                            point.x() - size * 2.1, point.y() + size * 0.2);
                bat.cubicTo(point.x() - size * 1.1, point.y() + size * 0.1,
                            point.x() - size * 0.7, point.y() + size * 0.55,
                            point.x(), point.y() + size * 0.35);
                bat.cubicTo(point.x() + size * 0.7, point.y() + size * 0.55,
                            point.x() + size * 1.1, point.y() + size * 0.1,
                            point.x() + size * 2.1, point.y() + size * 0.2);
                bat.cubicTo(point.x() + size * 1.9, point.y() - size * 0.5,
                            point.x() + size * 0.8, point.y() - size * 0.8,
                            point.x(), point.y());
                painter.setPen(Qt::NoPen);
                painter.setBrush(QColor(28, 12, 32, 120));
                painter.drawPath(bat);
            } else {
                QRadialGradient ember(point, particle.size * 3.8);
                ember.setColorAt(0.0, QColor(255, 180, 80, 115));
                ember.setColorAt(0.45, QColor(255, 110, 30, 60));
                ember.setColorAt(1.0, QColor(255, 80, 20, 0));
                painter.setPen(Qt::NoPen);
                painter.setBrush(ember);
                painter.drawEllipse(point, particle.size * 3.8, particle.size * 3.8);
            }
        }
        return;
    }

    QRadialGradient goldGlow(width * 0.12, height * 0.08, width * 0.32);
    goldGlow.setColorAt(0.0, QColor(232, 193, 90, 34));
    goldGlow.setColorAt(1.0, QColor(232, 193, 90, 0));
    painter.fillRect(area, goldGlow);
    QRadialGradient iceGlow(width * 0.9, height * 0.18, width * 0.28);
    iceGlow.setColorAt(0.0, QColor(120, 205, 235, 30));
    iceGlow.setColorAt(1.0, QColor(120, 205, 235, 0));
    painter.fillRect(area, iceGlow);

    QPainterPath snowBank;
    snowBank.moveTo(0.0, height);
    snowBank.lineTo(0.0, height - 13.0);
    snowBank.cubicTo(width * 0.18, height - 27.0, width * 0.31, height - 6.0,
                     width * 0.48, height - 18.0);
    snowBank.cubicTo(width * 0.62, height - 28.0, width * 0.76, height - 8.0,
                     width, height - 20.0);
    snowBank.lineTo(width, height);
    snowBank.closeSubpath();
    painter.setPen(QPen(QColor(160, 205, 225, 170), 1.2));
    painter.setBrush(QColor(241, 250, 255, 225));
    painter.drawPath(snowBank);

    for (const auto& particle : m_particles) {
        const QPointF point(particle.x * width, particle.y * height);
        const qreal twinkle = 0.55 + 0.45 * std::sin(m_elapsed * 2.1 + particle.phase);
        if (particle.kind == 2) {
            QRadialGradient bokeh(point, particle.size);
            bokeh.setColorAt(0.0, QColor(255, 222, 130, static_cast<int>(45 + twinkle * 55)));
            bokeh.setColorAt(0.5, QColor(255, 205, 90, static_cast<int>(20 + twinkle * 25)));
            bokeh.setColorAt(1.0, QColor(255, 205, 90, 0));
            painter.setPen(Qt::NoPen);
            painter.setBrush(bokeh);
            painter.drawEllipse(point, particle.size, particle.size);
        } else if (particle.kind == 1) {
            const qreal size = particle.size * (0.8 + twinkle * 0.5);
            painter.setPen(QPen(QColor(186, 139, 48, static_cast<int>(125 + twinkle * 100)),
                                1.5));
            painter.drawLine(QPointF(point.x() - size, point.y()),
                             QPointF(point.x() + size, point.y()));
            painter.drawLine(QPointF(point.x(), point.y() - size),
                             QPointF(point.x(), point.y() + size));
        } else {
            const qreal size = particle.size;
            painter.setPen(QPen(QColor(91, 150, 180, static_cast<int>(105 + twinkle * 100)),
                                1.3));
            for (int spoke = 0; spoke < 3; ++spoke) {
                const qreal angle = kPi * spoke / 3.0;
                const QPointF offset(std::cos(angle) * size, std::sin(angle) * size);
                painter.drawLine(point - offset, point + offset);
            }
            painter.setBrush(QColor(74, 133, 164, static_cast<int>(80 + twinkle * 80)));
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(point, size * 0.4, size * 0.4);
        }
    }
}

} // namespace pac::ui
