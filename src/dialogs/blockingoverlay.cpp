// BlockingOverlay.cpp
#include "BlockingOverlay.h"
#include <QMainWindow>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <qapplication.h>

BlockingOverlay::BlockingOverlay(QWidget *parent, const QString &text)
    : QDialog(parent, Qt::Dialog | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
    , m_label(new QLabel(text, this))
{
    setModal(true);
    setWindowModality(parent ? Qt::WindowModal : Qt::ApplicationModal);

    setAttribute(Qt::WA_TranslucentBackground);
    setStyleSheet("background: transparent;");

    m_label->setStyleSheet(
        "QLabel {"
        "   color: white;"
        "   font-size: 16px;"
        "   font-weight: bold;"
        "   background-color: rgba(30, 30, 30, 220);"
        "   padding: 16px 32px;"
        "   border-radius: 8px;"
        "   min-width: 200px;"
        "   text-align: center;"
        "}"
        );
    m_label->setAlignment(Qt::AlignCenter);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(m_label, 0, Qt::AlignCenter);
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);


}

void BlockingOverlay::setMessage(const QString &text)
{
    m_label->setText(text);
}

void BlockingOverlay::closeEvent(QCloseEvent *event)
{
    event->ignore();
}

void BlockingOverlay::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        event->ignore();
        return;
    }
    QDialog::keyPressEvent(event);
}
