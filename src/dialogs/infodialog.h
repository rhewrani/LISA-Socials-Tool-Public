#ifndef INFODIALOG_H
#define INFODIALOG_H

#include <QDialog>
#include "../core/manager.h"

class Manager;

namespace Ui {
class Infodialog;
}

class Infodialog : public QDialog
{
    Q_OBJECT

public:
    explicit Infodialog(Manager *managerRef, QWidget *parent = nullptr);
    ~Infodialog();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    Ui::Infodialog *ui;
    Manager *manager;

    QPoint m_dragStartPosition;
    bool m_isDragging = false;
    QWidget *titleBar = nullptr;

    void Init();
    void InitTitleBar();
    void InitLang();
};

#endif // INFODIALOG_H
