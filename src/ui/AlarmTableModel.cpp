#include "AlarmTableModel.h"

#include "core/Schedule.h"
#include "core/Settings.h"
#include "core/Fonts.h"

#include <QColor>
#include <QFont>

#include <algorithm>

namespace pac::ui {
namespace {

[[nodiscard]] QDateTime sortKey(const Alarm& alarm, const QDateTime& now)
{
    const QDateTime next = schedule::nextOccurrence(alarm, now);
    if (next.isValid()) {
        return next;
    }
    return QDateTime(now.date().addYears(100), QTime(0, 0));
}

void enableAlarm(Alarm& alarm)
{
    alarm.enabled = true;
    if (alarm.repeat != RepeatKind::Once) {
        return;
    }
    const QDateTime candidate(QDate::currentDate(), alarm.time);
    if (candidate <= QDateTime::currentDateTime()) {
        alarm.onceDate = QDate::currentDate().addDays(1);
    } else if (!alarm.onceDate.isValid() || alarm.onceDate < QDate::currentDate()) {
        alarm.onceDate = QDate::currentDate();
    }
}

} // namespace

AlarmTableModel::AlarmTableModel(Store& store, QObject* parent)
    : QAbstractTableModel(parent)
    , m_store(store)
{
    m_now = QDateTime::currentDateTime();
    rebuild();
}

int AlarmTableModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_rows.size();
}

int AlarmTableModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant AlarmTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size()) {
        return {};
    }
    const Row& row = m_rows.at(index.row());
    const auto& data = m_store.data();

    if (row.group) {
        const AlarmGroup& group = data.groups.at(static_cast<std::size_t>(row.groupIndex));
        switch (role) {
        case IsGroupRole:
            return true;
        case CountRole:
            return row.count;
        case CollapsedRole:
            return group.collapsed;
        case Qt::DisplayRole:
            return index.column() == Enabled ? group.name : QVariant();
        case Qt::ToolTipRole:
            return QStringLiteral("%1 · %2").arg(group.name).arg(
                tr("Будильников: %1").arg(row.count));
        case Qt::TextAlignmentRole:
            return static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter);
        default:
            return {};
        }
    }

    const Alarm& alarm = data.alarms.at(static_cast<std::size_t>(row.alarmIndex));
    const bool use24h = Settings::instance().use24h();
    const bool snoozed = alarm.snoozeUntil.isValid() && alarm.snoozeUntil > m_now;

    switch (role) {
    case IsGroupRole:
        return false;
    case AlarmIdRole:
        return alarm.id;
    case EnabledRole:
        return alarm.enabled;
    case AlarmColorRole:
        return QColor(alarm.color);
    case AlarmTimeRole:
        return alarm.time;
    case AlarmIconRole:
        return alarm.icon;
    case CountRole:
        return row.count;
    case CollapsedRole:
        return false;
    case AlternateRole:
        return index.row() % 2;
    case SnoozedRole:
        return snoozed;
    case Qt::DisplayRole:
        switch (index.column()) {
        case Countdown: {
            if (!alarm.enabled) {
                return tr("выключен");
            }
            const QDateTime next = schedule::nextOccurrence(alarm, m_now);
            if (!next.isValid()) {
                return QStringLiteral("—");
            }
            const qint64 seconds = std::max<qint64>(0, m_now.secsTo(next));
            const QString value = schedule::countdownText(seconds);
            return snoozed ? tr("отложен · %1").arg(value) : value;
        }
        case Time:
            return schedule::timeText(alarm.time, use24h);
        case Repeat:
            return schedule::repeatSummary(alarm);
        case Emoji:
            return alarm.icon;
        case Label:
            return alarm.label.isEmpty() ? tr("Без названия") : alarm.label;
        default:
            return {};
        }
    case Qt::CheckStateRole:
        if (index.column() == Enabled) {
            return alarm.enabled ? Qt::Checked : Qt::Unchecked;
        }
        return {};
    case Qt::ToolTipRole: {
        QString text = QStringLiteral("<b>%1</b><br>")
                           .arg(alarm.label.isEmpty() ? tr("Без названия") : alarm.label);
        text += schedule::repeatSummary(alarm);
        if (alarm.enabled) {
            const QDateTime next = schedule::nextOccurrence(alarm, m_now);
            if (next.isValid()) {
                text += QStringLiteral("<br>") + tr("Следующий: %1")
                                                  .arg(schedule::dateTimeText(next, m_now, use24h));
            }
        } else {
            text += QStringLiteral("<br>") + tr("Будильник выключен");
        }
        if (!alarm.note.isEmpty()) {
            text += QStringLiteral("<br><i>") + alarm.note.toHtmlEscaped() + QStringLiteral("</i>");
        }
        return text;
    }
    case Qt::TextAlignmentRole:
        switch (index.column()) {
        case Dial:
        case Enabled:
        case Emoji:
            return static_cast<int>(Qt::AlignCenter);
        default:
            return static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter);
        }
    case Qt::FontRole:
        if (index.column() == Time) {
            return fonts::digitFont(10.5, QFont::DemiBold);
        }
        if (index.column() == Label) {
            return fonts::uiFont(9.3, QFont::DemiBold);
        }
        return {};
    default:
        return {};
    }
}

