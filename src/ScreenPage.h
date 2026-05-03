#pragma once

#include <QWidget>

class QLabel;
class QPushButton;
class QFrame;
class QVBoxLayout;
class QHBoxLayout;

class ScreenPage : public QWidget
{
    Q_OBJECT

public:
    explicit ScreenPage(const QString& screenId,
                        const QString& title,
                        QWidget* parent = nullptr);

    QVBoxLayout* contentLayout() const { return m_contentLayout; }
    QHBoxLayout* navLayout() const { return m_navLayout; }
    QPushButton* addNavButton(const QString& text);
    void setTitle(const QString& title);

private:
    QLabel* m_idLabel = nullptr;
    QLabel* m_titleLabel = nullptr;
    QFrame* m_navFrame = nullptr;
    QVBoxLayout* m_mainLayout = nullptr;
    QVBoxLayout* m_contentLayout = nullptr;
    QHBoxLayout* m_navLayout = nullptr;
};
