#ifndef GCN_EXT_MENUBAR_H
#define GCN_EXT_MENUBAR_H

#include <vector>
#include "guichan/actionlistener.hpp"
#include "Panel.h"

namespace gcn {

class PopupMenu;
class MenuButton;


class MenuBar : public Panel, public ActionListener
{
public:
    MenuBar(bool horiz = true);
    virtual ~MenuBar();

    void addEntry(const std::string& buttonName, PopupMenu *menu = NULL);
    void setActiveButton(MenuButton *b);
    

    virtual void action(const ActionEvent& ae);

    inline MenuButton *getActiveButton() { return _curBtn; }
    inline MenuBar *getParentMenu() { return _parentMenu; }
    inline MenuButton *getButton(unsigned int id) { return _entries[id].first; }
    inline PopupMenu *getSubMenu(unsigned int id) { return _entries[id].second; }

protected:
    typedef std::pair<MenuButton*, PopupMenu*> ButtonMenuPair;
    typedef std::vector<ButtonMenuPair> ButtonMenuVector;
    ButtonMenuVector _entries;
    MenuButton *_curBtn;
    MenuBar *_parentMenu;
    bool _horiz; // true if horizontal

private:
    void _openSubMenu(MenuButton *b, PopupMenu *m);
};


} // end namespace gcn

#endif
