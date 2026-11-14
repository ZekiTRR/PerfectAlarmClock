#pragma once

#include <QStyledItemDelegate>

namespace pac::ui {

class AlarmDelegate final : public QStyledItemDelegate {
public:
    explicit AlarmDelegate(QObject* parent = nullptr);

    [[nodiscard]] QSize sizeHint(const QStyleOptionViewItem& option,
                                 const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
    bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option,
                     const QModelIndex& index) override;

    [[nodiscard]] static int alarmRowHeight();
};

} // namespace pac::ui
