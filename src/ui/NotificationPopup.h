#pragma once

#include "core/Icons.h"

#include <QColor>
#include <QList>
#include <QWidget>

class QComboBox;
class QLabel;
class QPushButton;

namespace pac::ui {

class ThemedButton;

class NotificationPopup final : public QWidget {
    Q_OBJECT

public:
    explicit NotificationPopup(QWidget* parent = nullptr);

    void setContent(const QString& overline, const QString& title, const QString& details,
                    const QString& emoji, const QColor& accent, const QString& hint,
                    const QString& primaryText, icons::Icon primaryIcon,
                    const QString& secondaryText = QString());
    void setActivateOnShow(bool activate);
    void setSnoozeOptions(const QList<int>& minutes, int selected);
    [[nodiscard]] int selectedSnoozeMinutes() const;
    void dismissNow();
    void triggerPrimary();
    [[nodiscard]] bool isUrgent() const noexcept;

signals:
    void primaryClicked();
    void dismissed();

protected:
    void showEvent(QShowEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    QLabel* m_overline{nullptr};
    QLabel* m_title{nullptr};
    QLabel* m_details{nullptr};
    QLabel* m_icon{nullptr};
    QLabel* m_hint{nullptr};
    ThemedButton* m_primary{nullptr};
    QComboBox* m_snoozeCombo{nullptr};
    ThemedButton* m_secondary{nullptr};
    QPushButton* m_close{nullptr};
    QColor m_accent;
    bool m_activateOnShow{true};
};

} // namespace pac::ui
