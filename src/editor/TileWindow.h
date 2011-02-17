#ifndef TILEWINDOW_H
#define TILEWINDOW_H

#include "guichan/widgets/window.hpp"
#include "guichan/widgets/button.hpp"
#include "guichan/widgets/label.hpp"
#include "guichan/actionlistener.hpp"
#include "GuichanExt.h"

class EditorEngine;
class TileLayer;
class TileboxPanel;

class TileWindow : public gcn::Window, public gcn::ActionListener
{
public:
    TileWindow(EditorEngine *engine);
    virtual ~TileWindow();

    virtual void action(const gcn::ActionEvent& ae);
    virtual void logic(void);

    inline TileboxPanel *GetTilesPanel(void) { return pTiles; }

protected:
    EditorEngine *_engine;
    TileboxPanel *pTiles; // the layer where all the tiles for selection will be put

    // GUI elements - tile window
    gcn::Label laCurFolder;
    gcn::Button btnNext;
    gcn::Button btnPrev;
    gcn::Button btnClose;
    gcn::Panel pBottom;
};

#endif
