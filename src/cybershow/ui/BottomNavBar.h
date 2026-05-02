#pragma once

#include <QWidget>
#include <QStringList>
#include <QList>

class QHBoxLayout;
class QPushButton;

// Reusable bottom navigation bar for Cybershow apps.
// - Fixed bottom placement is handled by the parent layout.
// - Buttons are checkable and expose index changes.
// - Intended mapping: number keys and left/right arrows in the host window.
class BottomNavBar : public QWidget {
    Q_OBJECT

public:
    explicit BottomNavBar(QWidget* parent = nullptr);

    void setItems(const QStringList& labels);
    void setCurrentIndex(int index);
    int currentIndex() const { return m_currentIndex; }

signals:
    void currentIndexChanged(int index);

private:
    void rebuild();

    QStringList m_labels;
    QList<QPushButton*> m_buttons;
    QHBoxLayout* m_layout = nullptr;
    int m_currentIndex = -1;
};
