#include "TodoPage.h"

#include "TodoDialog.h"
#include "core/Catalog.h"
#include "core/Schedule.h"
#include "core/Settings.h"
#include "core/Store.h"
#include "core/Theme.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QCheckBox>
#include <QDateTime>
#include <QEnterEvent>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QStackedWidget>
#include <QToolButton>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>
#include <functional>

namespace pac::ui {
namespace {

QString priorityPillStyle(int priority)
{
    const ThemeColors& colors = ThemeManager::instance().current();
    QColor color = colors.dim;
    if (priority >= 2) {
        color = colors.danger;
    } else if (priority == 1) {
        color = colors.accent;
    }
    return QStringLiteral("background: rgba(%1,%2,%3,28); color:%4; border-radius:9px;"
                          " padding: 1px 9px;")
        .arg(color.red())
        .arg(color.green())
        .arg(color.blue())
        .arg(color.name());
}

} // namespace

class TaskRow final : public QFrame {
public:
    using Callback = std::function<void()>;

    TaskRow(const Todo& todo, QWidget* parent = nullptr)
        : QFrame(parent)
        , m_id(todo.id)
    {
        setObjectName(QStringLiteral("Card"));
        setFrameShape(QFrame::NoFrame);
        auto* layout = new QHBoxLayout(this);
        m_layout = layout;

        m_check = new QCheckBox(this);
        m_check->setChecked(todo.done);
        m_check->setFixedSize(24, 24);
        m_check->setCursor(Qt::PointingHandCursor);
        m_check->setToolTip(todo.done ? tr("Вернуть в активные") : tr("Отметить выполненной"));
        connect(m_check, &QAbstractButton::clicked, this, [this] {
            if (m_onToggle) {
                m_onToggle();
            }
        });
        layout->addWidget(m_check);

        auto* textColumn = new QVBoxLayout;
        textColumn->setSpacing(0);
        m_title = new QLabel(this);
        QFont titleFont = m_title->font();
        titleFont.setPointSizeF(9.6);
        titleFont.setWeight(QFont::DemiBold);
        titleFont.setStrikeOut(todo.done);
        m_title->setFont(titleFont);
        m_title->setText(todo.title);
        m_title->setStyleSheet(QStringLiteral("color:%1;")
                                   .arg(todo.done ? ThemeManager::instance().current().dim.name()
                                                  : ThemeManager::instance().current().text.name()));
        textColumn->addWidget(m_title);
        m_meta = new QLabel(this);
        m_meta->setObjectName(QStringLiteral("Muted"));
        m_meta->setText(buildMeta(todo));
        textColumn->addWidget(m_meta);
        layout->addLayout(textColumn, 1);

        auto* priority = new QLabel(tr("%1").arg(catalog::priorityName(todo.priority)), this);
        QFont pillFont = priority->font();
        pillFont.setPointSizeF(7.6);
        priority->setFont(pillFont);
        priority->setStyleSheet(priorityPillStyle(todo.priority));
        priority->setFixedHeight(20);
        layout->addWidget(priority, 0, Qt::AlignVCenter);

        auto* buttons = new QHBoxLayout;
        buttons->setSpacing(1);
        m_edit = new ThemedToolButton(icons::Icon::Edit, tr("Изменить"), this);
        m_delete = new ThemedToolButton(icons::Icon::Delete, tr("Удалить"), this);
        m_delete->setGlyphColor(ThemeManager::instance().current().danger);
        for (auto* button : {m_edit, m_delete}) {
            button->setFixedSize(24, 24);
            button->setIconSizePx(14);
            buttons->addWidget(button);
        }
        layout->addLayout(buttons);

        connect(m_edit, &QToolButton::clicked, this, [this] {
            if (m_onEdit) {
                m_onEdit();
            }
        });
        connect(m_delete, &QToolButton::clicked, this, [this] {
            if (m_onDelete) {
                m_onDelete();
            }
        });
        if (!todo.notes.isEmpty()) {
            setToolTip(todo.notes);
        }
        connect(&Settings::instance(), &Settings::changed, this, [this] { applyDensity(); });
        applyDensity();
    }

    void applyDensity()
    {
        if (m_layout == nullptr) {
            return;
        }
        const int density = Settings::instance().rowDensity();
        m_layout->setContentsMargins(density == 0 ? 10 : (density == 2 ? 18 : 14),
                                     density == 0 ? 6 : (density == 2 ? 14 : 10),
                                     density == 0 ? 9 : (density == 2 ? 16 : 12),
                                     density == 0 ? 6 : (density == 2 ? 14 : 10));
        if (m_title != nullptr) {
            QFont titleFont = m_title->font();
            titleFont.setPointSizeF(density == 0 ? 9.2 : (density == 2 ? 10.2 : 9.6));
            m_title->setFont(titleFont);
        }
        updateGeometry();
        update();
    }

