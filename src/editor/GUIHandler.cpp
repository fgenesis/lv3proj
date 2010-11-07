#include "common.h"
#include "EditorEngine.h"
#include "ResourceMgr.h"
#include "GuichanExt.h"
#include "SDLImageLoaderManaged.h"
#include "FileDialog.h"
#include "LayerPanel.h"
#include "TileboxPanel.h"
#include "DrawAreaPanel.h"


void EditorEngine::ClearWidgets(void)
{
    _topWidget->clear();
    for(std::set<gcn::Widget*>::iterator it = _widgets.begin(); it != _widgets.end(); it++)
    {
        delete *it;
    }
    _widgets.clear();

}

// registers a widget for garbage collection
gcn::Widget *EditorEngine::RegWidget(gcn::Widget *w)
{
    _widgets.insert(w);
    return w;
}

// adds a widget to the top widget and registers it for garbage collection
gcn::Widget *EditorEngine::AddWidgetTop(gcn::Widget *w)
{
    _topWidget->add(w);
    return RegWidget(w);
}

// place widgets based on resolution.
// to be called on each resize event
void EditorEngine::SetupInterface(void)
{
    ClearWidgets();
    _topWidget->setDimension(gcn::Rectangle(0, 0, GetResX(), GetResY()));
    _topWidget->setOpaque(false);
    _topWidget->setBaseColor(gcn::Color(0,0,0,255));

    gcn::Panel *panel;
    gcn::Button *btn;
    gcn::Color fgcol;
    gcn::Color bgcol;

    // -- bottom panel start --
    panBottom = panel = new gcn::Panel(4,4);
    btnQuit = btn = new gcn::Button("Quit"); // we create this button earlier so the max height of the panel can be determined
    fgcol = gcn::Color(200,200,200,255);
    bgcol = gcn::Color(80,0,0,100);
    panel->setForegroundColor(fgcol);
    panel->setBackgroundColor(bgcol);
    panel->setSize(GetResX(), btn->getHeight() + panel->GetSpacingY() * 2);
    panel->SetMaxSlots(-1, 1);

    btn->addActionListener(this);
    panel->add(RegWidget(btn));

    btnNew = btn = new gcn::Button(" New ");
    btn->addActionListener(this);
    panel->add(RegWidget(btn));

    btnLoad = btn = new gcn::Button(" Load ");
    btn->addActionListener(this);
    panel->add(RegWidget(btn));

    btnSaveAs = btn = new gcn::Button(" Save as ");
    btn->addActionListener(this);
    panel->add(RegWidget(btn));

    btnData = btn = new gcn::Button(" Data ");
    btn->addActionListener(this);
    panel->add(RegWidget(btn));

    btnTiles = btn = new gcn::Button(" Tiles ");
    btn->addActionListener(this);
    panel->add(RegWidget(btn));

    btnToggleLayers = btn = new gcn::Button(" Layers ");
    btn->addActionListener(this);
    panel->add(RegWidget(btn));

    // this stuff is supposed to be added to the left edge of the panel
    btnToggleTilebox = btn = new gcn::Button("Toggle Tilebox");
    panel->InsertSpace(GetResX() - panel->GetNextX() - btn->getDimension().width - panel->GetSpacingX(), 0);
    btn->addActionListener(this);
    panel->add(RegWidget(btn));

    // add panel to top widget
    AddWidgetTop(panel)->setPosition(0, GetResY() - panel->getHeight());
    uint32 freeHeight = GetResY() - panel->getHeight();
    // -- bottom panel end --


    // -- right panel start --
    panTilebox = new TileboxPanel(this);
    panTilebox->SetBlockSize(16,16);
    fgcol = gcn::Color(200,200,200,255);
    bgcol = gcn::Color(30,30,30,200);
    panTilebox->setForegroundColor(fgcol);
    panTilebox->setBackgroundColor(bgcol);
    panTilebox->setSize(tileboxCols * 16, freeHeight);
    panTilebox->SetMaxSlots(tileboxCols, -1);
    panTilebox->addMouseListener(this);

    // the right tilebox panel must be added AFTER the main panel!

    // -- right panel end --

    // -- left layer panel start --
    panLayers = new LayerPanel(this, 180, GetResY() - panBottom->getHeight());
    AddWidgetTop(panLayers)->setPosition(0, 0);
    // -- left layer panel end --

    // -- main panel start --
    panMain = new DrawAreaPanel(this);
    panMain->SetLayerMgr(_layermgr);
    panMain->SetBlockSize(16, 16); // TODO: change this
    panMain->SetDraggable(false);
    panMain->setForegroundColor(gcn::Color(255,255,255,255));
    panMain->setBackgroundColor(gcn::Color(0,0,0,0));
    panMain->setSize(GetResX(), GetResY() - panBottom->getHeight());
    panMain->SetMaxSlots(-1,-1);
    panMain->addMouseListener(this);

    // add panel to top widget
    AddWidgetTop(panMain)->setPosition(0, 0);
    // -- main panel end --

    // time to add the right tilebox panel
    AddWidgetTop(panTilebox)->setPosition(GetResX() - panTilebox->getWidth(), 0);


    // -- tile window start --
    wndTiles = new gcn::Window("Tiles");
    wndTiles->setSize(GetResX(), GetResY() - panBottom->getHeight());
    wndTiles->setOpaque(true);
    wndTiles->setBaseColor(gcn::Color(0, 0, 0, 255));
    wndTiles->setFrameSize(0);
    wndTiles->setMovable(false);
    wndTiles->setVisible(false);
    wndTiles->setTitleBarHeight(0);
    btnTWPrev = btn = new gcn::Button("  Prev  ");
    btn->addActionListener(this);
    panel = new gcn::Panel(4,4);
    panel->setBackgroundColor(gcn::Color(0x99B0FF));
    panel->setForegroundColor(gcn::Color(255,255,255,255));
    panel->SetMaxSlots(-1, 1);
    panel->setSize(GetResX(), btn->getHeight() + panel->GetSpacingY() * 2);
    panel->add(RegWidget(btn));
    btnTWNext = btn = new gcn::Button("  Next  ");
    btn->addActionListener(this);
    panel->add(RegWidget(btn));
    panel->InsertSpace(15,0);
    laTWCurFolder = new gcn::Label("Current directory");
    panel->add(RegWidget(laTWCurFolder));
    wndTiles->add(RegWidget(panel), 0, wndTiles->getHeight() - panel->getHeight());
    wndTiles->addMouseListener(this);
    AddWidgetTop(wndTiles);
    // -- tile window end --

    // -- file dialog window start --
    if(!_fileDlg)
    {
        _fileDlg = new FileDialog();
        _fileDlg->SetCallback(this);
    }
    _topWidget->add(_fileDlg);
    // -- file dialog window end --

    panMain->requestMoveToBottom();


    SetupInterfaceLayers();
}

