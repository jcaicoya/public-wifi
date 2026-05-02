#pragma once

#include <QFrame>
#include <QString>

class QLabel;

class AlertBanner : public QFrame {
    Q_OBJECT

public:
    enum class Severity {
        Info,
        Success,
        Warning,
        Critical
    };

    explicit AlertBanner(QWidget* parent = nullptr);

    void setMessage(const QString& message);
    void setSeverity(Severity severity);

private:
    QLabel* m_label = nullptr;
};