    QUuid todoId() const
    {
        return m_id;
    }

    void setCallbacks(Callback toggle, Callback edit, Callback remove)
    {
        m_onToggle = std::move(toggle);
        m_onEdit = std::move(edit);
        m_onDelete = std::move(remove);
    }

private:
    static QString buildMeta(const Todo& todo)
    {
        if (todo.done) {
            if (todo.completedAt.isValid()) {
                return TaskRow::tr("Выполнено %1")
                    .arg(schedule::dateText(todo.completedAt.date(), false));
            }
            return TaskRow::tr("Выполнено");
        }
        if (!todo.hasDue || !todo.dueDate.isValid()) {
            return TaskRow::tr("Без срока");
        }
        const QDateTime now = QDateTime::currentDateTime();
        const QDateTime due(todo.dueDate, todo.dueTime.isValid() ? todo.dueTime : QTime(23, 59));
        QString text = schedule::dateTimeText(due, now, Settings::instance().use24h());
        if (due < now) {
            text = TaskRow::tr("Просрочено · %1").arg(text);
        }
        if (!todo.notes.isEmpty()) {
            text += TaskRow::tr(" · есть заметка");
        }
        return text;
    }

    QUuid m_id;
    QHBoxLayout* m_layout{nullptr};
    QCheckBox* m_check{nullptr};
    QLabel* m_title{nullptr};
    QLabel* m_meta{nullptr};
    ThemedToolButton* m_edit{nullptr};
    ThemedToolButton* m_delete{nullptr};
    Callback m_onToggle;
    Callback m_onEdit;
    Callback m_onDelete;
};

TodoPage::TodoPage(Store& store, QWidget* parent)
    : PageWidget(parent)
    , m_store(store)
{
    buildUi();
    connect(&m_store, &Store::todosChanged, this, [this] {
        QTimer::singleShot(0, this, [this] {
            rebuildRows();
            updateEmptyState();
        });
    });
    const auto applyListDensity = [this] {
        if (m_rowsLayout != nullptr) {
            const int density = Settings::instance().rowDensity();
            m_rowsLayout->setSpacing(density == 0 ? 4 : (density == 2 ? 12 : 7));
        }
    };
    connect(&Settings::instance(), &Settings::changed, this, applyListDensity);
    applyListDensity();
    rebuildRows();
    updateEmptyState();
}

void TodoPage::buildUi()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(10);

    auto* toolbar = new QHBoxLayout;
    toolbar->setSpacing(6);
    auto* icon = new QLabel(this);
    icon->setPixmap(icons::pixmap(icons::Icon::Tasks, 22,
                                  ThemeManager::instance().current().success));
    toolbar->addWidget(icon);
    auto* title = new QLabel(tr("Задачи"), this);
    QFont titleFont = title->font();
    titleFont.setPointSizeF(12.0);
    titleFont.setWeight(QFont::DemiBold);
    title->setFont(titleFont);
    toolbar->addWidget(title);
    m_countLabel = new QLabel(this);
    m_countLabel->setObjectName(QStringLiteral("Muted"));
    toolbar->addWidget(m_countLabel);
    toolbar->addStretch(1);

    m_activeFilter = new QPushButton(tr("Активные"), this);
    m_completedFilter = new QPushButton(tr("Выполненные"), this);
    m_allFilter = new QPushButton(tr("Все"), this);
    auto* filterGroup = new QButtonGroup(this);
    filterGroup->setExclusive(true);
    for (auto* button : {m_activeFilter, m_completedFilter, m_allFilter}) {
        button->setCheckable(true);
        button->setMinimumHeight(27);
        filterGroup->addButton(button);
        toolbar->addWidget(button);
    }
    m_activeFilter->setChecked(true);
    connect(m_activeFilter, &QPushButton::clicked, this, [this] { setFilter(Filter::Active); });
    connect(m_completedFilter, &QPushButton::clicked, this, [this] { setFilter(Filter::Completed); });
    connect(m_allFilter, &QPushButton::clicked, this, [this] { setFilter(Filter::All); });

    auto* addButton = new ThemedButton(tr("Новая задача"), icons::Icon::Plus, this, true);
    addButton->setMinimumHeight(28);
    connect(addButton, &QPushButton::clicked, this, &TodoPage::createTask);
    toolbar->addWidget(addButton);
    layout->addLayout(toolbar);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto* rowsHost = new QWidget(scroll);
    rowsHost->setObjectName(QStringLiteral("Transparent"));
    m_rowsLayout = new QVBoxLayout(rowsHost);
    m_rowsLayout->setContentsMargins(0, 0, 6, 0);
    m_rowsLayout->setSpacing(7);
    m_rowsLayout->addStretch(1);
    scroll->setWidget(rowsHost);

