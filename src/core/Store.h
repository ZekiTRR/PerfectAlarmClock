#pragma once

#include "Types.h"

#include <QObject>
#include <QTimer>

#include <vector>

namespace pac {

enum class Domain {
    Alarms,
    Timers,
    Todos,
    Clocks,
    Pomodoro,
    All
};

class Store final : public QObject {
    Q_OBJECT

public:
    explicit Store(QObject* parent = nullptr);
    ~Store() override;

    Store(const Store&) = delete;
    Store& operator=(const Store&) = delete;
    Store(Store&&) = delete;
    Store& operator=(Store&&) = delete;

    void load();
    void saveNow();
    void commit(Domain domain = Domain::All);

    [[nodiscard]] AppData& data() noexcept;
    [[nodiscard]] const AppData& data() const noexcept;

    [[nodiscard]] Alarm* findAlarm(const QUuid& id);
    [[nodiscard]] const Alarm* findAlarm(const QUuid& id) const;
    [[nodiscard]] CountdownTimer* findTimer(const QUuid& id);
    [[nodiscard]] const CountdownTimer* findTimer(const QUuid& id) const;
    [[nodiscard]] Todo* findTodo(const QUuid& id);

    void ensureGroup(const QString& name);
    [[nodiscard]] QString dataFilePath() const;

signals:
    void alarmsChanged();
    void timersChanged();
    void todosChanged();
    void clocksChanged();
    void pomodoroChanged();
    void changed();

private:
    void seedDemoData();
    void seedDefaultClocks();
    void normalize();
    void scheduleSave();

    AppData m_data;
    QTimer m_saveTimer;
    bool m_loaded{false};
};

} // namespace pac
