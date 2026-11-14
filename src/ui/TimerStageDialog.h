#pragma once

#include "core/Types.h"

#include <QDialog>

class QComboBox;
class QLabel;
class QLineEdit;
class QSpinBox;

namespace pac::ui {

class ThemedButton;

class TimerStageDialog final : public QDialog {
    Q_OBJECT

public:
    explicit TimerStageDialog(const TimerStage& stage, QWidget* parent = nullptr);
    ~TimerStageDialog() override;

    [[nodiscard]] TimerStage result() const;

protected:
    void accept() override;

private:
    void buildUi();
    void setDuration(int seconds);
    [[nodiscard]] int durationSeconds() const;
    void togglePreview();

    TimerStage m_stage;
    QLineEdit* m_name{nullptr};
    QLineEdit* m_message{nullptr};
    QSpinBox* m_hours{nullptr};
    QSpinBox* m_minutes{nullptr};
    QSpinBox* m_seconds{nullptr};
    QComboBox* m_kind{nullptr};
    QComboBox* m_color{nullptr};
    QComboBox* m_sound{nullptr};
    ThemedButton* m_preview{nullptr};
    QLabel* m_previewHint{nullptr};
    bool m_previewing{false};
};

} // namespace pac::ui
