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


protected:
    EditorEngine *_engine;
    gcn::Button btnTileBrush;
    gcn::Button btnObjBrush;
};

#endif
