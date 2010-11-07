#include "common.h"
#include "BottomBarPanel.h"
#include "EditorEngine.h"

BottomBarPanel::BottomBarPanel(EditorEngine *engine)
: Panel(4, 4), _engine(engine),
btnQuit(" Quit "),
btnNew(" New "),
btnToggleTilebox("Toggle Tilebox"),
btnSaveAs(" Save As "),
btnLoad(" Load "),
btnTiles(" Tiles "),
btnToggleLayers(" Layers ")
{
    SetMaxSlots(-1, 1);
    setSize(_engine->GetResX(), btnQuit.getHeight() + GetSpacingY() * 2);

    btnQuit.addActionListener(this);
    btnNew.addActionListener(this);
    btnLoad.addActionListener(this);
    btnSaveAs.addActionListener(this);
    btnTiles.addActionListener(this);
    btnToggleLayers.addActionListener(this);
    btnToggleTilebox.addActionListener(this);
    add(&btnQuit);
    add(&btnNew);
    add(&btnLoad);
    add(&btnSaveAs);
    add(&btnTiles);
    add(&btnToggleLayers);
    InsertSpace(_engine->GetResX() - GetNextX() - btnToggleTilebox.getDimension().width - GetSpacingX(), 0);
    add(&btnToggleTilebox);

    setForegroundColor(gcn::Color(200,200,200,255));
    setBackgroundColor(gcn::Color(80,0,0,100));
}

BottomBarPanel::~BottomBarPanel()
{
}

void BottomBarPanel::action(const gcn::ActionEvent& ae)
{
    gcn::Widget *src = ae.getSource();
    if(src == &btnToggleTilebox)
    {
        _engine->ToggleTilebox();
    }
    else if(src == &btnTiles)
    {
        _engine->ToggleTileWnd();
    }
    else if(src == &btnToggleLayers)
    {
        _engine->ToggleLayerPanel();
    }
    else if(src == &btnQuit)
    {
        _engine->Shutdown(); // TODO: "Do you really want to...? YES DAMNIT!"
    }
    else if(src == &btnSaveAs)
    {
        _engine->GetFileDlg()->Open(true, "map");
    }
    else if(src == &btnLoad)
    {
        _engine->GetFileDlg()->Open(false, "map");
    }
}
