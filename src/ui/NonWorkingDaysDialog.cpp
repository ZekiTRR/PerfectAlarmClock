#include "NonWorkingDaysDialog.h"

#include "UiKit.h"
#include "core/Catalog.h"
#include "core/Store.h"

#include <QDateEdit>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#include <algorithm>

namespace pac::ui {

NonWorkingDaysDialog::NonWorkingDaysDialog(Store& store, QWidget* parent)
    : QDialog(parent)
    , m_store(store)
    , m_days(store.data().nonWorkingDays)
{
    setWindowTitle(tr("Праздники и выходные"));
    setMinimumSize(620, 560);
    resize(660, 620);
    buildUi();
    rebuildList();
}

void NonWorkingDaysDialog::buildUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 14);
    root->setSpacing(10);

    auto* header = new QLabel(tr("Праздники и выходные"), this);
    QFont titleFont = header->font();
    titleFont.setPointSizeF(15.0);
    titleFont.setWeight(QFont::DemiBold);
    header->setFont(titleFont);
    root->addWidget(header);
    auto* subtitle = new QLabel(
        tr("Даты из этого списка используются правилами «пропустить» и «перенести» "
           "на следующий рабочий день."),
        this);
    subtitle->setObjectName(QStringLiteral("Muted"));
    subtitle->setWordWrap(true);
    root->addWidget(subtitle);

    m_list = new QListWidget(this);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_list->setUniformItemSizes(true);
    root->addWidget(m_list, 1);

    auto* addRow = new QHBoxLayout;
    addRow->setSpacing(8);
    m_date = new QDateEdit(this);
    m_date->setCalendarPopup(true);
    m_date->setDisplayFormat(QStringLiteral("dd.MM.yyyy"));
    m_date->setDate(QDate::currentDate());
    m_date->setFixedWidth(150);
    addRow->addWidget(m_date);
    m_name = new QLineEdit(this);
    m_name->setPlaceholderText(tr("Название праздника или выходного"));
    addRow->addWidget(m_name, 1);
    auto* addButton = new QPushButton(tr("Добавить"), this);
    addRow->addWidget(addButton);
    auto* removeButton = new QPushButton(tr("Удалить"), this);
    addRow->addWidget(removeButton);
    root->addLayout(addRow);

    m_count = new QLabel(this);
    m_count->setObjectName(QStringLiteral("Faint"));
    root->addWidget(m_count);

    auto* footer = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    footer->button(QDialogButtonBox::Save)->setText(tr("Сохранить"));
    footer->button(QDialogButtonBox::Save)->setProperty("accent", true);
    footer->button(QDialogButtonBox::Cancel)->setText(tr("Отмена"));
    root->addWidget(footer);

    connect(addButton, &QPushButton::clicked, this, &NonWorkingDaysDialog::addDay);
    connect(removeButton, &QPushButton::clicked, this, &NonWorkingDaysDialog::removeSelected);
    connect(m_name, &QLineEdit::returnPressed, this, &NonWorkingDaysDialog::addDay);
    connect(footer, &QDialogButtonBox::accepted, this, &NonWorkingDaysDialog::accept);
    connect(footer, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void NonWorkingDaysDialog::rebuildList()
{
    std::sort(m_days.begin(), m_days.end(), [](const NonWorkingDay& left,
                                               const NonWorkingDay& right) {
        return left.date < right.date;
    });
    m_list->clear();
    for (const auto& day : m_days) {
        const QString text = QStringLiteral("%1 · %2")
                                 .arg(day.date.toString(QStringLiteral("dd.MM.yyyy")),
                                      day.name);
        auto* item = new QListWidgetItem(text, m_list);
        item->setData(Qt::UserRole, day.date.toString(Qt::ISODate));
    }
    m_count->setText(tr("Всего дат: %1").arg(m_days.size()));
}

void NonWorkingDaysDialog::addDay()
{
    const QDate date = m_date->date();
    if (!date.isValid()) {
        return;
    }
    QString name = m_name->text().trimmed();
    if (name.isEmpty()) {
        name = tr("Выходной день");
    }
    for (const auto& day : m_days) {
        if (day.date == date) {
            QMessageBox::information(this, tr("Праздники"),
                                     tr("Эта дата уже есть в списке."));
            return;
        }
    }
    m_days.push_back({date, name});
    m_name->clear();
    rebuildList();
}

void NonWorkingDaysDialog::removeSelected()
{
    auto* item = m_list->currentItem();
    if (item == nullptr) {
        return;
    }
    const QDate date = QDate::fromString(item->data(Qt::UserRole).toString(), Qt::ISODate);
    m_days.erase(std::remove_if(m_days.begin(), m_days.end(),
                                [&date](const NonWorkingDay& day) {
                                    return day.date == date;
                                }),
                 m_days.end());
    rebuildList();
}

void NonWorkingDaysDialog::accept()
{
    std::sort(m_days.begin(), m_days.end(), [](const NonWorkingDay& left,
                                               const NonWorkingDay& right) {
        return left.date < right.date;
    });
    m_store.data().nonWorkingDays = m_days;
    m_store.commit(Domain::Alarms);
    QDialog::accept();
}

} // namespace pac::ui
