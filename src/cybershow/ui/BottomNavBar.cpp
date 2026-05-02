#include "BottomNavBar.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QSizePolicy>
#include <QtAlgorithms>

BottomNavBar::BottomNavBar(QWidget* parent) : QWidget(parent) {
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(32, 8, 32, 18);
    m_layout->setSpacing(8);
}

void BottomNavBar::setItems(const QStringList& labels) {
    m_labels = labels;
    rebuild();
    if (!m_labels.isEmpty()) {
        setCurrentIndex(0);
    }
}

void BottomNavBar::setCurrentIndex(int index) {
    if (index < 0 || index >= m_buttons.size()) {
        return;
    }

    m_currentIndex = index;
    for (int i = 0; i < m_buttons.size(); ++i) {
        m_buttons[i]->setChecked(i == index);
    }

    emit currentIndexChanged(index);
}

void BottomNavBar::rebuild() {
    qDeleteAll(m_buttons);
    m_buttons.clear();

    while (QLayoutItem* item = m_layout->takeAt(0)) {
        delete item;
    }

    for (int i = 0; i < m_labels.size(); ++i) {
        auto* button = new QPushButton(QStringLiteral("%1 · %2").arg(i + 1).arg(m_labels[i]), this);
        button->setObjectName("NavButton");
        button->setCheckable(true);
        button->setMinimumHeight(34);
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        connect(button, &QPushButton::clicked, this, [this, i]() {
            setCurrentIndex(i);
        });

        m_buttons.push_back(button);
        m_layout->addWidget(button);
    }
}
