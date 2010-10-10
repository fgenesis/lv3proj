#ifndef GCN_EXT_LISTBOX2_H
#define GCN_EXT_LISTBOX2_H

#include "guichan/widgets/listbox.hpp"

namespace gcn
{

class ListBox2: public ListBox
{
public:
    virtual void mousePressed(MouseEvent& mouseEvent);
};

}

#endif
