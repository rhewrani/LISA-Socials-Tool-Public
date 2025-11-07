#include "clickablelabel.h"

void ClickableLabel::mouseReleaseEvent(QMouseEvent *)
{
    emit clicked(this);
}
