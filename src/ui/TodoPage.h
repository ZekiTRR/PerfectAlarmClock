#pragma once

#include "UiKit.h"
#include "core/Types.h"

#include <QVector>
#include <QUuid>

class QLabel;
class QPushButton;
class QStackedWidget;
class QVBoxLayout;

namespace pac {
class Store;
}

namespace pac::ui {

class TaskRow;

class TodoPage final : public PageWidget {
    Q_OBJECT

public:
    explicit TodoPage(Store& store, QWidget* parent = nullptr);

    enum class Filter {
        Active,
        Completed,
        All
    };

public slots:
    void createTask();
    void createTaskOnDate(const QDate& date);
    void editTodoById(const QUuid& id);
    void refreshAll();

protected:
    void onActivated() override;
    void themeUpdated() override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void buildUi();
    void createTaskImpl(const QDate& dueDate);
    void rebuildRows();
    void rebuildFilters();
    void updateEmptyState();
    void setFilter(Filter filter);
    void toggleDone(const QUuid& id);
    void editTodo(const QUuid& id);
    void removeTodo(const QUuid& id);
    [[nodiscard]] bool matchesFilter(const Todo& todo) const;

    Store& m_store;
    Filter m_filter{Filter::Active};
    QStackedWidget* m_stack{nullptr};
    QVBoxLayout* m_rowsLayout{nullptr};
    QVector<TaskRow*> m_rows;
    QLabel* m_countLabel{nullptr};
    QPushButton* m_activeFilter{nullptr};
    QPushButton* m_completedFilter{nullptr};
    QPushButton* m_allFilter{nullptr};
    QLabel* m_emptyTitle{nullptr};
    QLabel* m_emptyHint{nullptr};
    ThemedButton* m_emptyAction{nullptr};
};

} // namespace pac::ui