QVariant AlarmTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return {};
    }
    switch (section) {
    case Enabled:
        return tr("Вкл.");
    case Dial:
        return tr("Часы");
    case Countdown:
        return tr("Осталось");
    case Time:
        return tr("Время");
    case Repeat:
        return tr("Повторение");
    case Emoji:
        return tr("Значок");
    case Label:
        return tr("Название");
    default:
        return {};
    }
}

Qt::ItemFlags AlarmTableModel::flags(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }
    Qt::ItemFlags result = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if (index.column() == Enabled && !isGroupRow(index.row())) {
        result |= Qt::ItemIsUserCheckable;
    }
    return result;
}

bool AlarmTableModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::CheckStateRole || index.column() != Enabled
        || isGroupRow(index.row())) {
        return false;
    }
    Q_UNUSED(value)
    toggleAlarm(index.row());
    return true;
}

void AlarmTableModel::reload()
{
    beginResetModel();
    m_now = QDateTime::currentDateTime();
    rebuild();
    endResetModel();
}

void AlarmTableModel::setSearch(const QString& text)
{
    const QString trimmed = text.trimmed();
    if (m_search == trimmed) {
        return;
    }
    m_search = trimmed;
    reload();
}

void AlarmTableModel::setGroupFilter(const QString& group)
{
    if (m_groupFilter == group) {
        return;
    }
    m_groupFilter = group;
    reload();
}

void AlarmTableModel::setSortMode(SortMode mode)
{
    if (m_sortMode == mode) {
        return;
    }
    m_sortMode = mode;
    reload();
}

void AlarmTableModel::tick(const QDateTime& now)
{
    m_now = now;
    for (int row = 0; row < m_rows.size(); ++row) {
        if (!m_rows.at(row).group) {
            const QModelIndex cell = index(row, Countdown);
            emit dataChanged(cell, cell, {Qt::DisplayRole});
        }
    }
}

void AlarmTableModel::toggleGroup(int row)
{
    if (row < 0 || row >= m_rows.size() || !m_rows.at(row).group) {
        return;
    }
    auto& group = m_store.data().groups.at(static_cast<std::size_t>(m_rows.at(row).groupIndex));
    group.collapsed = !group.collapsed;
    m_store.commit(Domain::Alarms);
    reload();
}

void AlarmTableModel::toggleAlarm(int row)
{
    if (row < 0 || row >= m_rows.size() || m_rows.at(row).group) {
        return;
    }
    auto& alarm = m_store.data().alarms.at(static_cast<std::size_t>(m_rows.at(row).alarmIndex));
    if (alarm.enabled) {
        alarm.enabled = false;
        alarm.snoozeUntil = QDateTime();
        alarm.snoozeCount = 0;
    } else {
        enableAlarm(alarm);
    }
    m_store.commit(Domain::Alarms);
    const QModelIndex first = index(row, 0);
    const QModelIndex last = index(row, ColumnCount - 1);
    emit dataChanged(first, last);
}

void AlarmTableModel::setAllEnabled(bool enabled)
{
    bool changed = false;
    for (auto& alarm : m_store.data().alarms) {
        if (alarm.enabled == enabled) {
            continue;
        }
        if (enabled) {
            enableAlarm(alarm);
        } else {
            alarm.enabled = false;
            alarm.snoozeUntil = QDateTime();
            alarm.snoozeCount = 0;
        }
        changed = true;
    }
    if (!changed) {
        return;
    }
    m_store.commit(Domain::Alarms);
    reload();
}

void AlarmTableModel::setGroupEnabled(int row, bool enabled)
{
    if (row < 0 || row >= m_rows.size() || !m_rows.at(row).group) {
        return;
    }
    const auto& group = m_store.data().groups.at(
        static_cast<std::size_t>(m_rows.at(row).groupIndex));
    bool changed = false;
    for (auto& alarm : m_store.data().alarms) {
        if (alarm.group != group.name || alarm.enabled == enabled) {
            continue;
        }
        if (enabled) {
            enableAlarm(alarm);
        } else {
            alarm.enabled = false;
            alarm.snoozeUntil = QDateTime();
            alarm.snoozeCount = 0;
        }
        changed = true;
    }
    if (!changed) {
        return;
    }
    m_store.commit(Domain::Alarms);
    reload();
}

