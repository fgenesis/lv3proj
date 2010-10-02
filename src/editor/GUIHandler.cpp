#include "common.h"
#include "EditorEngine.h"
#include "ResourceMgr.h"
#include "GuichanExt.h"
#include "SDLImageLoaderManaged.h"


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
    panTilebox = panel = new gcn::Panel(0,0);
    fgcol = gcn::Color(200,200,200,255);
    bgcol = gcn::Color(50,50,50,100);
    panel->setForegroundColor(fgcol);
    panel->setBackgroundColor(bgcol);
    panel->setSize(4 * 16, freeHeight);
    panel->SetMaxSlots(4, -1);
    panel->addMouseListener(this);

    // the right tilebox panel must be added AFTER the main panel!

    // -- right panel end --

    // -- left layer panel start --
    panLayers = panel = new gcn::Panel(4,4);
    panel->setBackgroundColor(gcn::Color(0,75,0,255));
    panel->setForegroundColor(gcn::Color(0,200,0,255));
    panel->setSize(100, GetResY() - panBottom->getHeight());
    panel->setVisible(false);
    panel->SetMaxSlots(2, -1);
    for(uint32 i = 0; i < LAYER_MAX; i++)
    {
        char numstr[8];
        sprintf(numstr," %s%u ", i <= 9 ? " " : "", i);
        btnLayers[i] = btn = new gcn::Button(numstr);
        btn->addMouseListener(this);
        panel->add(RegWidget(btn));
    }

    // add panel to top widget
    AddWidgetTop(panel)->setPosition(0, 0);

    // -- left layer panel end --

    // -- main panel start --
    panMain = panel = new gcn::Panel(0,0);
    panel->setForegroundColor(gcn::Color(255,255,255,255));
    panel->setBackgroundColor(gcn::Color(0,0,0,0));
    panel->setSize(GetResX(), GetResY() - panBottom->getHeight());
    //panel->moveToBottom(panTilebox);
    panel->SetMaxSlots(-1,-1);
    panel->addMouseListener(this);

    // add panel to top widget
    AddWidgetTop(panel)->setPosition(0, 0);
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
    wndTiles->setFrameSize(0);
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


    SetupInterfaceLayers();
}

void EditorEngine::SetupInterfaceLayers(void)
{
    if(_selLayer)
        delete _selLayer;
    if(panTileboxLayer)
        delete panTileboxLayer;
    if(wndTilesLayer)
        delete wndTilesLayer;

    uint32 resmax = std::max(GetResX(), GetResY());
    _layermgr->SetMaxDim(resmax / 16); // TODO: this should be done less hacklike, maybe...
    _selLayer = (TileLayer*)_layermgr->CreateLayer(false); // this is explicitly created in the upper left corner

    int xo, yo;
    panTilebox->getAbsolutePosition(xo,yo);
    panTileboxLayer = (TileLayer*)_layermgr->CreateLayer(false, xo, yo);

    wndTilesLayer = (TileLayer*)_layermgr->CreateLayer(false);
    wndTilesLayer->visible = false;

    // ##### TEMP TEST DEBUG STUFF ####
    uint32 cnt = 0;
    std::list<std::string> dirs;
    dirs.push_back("ship");
    dirs.push_back("sprites");
    dirs.push_back("water");
    for(std::list<std::string>::iterator idir = dirs.begin(); idir != dirs.end(); idir++)
    {
        std::deque<std::string> files = GetFileList(std::string("gfx/") + *idir);
        for(std::deque<std::string>::iterator fi = files.begin(); fi != files.end(); fi++)
        {
            std::string fn(AddPathIfNecessary(*fi,*idir));
            BasicTile *tile = AnimatedTile::New(fn.c_str());

            if(tile)
            {
                wndTilesLayer->SetTile(cnt % (wndTilesLayer->GetArraySize() / 2), cnt / (wndTilesLayer->GetArraySize() / 2), tile);
                panTileboxLayer->SetTile(cnt % 4, cnt / 4, tile);
                cnt++;
            }
        }
    }
    // ### end debug stuff ###
}

gcn::Rectangle EditorEngine::Get16pxAlignedFrame(gcn::Rectangle rsrc)
{
    rsrc.width += 16;
    rsrc.height += 16;
    uint32 modx = rsrc.x % 16;
    uint32 mody = rsrc.y % 16;
    uint32 modw = rsrc.width % 16;
    uint32 modh = rsrc.height % 16;
    rsrc.x -= modx;
    rsrc.y -= mody;
    rsrc.width -= modw;
    rsrc.width -= rsrc.x;
    rsrc.height -= modh;
    rsrc.height -= rsrc.y;
    if(!rsrc.width)
        rsrc.width = 16;
    if(!rsrc.height)
        rsrc.height = 16;
    return rsrc;
}

void EditorEngine::ToggleVisible(gcn::Widget *w)
{
    w->setVisible(!w->isVisible());
}


TileLayer *EditorEngine::_GetActiveLayerForWidget(gcn::Widget *src)
{
    if(src == wndTiles)
        return wndTilesLayer;
    else if(src == panTilebox)
        return panTileboxLayer;
    // TODO: add support for more source layers if required
    // TODO: especially support for the different panMain layers!
    else if(src == panMain)
        return (TileLayer*)_layermgr->GetLayer(_activeLayer);

    return NULL;
}

void EditorEngine::ToggleSelPreviewLayer(void)
{
    _selLayer->visible = !_selLayer->visible;
}

void EditorEngine::ToggleTilebox(void)
{
    ToggleVisible(panTilebox);
    panTileboxLayer->visible = panTilebox->isVisible();
}

void EditorEngine::ToggleTileWnd(void)
{
    ToggleVisible(wndTiles);
    wndTilesLayer->visible = wndTiles->isVisible();
    _selLayerShow = !wndTilesLayer->visible;
}

void EditorEngine::ToggleLayerPanel(void)
{
    ToggleVisible(panLayers);
    if(panLayers->isVisible())
        SetLeftMainDistance(panLayers->getWidth());
    else
        SetLeftMainDistance(0);
}

void EditorEngine::SetActiveLayer(uint32 layerId)
{
    _activeLayer = layerId;
    UpdateLayerButtonColors();
}

void EditorEngine::ToggleLayerVisible(uint32 layerId)
{
    TileLayer *layer = _layermgr->GetLayer(layerId);
    layer->visible = !layer->visible;
    UpdateLayerButtonColors();
}

void EditorEngine::UpdateLayerButtonColors(void)
{
    for(uint32 i = 0; i < LAYER_MAX; i++)
    {
        TileLayer *layer = _layermgr->GetLayer(i);
        uint8 alpha = layer->visible ? 255 : 150;
        if(i == _activeLayer)
        {
            btnLayers[i]->setBaseColor(gcn::Color(180,255,180,alpha));
        }
        else
        {
            btnLayers[i]->setBaseColor(gcn::Color(128,128,144,alpha));
        }
    }
}

void EditorEngine::SetLeftMainDistance(uint32 dist)
{
    panMain->setX(dist);
    panMain->setWidth(GetResX() - dist);
    for(uint32 i = 0; i < LAYER_MAX; i++)
    {
        if(TileLayer *layer = _layermgr->GetLayer(i))
            layer->xoffs = dist;
    }
    _selLayer->xoffs = dist;
    _selLayerBorderRect.x = dist;
}
