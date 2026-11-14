#pragma once

#include "core/Types.h"

#include <QDialog>

class QCheckBox;
class QComboBox;
class QSpinBox;

namespace pac::ui {

class PomodoroSettingsDialog final : public QDialog {
    Q_OBJECT

public:
    explicit PomodoroSettingsDialog(const PomodoroSettings& settings, QWidget* parent = nullptr);

    [[nodiscard]] PomodoroSettings result() const;
    [[nodiscard]] bool resetStatsRequested() const;

protected:
    void accept() override;

private:
    PomodoroSettings m_settings;

    QSpinBox* m_work{nullptr};
    QSpinBox* m_shortBreak{nullptr};
    QSpinBox* m_longBreak{nullptr};
    QSpinBox* m_longEvery{nullptr};
    QSpinBox* m_dailyGoal{nullptr};
    QCheckBox* m_autoBreaks{nullptr};
    QCheckBox* m_autoWork{nullptr};
    QCheckBox* m_soundEnabled{nullptr};
    QComboBox* m_soundCombo{nullptr};
    QSpinBox* m_volume{nullptr};
    QCheckBox* m_notifications{nullptr};
    QCheckBox* m_resetStats{nullptr};
};

} // namespace pac::ui
