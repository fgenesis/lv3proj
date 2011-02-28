#ifndef BOTTOMBARPANEL_H
#define BOTTOMBARPANEL_H

#include "GuichanExt.h"

class EditorEngine;

class BottomBarPanel : public gcn::Panel, public gcn::ActionListener
{
public:
    BottomBarPanel(EditorEngine *engine);
    virtual ~BottomBarPanel();

    virtual void action(const gcn::ActionEvent& ae);
    virtual void clear(void);

    // add a mode selection button. when it is pressed, ActivateMode() falcon func is called
    void addButton(const char *text);


protected:
    EditorEngine *_engine;
    std::vector<gcn::Button*> _buttons;
};

#endif
