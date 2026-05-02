#pragma once

#include <QFrame>
#include <QColor>
#include <QString>

class QLabel;

class MetricCard : public QFrame {
    Q_OBJECT

public:
    explicit MetricCard(QWidget* parent = nullptr);

    void setLabel(const QString& label);
    void setValue(const QString& value);
    void setSubtitle(const QString& subtitle);
    void setAccentColor(const QColor& color);

private:
    QLabel* m_label = nullptr;
    QLabel* m_value = nullptr;
    QLabel* m_subtitle = nullptr;
    QColor m_accent;
};
