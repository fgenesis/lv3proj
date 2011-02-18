#include <stdio.h>
#include "PopupMenu.h"

namespace gcn {

PopupMenu::PopupMenu()
: MenuBar(false) // not horizontal = vertical
{
    SetMaxSlots(-1, 1);
    setVisible(false);
    setPosition(0,0);
    setFrameSize(2);
}

PopupMenu::~PopupMenu()
{
}

void PopupMenu::action(const ActionEvent& ae)
{
    _curBtn = (MenuButton*)ae.getSource(); // <-- unsafe typecast!
    for(unsigned int i = 0; i < _entries.size(); ++i)
        if(_entries[i].first == _curBtn)
        {
            _curBtn->setPressed(false);
            if(MenuBar *prm = getParentMenu())
                prm->setActiveButton(NULL);
            setVisible(false);
            usedEntry(i);
            break;
        }
}

void PopupMenu::usedEntry(unsigned int which)
{
    printf("PopupMenu used entry %u, action un-implemented\n", which);
}

} // end namespace gcn
