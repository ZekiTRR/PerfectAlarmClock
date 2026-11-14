#pragma once

#include "UiKit.h"

#include <QVector>

class QGridLayout;
class QLabel;
class QWidget;

namespace pac {
class Store;
}

namespace pac::ui {

class ClockCard;

class WorldClockPage final : public PageWidget {
    Q_OBJECT

public:
    explicit WorldClockPage(Store& store, QWidget* parent = nullptr);

    void refreshClock(const QDateTime& now) override;

public slots:
    void addCity();

protected:
    void onActivated() override;
    void themeUpdated() override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void buildUi();
    void rebuildCards();
    void relayoutCards();
    void removeCity(const QUuid& id);
    [[nodiscard]] int columnCount() const;

    Store& m_store;
    QWidget* m_gridHost{nullptr};
    QGridLayout* m_grid{nullptr};
    QVector<ClockCard*> m_cards;
    QLabel* m_countLabel{nullptr};
    int m_lastColumns{-1};
};

} // namespace pac::ui
