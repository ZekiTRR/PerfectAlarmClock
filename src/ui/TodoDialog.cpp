#include "TodoDialog.h"

#include "UiKit.h"
#include "core/Settings.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStyle>
#include <QTimeEdit>
#include <QVBoxLayout>

namespace pac::ui {

TodoDialog::TodoDialog(const Todo& todo, bool isNew, QWidget* parent)
    : QDialog(parent)
    , m_todo(todo)
    , m_isNew(isNew)
{
    setWindowTitle(isNew ? tr("Новая задача") : tr("Изменить задачу"));
    setMinimumWidth(560);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(14, 14, 14, 12);
    root->setSpacing(10);

    auto* header = makeCard(this);
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(16, 14, 16, 14);
    headerLayout->setSpacing(12);
    auto* icon = new QLabel(header);
    icon->setPixmap(emojiPixmap(QStringLiteral("📝"), 28));
    icon->setFixedSize(48, 48);
    icon->setAlignment(Qt::AlignCenter);
    icon->setStyleSheet(QStringLiteral("background: rgba(76,154,98,36); border-radius: 10px;"));
    headerLayout->addWidget(icon);
    m_title = new QLineEdit(header);
    m_title->setPlaceholderText(tr("Что нужно сделать?"));
    QFont titleFont = m_title->font();
    titleFont.setPointSizeF(12.0);
    titleFont.setWeight(QFont::DemiBold);
    m_title->setFont(titleFont);
    m_title->setText(todo.title);
    headerLayout->addWidget(m_title, 1);
    root->addWidget(header);

    auto* dueBox = new QGroupBox(tr("Срок"), this);
    auto* dueLayout = new QVBoxLayout(dueBox);
    dueLayout->setContentsMargins(12, 10, 12, 12);
    dueLayout->setSpacing(8);
    m_hasDue = new QCheckBox(tr("Указать срок выполнения"), dueBox);
    m_hasDue->setChecked(todo.hasDue);
    dueLayout->addWidget(m_hasDue);
    auto* dueRow = new QHBoxLayout;
    dueRow->setSpacing(8);
    m_dueDate = new QDateEdit(dueBox);
    m_dueDate->setCalendarPopup(true);
    m_dueDate->setDisplayFormat(QStringLiteral("dd.MM.yyyy"));
    m_dueDate->setDate(todo.hasDue && todo.dueDate.isValid() ? todo.dueDate
                                                              : QDate::currentDate());
    m_dueTime = new QTimeEdit(dueBox);
    m_dueTime->setDisplayFormat(Settings::instance().use24h() ? QStringLiteral("HH:mm")
                                                               : QStringLiteral("h:mm AP"));
    m_dueTime->setTime(todo.hasDue && todo.dueTime.isValid() ? todo.dueTime : QTime(18, 0));
    dueRow->addWidget(m_dueDate);
    dueRow->addWidget(m_dueTime);
    dueRow->addStretch(1);
    dueLayout->addLayout(dueRow);
    root->addWidget(dueBox);

    auto* priorityBox = new QGroupBox(tr("Приоритет"), this);
    auto* priorityLayout = new QHBoxLayout(priorityBox);
    priorityLayout->setContentsMargins(12, 10, 12, 12);
    m_priority = new QComboBox(priorityBox);
    m_priority->addItem(tr("Низкий"), 0);
    m_priority->addItem(tr("Обычный"), 1);
    m_priority->addItem(tr("Высокий"), 2);
    const int index = m_priority->findData(todo.priority);
    m_priority->setCurrentIndex(index >= 0 ? index : 1);
    priorityLayout->addWidget(m_priority, 1);
    root->addWidget(priorityBox);

    auto* noteBox = new QGroupBox(tr("Заметка"), this);
    auto* noteLayout = new QVBoxLayout(noteBox);
    noteLayout->setContentsMargins(12, 10, 12, 12);
    m_notes = new QPlainTextEdit(noteBox);
    m_notes->setPlaceholderText(tr("Детали, ссылки, следующий шаг…"));
    m_notes->setFixedHeight(90);
    m_notes->setPlainText(todo.notes);
    noteLayout->addWidget(m_notes);
    root->addWidget(noteBox);
    root->addStretch(1);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    auto* saveButton = buttons->button(QDialogButtonBox::Save);
    saveButton->setText(tr("Сохранить"));
    saveButton->setProperty("accent", true);
    saveButton->setDefault(true);
    saveButton->style()->unpolish(saveButton);
    saveButton->style()->polish(saveButton);
    buttons->button(QDialogButtonBox::Cancel)->setText(tr("Отмена"));
    connect(buttons, &QDialogButtonBox::accepted, this, &TodoDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(buttons);

    connect(m_hasDue, &QCheckBox::toggled, this, [this] { updateDueControls(); });
    updateDueControls();
    m_title->setFocus(Qt::OtherFocusReason);
    if (m_title->text().isEmpty()) {
        m_title->setPlaceholderText(tr("Что нужно сделать?"));
    } else {
        m_title->selectAll();
    }
}

void TodoDialog::updateDueControls()
{
    const bool enabled = m_hasDue->isChecked();
    m_dueDate->setEnabled(enabled);
    m_dueTime->setEnabled(enabled);
}

Todo TodoDialog::result() const
{
    return m_todo;
}

bool TodoDialog::isNew() const noexcept
{
    return m_isNew;
}

void TodoDialog::accept()
{
    if (m_title->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Задача"), tr("Введите название задачи."));
        return;
    }
    m_todo.title = m_title->text().trimmed();
    m_todo.notes = m_notes->toPlainText();
    m_todo.hasDue = m_hasDue->isChecked();
    if (m_todo.hasDue) {
        m_todo.dueDate = m_dueDate->date();
        m_todo.dueTime = m_dueTime->time();
    } else {
        m_todo.dueDate = QDate();
        m_todo.dueTime = QTime();
    }
    m_todo.priority = m_priority->currentData().toInt();
    QDialog::accept();
}

} // namespace pac::ui
