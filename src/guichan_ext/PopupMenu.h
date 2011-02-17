/*
    This class is a horizontal stack of MenuButtons:
    +-----------+
    | Entry 1   |
    +-----------+
    | Entry 2   |
    +-----------+
    | Entry 3   |
    +-----------+
*/

#ifndef GCN_EXT_POPUPMENU_H

#include "MenuButton.h"
#include "MenuBar.h"

namespace gcn {


class PopupMenu : public MenuBar
{
public:
    PopupMenu();
    virtual ~PopupMenu();

    virtual void action(const ActionEvent& ae);
    virtual void usedEntry(unsigned int which);
};


} // end namespace gcn

#endif
