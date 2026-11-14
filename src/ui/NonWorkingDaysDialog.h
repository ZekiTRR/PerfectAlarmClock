#pragma once

#include "core/Types.h"

#include <QDialog>
#include <vector>

class QDateEdit;
class QLineEdit;
class QListWidget;
class QLabel;

namespace pac {
class Store;
}

namespace pac::ui {

class NonWorkingDaysDialog final : public QDialog {
    Q_OBJECT

public:
    explicit NonWorkingDaysDialog(Store& store, QWidget* parent = nullptr);

protected:
    void accept() override;

private:
    void buildUi();
    void rebuildList();
    void addDay();
    void removeSelected();

    Store& m_store;
    std::vector<NonWorkingDay> m_days;
    QListWidget* m_list{nullptr};
    QDateEdit* m_date{nullptr};
    QLineEdit* m_name{nullptr};
    QLabel* m_count{nullptr};
};

} // namespace pac::ui