void AlarmTableModel::collapseAll(bool collapsed)
{
    bool changed = false;
    for (auto& group : m_store.data().groups) {
        if (group.collapsed != collapsed) {
            group.collapsed = collapsed;
            changed = true;
        }
    }
    if (!changed) {
        return;
    }
    m_store.commit(Domain::Alarms);
    reload();
}

bool AlarmTableModel::isGroupRow(int row) const
{
    return row >= 0 && row < m_rows.size() && m_rows.at(row).group;
}

const Alarm* AlarmTableModel::alarmAtRow(int row) const
{
    if (row < 0 || row >= m_rows.size() || m_rows.at(row).group) {
        return nullptr;
    }
    return &m_store.data().alarms.at(static_cast<std::size_t>(m_rows.at(row).alarmIndex));
}

QUuid AlarmTableModel::alarmIdAtRow(int row) const
{
    const Alarm* alarm = alarmAtRow(row);
    return alarm != nullptr ? alarm->id : QUuid();
}

int AlarmTableModel::rowForAlarm(const QUuid& id) const
{
    for (int row = 0; row < m_rows.size(); ++row) {
        const Row& entry = m_rows.at(row);
        if (!entry.group
            && m_store.data().alarms.at(static_cast<std::size_t>(entry.alarmIndex)).id == id) {
            return row;
        }
    }
    return -1;
}

bool AlarmTableModel::hasAlarms() const
{
    return !m_store.data().alarms.empty();
}

bool AlarmTableModel::isFiltered() const
{
    return !m_search.isEmpty() || !m_groupFilter.isEmpty();
}

QString AlarmTableModel::groupFilter() const
{
    return m_groupFilter;
}

QString AlarmTableModel::search() const
{
    return m_search;
}

AlarmTableModel::SortMode AlarmTableModel::sortMode() const
{
    return m_sortMode;
}

void AlarmTableModel::rebuild()
{
    m_rows.clear();
    const auto& data = m_store.data();

    for (int groupIndex = 0; groupIndex < static_cast<int>(data.groups.size()); ++groupIndex) {
        const AlarmGroup& group = data.groups.at(static_cast<std::size_t>(groupIndex));
        if (!m_groupFilter.isEmpty() && m_groupFilter != group.name) {
            continue;
        }

        QVector<int> alarmIndices;
        for (int i = 0; i < static_cast<int>(data.alarms.size()); ++i) {
            const Alarm& alarm = data.alarms.at(static_cast<std::size_t>(i));
            if (alarm.group == group.name && matches(alarm)) {
                alarmIndices.append(i);
            }
        }
        if (alarmIndices.isEmpty()) {
            if (m_search.isEmpty() && !m_groupFilter.isEmpty() && m_groupFilter == group.name) {
                Row emptyGroup;
                emptyGroup.group = true;
                emptyGroup.groupIndex = groupIndex;
                emptyGroup.count = 0;
                m_rows.append(emptyGroup);
            }
            continue;
        }
        std::stable_sort(alarmIndices.begin(), alarmIndices.end(), [&data, this](int left, int right) {
            const Alarm& a = data.alarms.at(static_cast<std::size_t>(left));
            const Alarm& b = data.alarms.at(static_cast<std::size_t>(right));
            switch (m_sortMode) {
            case SortMode::Time:
                if (a.time != b.time) {
                    return a.time < b.time;
                }
                break;
            case SortMode::Name:
                break;
            case SortMode::Enabled:
                if (a.enabled != b.enabled) {
                    return a.enabled;
                }
                if (a.time != b.time) {
                    return a.time < b.time;
                }
                break;
            case SortMode::NextRun: {
                const QDateTime leftKey = sortKey(a, m_now);
                const QDateTime rightKey = sortKey(b, m_now);
                if (leftKey != rightKey) {
                    return leftKey < rightKey;
                }
                break;
            }
            }
            return a.label.localeAwareCompare(b.label) < 0;
        });

        Row groupRow;
        groupRow.group = true;
        groupRow.groupIndex = groupIndex;
        groupRow.count = alarmIndices.size();
        m_rows.append(groupRow);

        if (group.collapsed && m_search.isEmpty()) {
            continue;
        }
        for (const int alarmIndex : alarmIndices) {
            Row alarmRow;
            alarmRow.group = false;
            alarmRow.groupIndex = groupIndex;
            alarmRow.alarmIndex = alarmIndex;
            alarmRow.count = alarmIndices.size();
            m_rows.append(alarmRow);
        }
    }
}

bool AlarmTableModel::matches(const Alarm& alarm) const
{
    if (m_search.isEmpty()) {
        return true;
    }
    return alarm.label.contains(m_search, Qt::CaseInsensitive)
           || alarm.note.contains(m_search, Qt::CaseInsensitive)
           || alarm.group.contains(m_search, Qt::CaseInsensitive)
           || schedule::repeatSummary(alarm).contains(m_search, Qt::CaseInsensitive)
           || alarm.icon.contains(m_search);
}

} // namespace pac::ui
