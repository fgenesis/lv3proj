#include "ListBox2.h"


void gcn::ListBox2::mousePressed(MouseEvent& mouseEvent)
{
    if (mouseEvent.getButton() == MouseEvent::LEFT)
    {
        setSelected(mouseEvent.getY() / getRowHeight());
        if(mouseEvent.getClickCount() > 1) // FG: send action only on double click
            distributeActionEvent();
    }
}
