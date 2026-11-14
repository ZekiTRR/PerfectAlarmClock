#pragma once

#include "core/Types.h"

#include <QDialog>
#include <QList>

class QCheckBox;
class QComboBox;
class QDateEdit;
class QLabel;
class QPushButton;
class QSpinBox;
class QTimeEdit;
class QWidget;

namespace pac::ui {

class AlarmScheduleDialog final : public QDialog {
    Q_OBJECT

public:
    explicit AlarmScheduleDialog(const AlarmSchedule& schedule, QWidget* parent = nullptr);

    [[nodiscard]] AlarmSchedule result() const;

protected:
    void accept() override;

private:
    void buildUi();
    void updatePanels();
    [[nodiscard]] AlarmSchedule collect() const;

    AlarmSchedule m_schedule;
    QTimeEdit* m_time{nullptr};
    QCheckBox* m_enabled{nullptr};
    QComboBox* m_repeat{nullptr};
    QDateEdit* m_anchor{nullptr};
    QLabel* m_anchorLabel{nullptr};
    QWidget* m_anchorRow{nullptr};
    QWidget* m_weeklyPanel{nullptr};
    QList<QPushButton*> m_dayButtons;
    QWidget* m_everyDaysPanel{nullptr};
    QSpinBox* m_everyDays{nullptr};
    QWidget* m_everyWeeksPanel{nullptr};
    QSpinBox* m_everyWeeks{nullptr};
    QWidget* m_nthPanel{nullptr};
    QComboBox* m_nthWeekday{nullptr};
    QComboBox* m_nthWeek{nullptr};
    QWidget* m_monthlyPanel{nullptr};
    QSpinBox* m_monthDay{nullptr};
    QWidget* m_intervalPanel{nullptr};
    QSpinBox* m_intervalMinutes{nullptr};
    QTimeEdit* m_intervalFrom{nullptr};
    QTimeEdit* m_intervalTo{nullptr};
    QLabel* m_repeatHint{nullptr};
};

} // namespace pac::ui
