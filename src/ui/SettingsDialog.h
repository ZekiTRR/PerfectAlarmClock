#pragma once

#include <QDialog>
#include <QString>
#include <QVector>

class QAbstractButton;
class QCheckBox;
class QComboBox;
class QFontComboBox;
class QLineEdit;
class QListWidget;
class QSpinBox;
class QStackedWidget;
class QVBoxLayout;
class QWidget;

namespace pac::ui {

class SettingsDialog final : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget* parent = nullptr);

private:
    void buildUi();
    void readSettings();
    void updateThemeChecks();
    void resetToDefaults();
    void applySettings();
    void makeBackup();

    QWidget* makePage(const QString& title, QVBoxLayout** contentLayout);
    void buildGeneralPage();
    void buildAppearancePage();
    void buildAlarmsPage();
    void buildTimersPage();
    void buildNotificationsPage();
    void buildCalendarPage();
    void buildDataPage();

    QListWidget* m_nav{nullptr};
    QStackedWidget* m_pages{nullptr};
    QString m_themeId;
    QVector<QAbstractButton*> m_themeButtons;

    QComboBox* m_timeFormat{nullptr};
    QComboBox* m_firstDay{nullptr};
    QCheckBox* m_secondsTitle{nullptr};
    QCheckBox* m_secondsClocks{nullptr};
    QCheckBox* m_autostart{nullptr};
    QCheckBox* m_startMinimized{nullptr};
    QCheckBox* m_closeToTray{nullptr};
    QCheckBox* m_minimizeToTray{nullptr};

    QCheckBox* m_showNav{nullptr};
    QCheckBox* m_showStatus{nullptr};
    QCheckBox* m_showGrid{nullptr};
    QCheckBox* m_alternateRows{nullptr};
    QComboBox* m_density{nullptr};
    QFontComboBox* m_uiFontCombo{nullptr};
    QFontComboBox* m_digitFontCombo{nullptr};
    QString m_uiFontFile;
    QString m_digitFontFile;

    QComboBox* m_alarmSound{nullptr};
    QSpinBox* m_alarmVolume{nullptr};
    QCheckBox* m_alarmLoop{nullptr};
    QSpinBox* m_alarmMaxSound{nullptr};
    QCheckBox* m_alarmPopup{nullptr};
    QSpinBox* m_snoozeMinutes{nullptr};
    QSpinBox* m_maxSnoozes{nullptr};
    QSpinBox* m_preAlert{nullptr};

    QSpinBox* m_timerMinutes{nullptr};
    QComboBox* m_timerSound{nullptr};
    QSpinBox* m_timerVolume{nullptr};
    QCheckBox* m_timerLoop{nullptr};
    QCheckBox* m_timerPopup{nullptr};

    QCheckBox* m_preAlerts{nullptr};
    QCheckBox* m_muted{nullptr};

    QCheckBox* m_calendarAlarms{nullptr};
    QCheckBox* m_calendarTasks{nullptr};
    QCheckBox* m_weekNumbers{nullptr};

    QLineEdit* m_dataPath{nullptr};
    QLineEdit* m_settingsPath{nullptr};
};

} // namespace pac::ui