    auto* empty = new QWidget(this);
    auto* emptyLayout = new QVBoxLayout(empty);
    emptyLayout->setContentsMargins(24, 24, 24, 24);
    emptyLayout->addStretch(1);
    auto* card = makeCard(empty);
    card->setMaximumWidth(500);
    auto* inner = new QVBoxLayout(card);
    inner->setContentsMargins(30, 28, 30, 28);
    inner->setSpacing(8);
    auto* emptyIcon = new QLabel(card);
    emptyIcon->setAlignment(Qt::AlignHCenter);
    emptyIcon->setPixmap(icons::pixmap(icons::Icon::Tasks, 44,
                                       ThemeManager::instance().current().success));
    inner->addWidget(emptyIcon);
    m_emptyTitle = makeLabel(QString(), QStringLiteral("PageTitle"), card);
    m_emptyTitle->setAlignment(Qt::AlignHCenter);
    inner->addWidget(m_emptyTitle);
    m_emptyHint = makeLabel(QString(), QStringLiteral("Muted"), card);
    m_emptyHint->setAlignment(Qt::AlignHCenter);
    m_emptyHint->setWordWrap(true);
    inner->addWidget(m_emptyHint);
    inner->addSpacing(4);
    m_emptyAction = new ThemedButton(tr("Новая задача"), icons::Icon::Plus, card, true);
    connect(m_emptyAction, &QPushButton::clicked, this, [this] {
        if (m_store.data().todos.empty()) {
            createTask();
        } else {
            setFilter(Filter::All);
        }
    });
    inner->addWidget(m_emptyAction, 0, Qt::AlignHCenter);
    emptyLayout->addWidget(card, 0, Qt::AlignHCenter);
    emptyLayout->addStretch(1);

    m_stack = new QStackedWidget(this);
    m_stack->setObjectName(QStringLiteral("Transparent"));
    m_stack->addWidget(scroll);
    m_stack->addWidget(empty);
    layout->addWidget(m_stack, 1);
}

void TodoPage::rebuildRows()
{
    for (auto* row : m_rows) {
        m_rowsLayout->removeWidget(row);
        delete row;
    }
    m_rows.clear();
    if (m_rowsLayout->count() == 0) {
        m_rowsLayout->addStretch(1);
    }

    std::vector<const Todo*> visible;
    for (const auto& todo : m_store.data().todos) {
        if (matchesFilter(todo)) {
            visible.push_back(&todo);
        }
    }
    std::sort(visible.begin(), visible.end(), [this](const Todo* left, const Todo* right) {
        if (m_filter == Filter::Completed) {
            return left->completedAt > right->completedAt;
        }
        auto key = [](const Todo& todo) {
            if (!todo.hasDue || !todo.dueDate.isValid()) {
                return QDateTime(QDate(2099, 12, 31), QTime(23, 59));
            }
            return QDateTime(todo.dueDate, todo.dueTime.isValid() ? todo.dueTime : QTime(23, 59));
        };
        if (m_filter == Filter::All && left->done != right->done) {
            return !left->done;
        }
        const QDateTime leftKey = key(*left);
        const QDateTime rightKey = key(*right);
        if (leftKey == rightKey) {
            if (left->priority != right->priority) {
                return left->priority > right->priority;
            }
            return left->title.localeAwareCompare(right->title) < 0;
        }
        if (left->done != right->done) {
            return !left->done;
        }
        return leftKey < rightKey;
    });

    for (const Todo* todo : visible) {
        auto* row = new TaskRow(*todo, m_rowsLayout->parentWidget());
        const QUuid id = todo->id;
        row->setCallbacks([this, id] { toggleDone(id); }, [this, id] { editTodo(id); },
                          [this, id] { removeTodo(id); });
        m_rowsLayout->insertWidget(m_rowsLayout->count() - 1, row);
        m_rows.append(row);
    }
    rebuildFilters();
}

void TodoPage::rebuildFilters()
{
    int active = 0;
    int completed = 0;
    for (const auto& todo : m_store.data().todos) {
        if (todo.done) {
            ++completed;
        } else {
            ++active;
        }
    }
    m_activeFilter->setText(tr("Активные  %1").arg(active));
    m_completedFilter->setText(tr("Выполненные  %1").arg(completed));
    m_allFilter->setText(tr("Все  %1").arg(active + completed));
    m_activeFilter->setChecked(m_filter == Filter::Active);
    m_completedFilter->setChecked(m_filter == Filter::Completed);
    m_allFilter->setChecked(m_filter == Filter::All);
    m_countLabel->setText(tr("%1 активных · %2 выполнено").arg(active).arg(completed));
}

