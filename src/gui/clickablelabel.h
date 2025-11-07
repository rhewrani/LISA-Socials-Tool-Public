#ifndef CLICKEDLABEL_H
#define CLICKEDLABEL_H

#include "../core/manager.h"

class ClickableLabel : public QLabel
{
    Q_OBJECT
public:
    ClickableLabel(QWidget *parent=0): QLabel(parent){}
    ~ClickableLabel() {}
signals:
    void clicked(ClickableLabel* click);
protected:
    void mouseReleaseEvent(QMouseEvent*);
};

#endif
