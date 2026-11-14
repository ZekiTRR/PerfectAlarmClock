#pragma once

#include "core/Types.h"

#include <QDialog>

class QCheckBox;
class QComboBox;
class QDateTimeEdit;
class QListWidget;
class QLabel;
class QLineEdit;
class QPushButton;
class QSlider;
class QSpinBox;

namespace pac::ui {

class ThemedButton;

class TimerDialog final : public QDialog {
    Q_OBJECT

public:
    TimerDialog(const CountdownTimer& timer, bool isNew, QWidget* parent = nullptr);

    [[nodiscard]] CountdownTimer result() const;
    [[nodiscard]] bool startImmediately() const;

protected:
    void accept() override;
    void reject() override;

private:
    void buildUi();
    void setDuration(int hours, int minutes, int seconds);
    [[nodiscard]] int durationSeconds() const;
    void updateSummary();
    void updateSoundControls();
    void updateModeUi();
    void refreshStageList();
    void updatePlanSummary();
    void addStage();
    void editStage();
    void removeStage();
    void moveStage(int delta);
    void syncStagesFromList();
    void applyPreset(int index);
    void chooseSound();
    void togglePreview();
    [[nodiscard]] QString soundId() const;
    [[nodiscard]] int selectedStageIndex() const;

    CountdownTimer m_timer;
    bool m_isNew{false};
    bool m_previewing{false};

    QWidget* m_durationRow{nullptr};
    QWidget* m_quickPresets{nullptr};
    QWidget* m_stageBox{nullptr};
    QComboBox* m_mode{nullptr};
    QDateTimeEdit* m_targetAt{nullptr};
    QWidget* m_targetRow{nullptr};
    QSpinBox* m_randomMin{nullptr};
    QSpinBox* m_randomMax{nullptr};
    QWidget* m_randomRow{nullptr};
    QComboBox* m_presetCombo{nullptr};
    QSpinBox* m_cycles{nullptr};
    QListWidget* m_stageList{nullptr};
    QLabel* m_planSummary{nullptr};
    bool m_loading{false};
    QSpinBox* m_hours{nullptr};
    QSpinBox* m_minutes{nullptr};
    QSpinBox* m_seconds{nullptr};
    QLineEdit* m_label{nullptr};
    QLabel* m_summary{nullptr};
    QCheckBox* m_startNow{nullptr};
    QCheckBox* m_autoRepeat{nullptr};
    QCheckBox* m_popup{nullptr};
    QCheckBox* m_soundEnabled{nullptr};
    QComboBox* m_soundCombo{nullptr};
    QPushButton* m_browseSound{nullptr};
    ThemedButton* m_previewButton{nullptr};
    QSlider* m_volume{nullptr};
    QLabel* m_volumeValue{nullptr};
    QCheckBox* m_loop{nullptr};
};

} // namespace pac::ui
