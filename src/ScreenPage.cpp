#include "ScreenPage.h"

#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

ScreenPage::ScreenPage(const QString& screenId,
                       const QString& title,
                       QWidget* parent)
    : QWidget(parent)
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(24, 24, 24, 24);
    rootLayout->setSpacing(16);

    auto* headerFrame = new QFrame(this);
    headerFrame->setFrameShape(QFrame::StyledPanel);

    auto* headerLayout = new QHBoxLayout(headerFrame);
    headerLayout->setContentsMargins(16, 12, 16, 12);

    m_idLabel = new QLabel(screenId, headerFrame);
    QFont idFont = m_idLabel->font();
    idFont.setPointSize(24);
    idFont.setBold(true);
    m_idLabel->setFont(idFont);

    m_titleLabel = new QLabel(title, headerFrame);
    QFont titleFont = m_titleLabel->font();
    titleFont.setPointSize(20);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);

    headerLayout->addWidget(m_idLabel, 0);
    headerLayout->addSpacing(20);
    headerLayout->addWidget(m_titleLabel, 1);

    rootLayout->addWidget(headerFrame, 0);

    auto* contentFrame = new QFrame(this);
    contentFrame->setFrameShape(QFrame::StyledPanel);

    m_contentLayout = new QVBoxLayout(contentFrame);
    m_contentLayout->setContentsMargins(16, 16, 16, 16);
    m_contentLayout->setSpacing(12);

    rootLayout->addWidget(contentFrame, 1);

    auto* navFrame = new QFrame(this);
    navFrame->setFrameShape(QFrame::StyledPanel);

    m_navLayout = new QHBoxLayout(navFrame);
    m_navLayout->setContentsMargins(16, 12, 16, 12);
    m_navLayout->setSpacing(10);

    rootLayout->addWidget(navFrame, 0);

    m_mainLayout = rootLayout;
}

QPushButton* ScreenPage::addNavButton(const QString& text)
{
    auto* button = new QPushButton(text, this);
    button->setMinimumHeight(36);
    button->setFocusPolicy(Qt::NoFocus);
    m_navLayout->addWidget(button);
    return button;
}

void ScreenPage::setTitle(const QString& title)
{
    if (m_titleLabel) {
        m_titleLabel->setText(title);
    }
}
