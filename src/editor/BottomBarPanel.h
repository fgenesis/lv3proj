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
    gcn::Button btnQuit;
    gcn::Button btnNew;
    gcn::Button btnToggleTilebox;
    gcn::Button btnSaveAs;
    gcn::Button btnLoad;
    gcn::Button btnData;
    gcn::Button btnTiles;
    gcn::Button btnToggleLayers;
};

#endif
