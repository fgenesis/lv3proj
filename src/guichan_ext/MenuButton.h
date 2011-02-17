#ifndef GCN_EXT_MENUBUTTON_H
#define GCN_EXT_MENUBUTTON_H

#include "guichan/platform.hpp"
#include "guichan/widget.hpp"
#include "guichan/graphics.hpp"
#include "guichan/widgets/button.hpp"
#include "guichan/actionlistener.hpp"
#include "guichan/actionevent.hpp"

namespace gcn {

class MenuButton : public Button, public ActionListener
{
public:
    MenuButton();
    MenuButton(const std::string& caption);
    virtual ~MenuButton();

    virtual void draw(Graphics* g);
    virtual void adjustSize(void);
    
    virtual void mouseEntered(MouseEvent& me);
    virtual void action(const ActionEvent& ae);
    virtual bool isPressed();
    virtual void setPressed(bool b);

protected:
    bool _pressed;
};


} // end namespace gcn

#endif
