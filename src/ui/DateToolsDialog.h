#pragma once

#include <QDialog>

class QComboBox;
class QDateEdit;
class QDateTimeEdit;
class QHBoxLayout;
class QLabel;
class QLineEdit;
class QListWidget;
class QSpinBox;
class QStackedWidget;
class QTimer;
class QTimeEdit;
class QVBoxLayout;

namespace pac::ui {

class DateToolsDialog final : public QDialog {
    Q_OBJECT

public:
    explicit DateToolsDialog(QWidget* parent = nullptr);

private:
    void buildUi();
    void buildNavigation(QHBoxLayout* content);
    void buildDatePages();
    void buildTimePages();
    void addNavSection(const QString& title);
    void addToolPage(const QString& navTitle, QWidget* page);
    void connectInputs();
    void recalculateDates();
    void recalculateTime();
    void updateLive();

    QListWidget* m_nav{nullptr};
    QStackedWidget* m_pages{nullptr};
    QLabel* m_headerDate{nullptr};
    QLabel* m_headerClock{nullptr};

    // Date pages
    QDateEdit* m_diffFrom{nullptr};
    QDateEdit* m_diffTo{nullptr};
    QLabel* m_diffResult{nullptr};
    QDateEdit* m_addBase{nullptr};
    QSpinBox* m_addAmount{nullptr};
    QComboBox* m_addUnit{nullptr};
    QLabel* m_addResult{nullptr};
    QDateEdit* m_bizBase{nullptr};
    QSpinBox* m_bizDays{nullptr};
    QLabel* m_bizResult{nullptr};
    QDateEdit* m_infoDate{nullptr};
    QLabel* m_infoResult{nullptr};
    QDateEdit* m_birthDate{nullptr};
    QDateEdit* m_ageDate{nullptr};
    QLabel* m_ageResult{nullptr};
    QDateTimeEdit* m_eventAt{nullptr};
    QLabel* m_eventResult{nullptr};
    QDateTimeEdit* m_countdownAt{nullptr};
    QLabel* m_countdownResult{nullptr};
    QDateEdit* m_rangeFrom{nullptr};
    QDateEdit* m_rangeTo{nullptr};
    QLabel* m_rangeResult{nullptr};

    // Time pages
    QDateTimeEdit* m_momentA{nullptr};
    QDateTimeEdit* m_momentB{nullptr};
    QComboBox* m_momentZoneA{nullptr};
    QComboBox* m_momentZoneB{nullptr};
    QLabel* m_momentResult{nullptr};

    QDateTimeEdit* m_convertAt{nullptr};
    QComboBox* m_convertFrom{nullptr};
    QComboBox* m_convertTo{nullptr};
    QLabel* m_convertResult{nullptr};

    QLineEdit* m_unixInput{nullptr};
    QComboBox* m_unixZone{nullptr};
    QLabel* m_unixResult{nullptr};
    QDateTimeEdit* m_unixDate{nullptr};

    QComboBox* m_overlapZoneA{nullptr};
    QComboBox* m_overlapZoneB{nullptr};
    QTimeEdit* m_overlapStartA{nullptr};
    QTimeEdit* m_overlapEndA{nullptr};
    QTimeEdit* m_overlapStartB{nullptr};
    QTimeEdit* m_overlapEndB{nullptr};
    QDateEdit* m_overlapDate{nullptr};
    QLabel* m_overlapResult{nullptr};

    QLabel* m_nowClock{nullptr};
    QLabel* m_nowDate{nullptr};
    QLabel* m_nowUtc{nullptr};
    QLabel* m_nowIso{nullptr};
    QTimer* m_ticker{nullptr};
};

} // namespace pac::ui
