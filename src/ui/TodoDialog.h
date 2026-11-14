#pragma once

#include "core/Types.h"

#include <QDialog>

class QCheckBox;
class QComboBox;
class QDateEdit;
class QLineEdit;
class QPlainTextEdit;
class QTimeEdit;

namespace pac::ui {

class TodoDialog final : public QDialog {
    Q_OBJECT

public:
    TodoDialog(const Todo& todo, bool isNew, QWidget* parent = nullptr);

    [[nodiscard]] Todo result() const;
    [[nodiscard]] bool isNew() const noexcept;

protected:
    void accept() override;

private:
    void updateDueControls();

    Todo m_todo;
    bool m_isNew{false};
    QLineEdit* m_title{nullptr};
    QCheckBox* m_hasDue{nullptr};
    QDateEdit* m_dueDate{nullptr};
    QTimeEdit* m_dueTime{nullptr};
    QComboBox* m_priority{nullptr};
    QPlainTextEdit* m_notes{nullptr};
};

} // namespace pac::ui