void TodoPage::updateEmptyState()
{
    const bool empty = m_rows.isEmpty();
    m_stack->setCurrentIndex(empty ? 1 : 0);
    if (!empty) {
        return;
    }
    if (m_store.data().todos.empty()) {
        m_emptyTitle->setText(tr("Пока нет задач"));
        m_emptyHint->setText(tr("Запишите первое дело — срок и приоритет помогут "
                                "не потерять его из виду."));
        m_emptyAction->setText(tr("Новая задача"));
        m_emptyAction->setGlyph(icons::Icon::Plus);
    } else if (m_filter == Filter::Active) {
        m_emptyTitle->setText(tr("Всё сделано"));
        m_emptyHint->setText(tr("Активных задач не осталось. Отличная работа!"));
        m_emptyAction->setText(tr("Показать все"));
        m_emptyAction->setGlyph(icons::Icon::Tasks);
    } else if (m_filter == Filter::Completed) {
        m_emptyTitle->setText(tr("Выполненных пока нет"));
        m_emptyHint->setText(tr("Отметьте задачу выполненной, и она появится здесь."));
        m_emptyAction->setText(tr("Показать активные"));
        m_emptyAction->setGlyph(icons::Icon::List);
    } else {
        m_emptyTitle->setText(tr("Задач нет"));
        m_emptyHint->setText(tr("Добавьте первую задачу."));
        m_emptyAction->setText(tr("Новая задача"));
        m_emptyAction->setGlyph(icons::Icon::Plus);
    }
}

bool TodoPage::matchesFilter(const Todo& todo) const
{
    switch (m_filter) {
    case Filter::Active:
        return !todo.done;
    case Filter::Completed:
        return todo.done;
    case Filter::All:
        return true;
    }
    return true;
}

void TodoPage::setFilter(Filter filter)
{
    m_filter = filter;
    rebuildRows();
    updateEmptyState();
}

void TodoPage::createTask()
{
    createTaskImpl(QDate());
}

void TodoPage::createTaskOnDate(const QDate& date)
{
    createTaskImpl(date);
}

void TodoPage::editTodoById(const QUuid& id)
{
    editTodo(id);
}

void TodoPage::refreshAll()
{
    rebuildRows();
    updateEmptyState();
}

void TodoPage::createTaskImpl(const QDate& dueDate)
{
    Todo todo;
    todo.hasDue = dueDate.isValid();
    todo.priority = 1;
    if (dueDate.isValid()) {
        todo.dueDate = dueDate;
        todo.dueTime = QTime(18, 0);
    }
    TodoDialog dialog(todo, true, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    Todo created = dialog.result();
    m_store.data().todos.push_back(created);
    m_store.commit(Domain::Todos);
    if (m_filter == Filter::Completed) {
        setFilter(Filter::Active);
    }
    rebuildRows();
    updateEmptyState();
}

void TodoPage::toggleDone(const QUuid& id)
{
    Todo* todo = m_store.findTodo(id);
    if (todo == nullptr) {
        return;
    }
    todo->done = !todo->done;
    todo->completedAt = todo->done ? QDateTime::currentDateTime() : QDateTime();
    m_store.commit(Domain::Todos);
}

void TodoPage::editTodo(const QUuid& id)
{
    Todo* todo = m_store.findTodo(id);
    if (todo == nullptr) {
        return;
    }
    TodoDialog dialog(*todo, false, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    *todo = dialog.result();
    m_store.commit(Domain::Todos);
}

void TodoPage::removeTodo(const QUuid& id)
{
    Todo* todo = m_store.findTodo(id);
    if (todo == nullptr) {
        return;
    }
    QMessageBox box(QMessageBox::Question, tr("Удалить задачу"),
                    tr("Удалить задачу «%1»?").arg(todo->title),
                    QMessageBox::Yes | QMessageBox::No, this);
    box.setDefaultButton(QMessageBox::No);
    if (box.exec() != QMessageBox::Yes) {
        return;
    }
    const auto it = std::find_if(m_store.data().todos.begin(), m_store.data().todos.end(),
                                 [&id](const Todo& entry) { return entry.id == id; });
    if (it == m_store.data().todos.end()) {
        return;
    }
    m_store.data().todos.erase(it);
    m_store.commit(Domain::Todos);
}

void TodoPage::onActivated()
{
    rebuildRows();
    updateEmptyState();
}

void TodoPage::themeUpdated()
{
    rebuildRows();
    updateEmptyState();
}

void TodoPage::resizeEvent(QResizeEvent* event)
{
    PageWidget::resizeEvent(event);
    if (m_countLabel != nullptr) {
        m_countLabel->setVisible(width() >= 760);
    }
}

} // namespace pac::ui
