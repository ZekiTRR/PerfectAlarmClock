#pragma once

#include "core/Store.h"

#include <QAbstractTableModel>
#include <QDateTime>
#include <QVector>

namespace pac::ui {

class AlarmTableModel final : public QAbstractTableModel {
    Q_OBJECT

public:
    enum Column {
        Enabled = 0,
        Dial,
        Countdown,
        Time,
        Repeat,
        Emoji,
        Label,
        ColumnCount
    };

    enum class SortMode {
        NextRun,
        Time,
        Name,
        Enabled
    };

    enum Role {
        IsGroupRole = Qt::UserRole + 1,
        AlarmIdRole,
        EnabledRole,
        AlarmColorRole,
        AlarmTimeRole,
        AlarmIconRole,
        CountRole,
        CollapsedRole,
        AlternateRole,
        SnoozedRole
    };

    explicit AlarmTableModel(Store& store, QObject* parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation,
                                      int role = Qt::DisplayRole) const override;
    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

    void reload();
    void setSearch(const QString& text);
    void setGroupFilter(const QString& group);
    void setSortMode(SortMode mode);
    void tick(const QDateTime& now);
    void toggleGroup(int row);
    void toggleAlarm(int row);
    void collapseAll(bool collapsed);
    void setAllEnabled(bool enabled);
    void setGroupEnabled(int row, bool enabled);

    [[nodiscard]] bool isGroupRow(int row) const;
    [[nodiscard]] const Alarm* alarmAtRow(int row) const;
    [[nodiscard]] QUuid alarmIdAtRow(int row) const;
    [[nodiscard]] int rowForAlarm(const QUuid& id) const;
    [[nodiscard]] bool hasAlarms() const;
    [[nodiscard]] bool isFiltered() const;
    [[nodiscard]] QString groupFilter() const;
    [[nodiscard]] QString search() const;
    [[nodiscard]] SortMode sortMode() const;

private:
    struct Row {
        bool group{false};
        int groupIndex{-1};
        int alarmIndex{-1};
        int count{0};
    };

    void rebuild();
    [[nodiscard]] bool matches(const Alarm& alarm) const;

    Store& m_store;
    QVector<Row> m_rows;
    QString m_search;
    QString m_groupFilter;
    SortMode m_sortMode{SortMode::NextRun};
    QDateTime m_now;
};

} // namespace pac::ui
