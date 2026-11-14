#pragma once

#include "UiKit.h"
#include "core/Types.h"

#include <QDate>
#include <QUuid>

class QComboBox;
class QLabel;
class QLineEdit;
class QScrollArea;
class QStackedWidget;
class QTableView;
class QVBoxLayout;
class QWidget;

namespace pac {
class Store;
}

namespace pac::ui {

class AlarmDelegate;
class AlarmTableModel;

class AlarmsPage final : public PageWidget {
    Q_OBJECT

public:
    explicit AlarmsPage(Store& store, QWidget* parent = nullptr);

    void refreshClock(const QDateTime& now) override;

public slots:
    void createAlarm();
    void createGroup();
    void editSelected();
    void duplicateSelected();
    void deleteSelected();
    void toggleSelected();
    void collapseAll();
    void setSearch(const QString& text);
    void showHolidays();
    void applyDensity();
    void reload();
    void applyThemeMode() override;
    void focusSearch();
    void createAlarmOnDate(const QDate& date);
    void editAlarmById(const QUuid& id);

signals:
    void searchResetRequested();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void onActivated() override;
    void themeUpdated() override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void buildUi();
    void createAlarmImpl(const QDate& presetDate);
    void editAlarm(const QUuid& id);
    void duplicateAlarm(const QUuid& id);
    void deleteAlarm(const QUuid& id);
    void showContextMenu(const QPoint& viewportPosition);
    void showCardContextMenu(const QUuid& id, const QPoint& globalPosition);
    void renameGroup(const QString& oldName);
    void deleteGroup(const QString& name);
    void resetFilters();
    void updateGroupCombo();
    void updateGroupIcons();
    void updateLayoutRows();
    void updateCountLabel();
    void updateEmptyState();
    void rebuildCards();
    QWidget* makeAlarmCard(const Alarm& alarm);
    void updateCollapseButton();
    void updatePauseButton();
    void showPauseMenu();
    void reloadSelecting(const QUuid& id);
    void selectAlarm(const QUuid& id);
    void showMessage(const QString& title, const QString& text);
    [[nodiscard]] QStringList groupNames() const;
    [[nodiscard]] int currentRow() const;
    [[nodiscard]] QUuid selectedAlarmId() const;
    [[nodiscard]] QString defaultGroup() const;

    Store& m_store;
    AlarmTableModel* m_model{nullptr};
    AlarmDelegate* m_delegate{nullptr};
    QTableView* m_table{nullptr};
    QStackedWidget* m_listStack{nullptr};
    QWidget* m_toolbar{nullptr};
    QScrollArea* m_cardScroll{nullptr};
    QWidget* m_cardHost{nullptr};
    QVBoxLayout* m_cardLayout{nullptr};
    QLineEdit* m_fluentSearch{nullptr};
    QComboBox* m_groupCombo{nullptr};
    QComboBox* m_sortCombo{nullptr};
    QLabel* m_countLabel{nullptr};
    QLabel* m_emptyIcon{nullptr};
    QLabel* m_emptyTitle{nullptr};
    QLabel* m_emptyHint{nullptr};
    ThemedButton* m_emptyAction{nullptr};
    ThemedToolButton* m_resetFilterButton{nullptr};
    ThemedToolButton* m_collapseButton{nullptr};
    ThemedToolButton* m_enableAllButton{nullptr};
    ThemedToolButton* m_disableAllButton{nullptr};
    ThemedToolButton* m_pauseButton{nullptr};
    bool m_updatingGroups{false};
    bool m_allCollapsed{false};
    bool m_fluent{false};
};

} // namespace pac::ui
