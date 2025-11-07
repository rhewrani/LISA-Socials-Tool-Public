// BlockingOverlay.h
#ifndef BLOCKINGOVERLAY_H
#define BLOCKINGOVERLAY_H

#include <QDialog>
#include <QLabel>
#include "../core/utils.h"

class BlockingOverlay : public QDialog
{
    Q_OBJECT

public:
    explicit BlockingOverlay(QWidget *parent = nullptr, const QString &text = "Loading…");

    void setMessage(const QString &text);

    QLabel *m_label;

protected:
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
};

#endif // BLOCKINGOVERLAY_H
