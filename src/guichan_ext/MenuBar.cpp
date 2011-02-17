#include "guichan/exception.hpp"

#include "MenuBar.h"
#include "MenuButton.h"
#include "PopupMenu.h"

namespace gcn {


MenuBar::MenuBar(bool horiz /* = true */)
: Panel(0, 0), _horiz(horiz), _curBtn(NULL), _parentMenu(NULL)
{
    if(horiz)
        SetMaxSlots(-1, 1);
    else
        SetMaxSlots(1, -1);
}

MenuBar::~MenuBar()
{
    for(unsigned int i = 0; i < _entries.size(); ++i)
    {
        delete _entries[i].first;
        // the sub-menu is registered at the top widget and will be deleted with it
    }
    mWidgets.clear(); // to be sure this can't cause problems
}

void MenuBar::addEntry(const std::string& buttonName, PopupMenu *menu /* = NULL */)
{
    MenuButton *btn = new MenuButton(buttonName);
    btn->addActionListener(this);
    btn->adjustSize();
    // fix height + width
    if(_horiz)
    {
        setHeight(std::max<int>(getHeight(), btn->getHeight() + GetSpacingY() * 2));
        setWidth(getWidth() + btn->getWidth() + GetSpacingX() * 2);
    }
    else // vertical
    {
        unsigned int maxwidth = std::max<int>(getWidth(), btn->getWidth() + GetSpacingX() * 2);
        setWidth(maxwidth);
        setHeight(getHeight() + btn->getHeight() + GetSpacingY() * 2);
        btn->setWidth(maxwidth);
        for(unsigned int i = 0; i < _entries.size(); ++i)
            _entries[i].first->setWidth(maxwidth);
    }
    _entries.push_back(std::make_pair(btn, menu));
    if(menu)
        menu->_parentMenu = this;

    add(btn);
}

void MenuBar::action(const ActionEvent& ae)
{
    MenuButton *b = (MenuButton*)ae.getSource(); // <-- beware, unsafe cast!
    if(!b->isPressed())
        b = NULL;

    setActiveButton(b);
}

void MenuBar::setActiveButton(gcn::MenuButton *b)
{
    PopupMenu *menu = NULL;

    for(unsigned int i = 0; i < _entries.size(); ++i)
    {
        if(_entries[i].first == b)
            menu = _entries[i].second;
        if(_entries[i].first == _curBtn && _entries[i].second)
            _entries[i].second->setVisible(false);
    }

    if(_curBtn)
        _curBtn->setPressed(false);
    _curBtn = b;

    if(!b)
        return; // nothing else to do

    b->setPressed(true);
    
    if(menu)
        _openSubMenu(b, menu);
}

void MenuBar::_openSubMenu(MenuButton *b, PopupMenu *m)
{
    int x, y;
    b->getAbsolutePosition(x,y);
    m->setPosition(x, y + b->getHeight());
    m->setVisible(true);
}


} // end namespace gcn