void EditorEngine::SetupInterfaceLayers(void)
{
    uint32 resmax = std::max(GetResX(), GetResY());
    uint32 tilesmax = resmax / 16; // TODO: fix for tile size != 16

    std::vector<TileLayer*>& tblayerv = panTilebox->GetTiles();
    TileLayer *tl;
    if(tblayerv.empty())
    {
         tl = new TileLayer();
        tl->target = GetSurface();
        tl->visible = true;

        tblayerv.push_back(tl);
    }
    else
    {
        tl = tblayerv[0];
    }

    tl->Resize(tilesmax);

    if(!wndTilesLayer)
    {
        wndTilesLayer = new TileLayer();
        wndTilesLayer->target = GetSurface();
        wndTilesLayer->visible = false;
    }
    wndTilesLayer->Resize(tilesmax);
}

void EditorEngine::ToggleVisible(gcn::Widget *w)
{
    w->setVisible(!w->isVisible());
}

void EditorEngine::ToggleSelPreviewLayer(void)
{
    TileLayerPanel *preview = panMain->GetPreview();
    preview->setVisible(!preview->isVisible());
}

void EditorEngine::ToggleTilebox(void)
{
    ToggleVisible(panTilebox);
}

void EditorEngine::ToggleTileWnd(void)
{
    ToggleVisible(wndTiles);
    wndTilesLayer->visible = wndTiles->isVisible();
}

void EditorEngine::ToggleLayerPanel(void)
{
    ToggleVisible(panLayers);
}

void EditorEngine::ToggleLayerVisible(uint32 layerId)
{
    TileLayer *layer = _layermgr->GetLayer(layerId);
    layer->visible = !layer->visible;
    panLayers->UpdateSelection();
}
