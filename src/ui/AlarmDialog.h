#pragma once

#include "core/Types.h"

#include <QDialog>
#include <QList>
#include <QStringList>

class QCheckBox;
class QComboBox;
class QDateEdit;
class QDialogButtonBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QPushButton;
class QSlider;
class QScrollArea;
class QSpinBox;
class QTimeEdit;
class QToolButton;
class QWidget;

namespace pac::ui {

class ThemedButton;

class AlarmDialog final : public QDialog {
    Q_OBJECT

public:
    AlarmDialog(const Alarm& alarm, bool isNew, const QStringList& groups,
                QWidget* parent = nullptr);
    ~AlarmDialog() override;

    [[nodiscard]] Alarm result() const;

protected:
    void accept() override;
    void reject() override;
    void showEvent(QShowEvent* event) override;

private:
    void buildUi();
    void populateSounds();
    void populateGroups(const QStringList& groups);
    void loadAlarm();
    void updateRepeatPanels();
    void updateSchedulePreview();
    void updateAppearance();
    void updateSoundControls();
    void updateSnoozeControls();
    void updatePreAlertControls();
    void updateActiveRangeControls();
    void updateExtraScheduleList();
    void addExtraSchedule();
    void editExtraSchedule();
    void removeExtraSchedule();
    void chooseSoundFile();
    void togglePreview();
    [[nodiscard]] Alarm collect() const;
    [[nodiscard]] bool validate(const Alarm& alarm);
    [[nodiscard]] QString selectedSoundId() const;

    Alarm m_alarm;
    Alarm m_original;
    bool m_isNew{false};
    bool m_previewing{false};
    QString m_customSoundId;
    RepeatKind m_originalRepeat{RepeatKind::Daily};

    QLabel* m_iconPreview{nullptr};
    QTimeEdit* m_timeEdit{nullptr};
    QLineEdit* m_labelEdit{nullptr};
    QComboBox* m_groupCombo{nullptr};
    QCheckBox* m_enabledCheck{nullptr};

    QComboBox* m_repeatCombo{nullptr};
    QWidget* m_oncePanel{nullptr};
    QWidget* m_weeklyPanel{nullptr};
    QWidget* m_monthlyPanel{nullptr};
    QWidget* m_yearlyPanel{nullptr};
    QWidget* m_intervalPanel{nullptr};
    QDateEdit* m_onceDate{nullptr};
    QList<QPushButton*> m_dayButtons;
    QSpinBox* m_monthDay{nullptr};
    QComboBox* m_yearMonth{nullptr};
    QSpinBox* m_yearDay{nullptr};
    QWidget* m_everyDaysPanel{nullptr};
    QSpinBox* m_everyDays{nullptr};
    QDateEdit* m_seriesStart{nullptr};
    QWidget* m_seriesRow{nullptr};
    QWidget* m_everyWeeksPanel{nullptr};
    QSpinBox* m_everyWeeks{nullptr};
    QWidget* m_nthPanel{nullptr};
    QComboBox* m_nthWeekday{nullptr};
    QComboBox* m_nthWeek{nullptr};
    QSpinBox* m_intervalMinutes{nullptr};
    QTimeEdit* m_intervalFrom{nullptr};
    QTimeEdit* m_intervalTo{nullptr};
    QComboBox* m_timeZoneCombo{nullptr};
    QComboBox* m_holidayPolicyCombo{nullptr};
    QLineEdit* m_exceptionsEdit{nullptr};
    QCheckBox* m_randomWindow{nullptr};
    QSpinBox* m_randomBefore{nullptr};
    QSpinBox* m_randomAfter{nullptr};
    QCheckBox* m_backupEnabled{nullptr};
    QSpinBox* m_backupAfterMinutes{nullptr};
    QCheckBox* m_persistent{nullptr};
    QListWidget* m_extraScheduleList{nullptr};
    QCheckBox* m_activeFromCheck{nullptr};
    QDateEdit* m_activeFromDate{nullptr};
    QCheckBox* m_activeToCheck{nullptr};
    QDateEdit* m_activeToDate{nullptr};
    QLabel* m_repeatSummary{nullptr};
    QLabel* m_nextRunLabel{nullptr};

    QCheckBox* m_soundEnabled{nullptr};
    QComboBox* m_soundCombo{nullptr};
    QPushButton* m_browseSound{nullptr};
    ThemedButton* m_previewButton{nullptr};
    QSlider* m_volumeSlider{nullptr};
    QLabel* m_volumeValue{nullptr};
    QCheckBox* m_fadeIn{nullptr};
    QSpinBox* m_fadeInSeconds{nullptr};
    QSpinBox* m_fadeOutSeconds{nullptr};
    QCheckBox* m_randomSound{nullptr};
    QComboBox* m_repeatSoundCombo{nullptr};
    QCheckBox* m_loopSound{nullptr};
    QSpinBox* m_maxSoundMinutes{nullptr};

    QCheckBox* m_snoozeEnabled{nullptr};
    QSpinBox* m_snoozeMinutes{nullptr};
    QSpinBox* m_snoozeMinutesAlt{nullptr};
    QCheckBox* m_snoozeDecrease{nullptr};
    QSpinBox* m_snoozeDecreaseStep{nullptr};
    QSpinBox* m_snoozeVolumeIncrease{nullptr};
    QComboBox* m_afterLastSnooze{nullptr};
    QSpinBox* m_maxSnoozes{nullptr};
    QCheckBox* m_popupEnabled{nullptr};
    QCheckBox* m_preAlertEnabled{nullptr};
    QSpinBox* m_preAlertMinutes{nullptr};
    QCheckBox* m_postSummary{nullptr};
    QCheckBox* m_postTasks{nullptr};
    QLineEdit* m_postMessage{nullptr};
    QLineEdit* m_postOpen{nullptr};
    QCheckBox* m_disableAfterDismiss{nullptr};
    QPlainTextEdit* m_noteEdit{nullptr};

    QString m_selectedIcon;
    QString m_selectedColor;
    QList<QPushButton*> m_iconButtons;
    QList<QPushButton*> m_colorButtons;

    QDialogButtonBox* m_buttons{nullptr};
    QScrollArea* m_scroll{nullptr};
};

} // namespace pac::ui
