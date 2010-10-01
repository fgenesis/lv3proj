#include "common.h"
#include "AnimParser.h"
#include "ResourceMgr.h"
#include "LayerMgr.h"
#include "SoundCore.h"
#include "AsciiLevelParser.h"
#include "GuichanExt.h"
#include "LVPAFile.h"
#include "SDLImageLoaderManaged.h"
#include "MapFile.h"
#include "EditorEngine.h"


EditorEngine::EditorEngine()
: Engine()
{
    _gcnImgLoader = new gcn::SDLImageLoaderManaged();
    _gcnGfx = new gcn::SDLGraphics();
    _gcnInput = new gcn::SDLInput();
    _gcnGui = new gcn::Gui();
    _topWidget = new gcn::Container();
    _selOverlayShow = false;
    _selLayerShow = true;
    _selOverlayHighlight = false;
    panTileboxLayer = NULL;
    wndTilesLayer = NULL;
    _selLayer = NULL;
    _activeLayer = 0;
}

EditorEngine::~EditorEngine()
{
    ClearWidgets();

    delete _topWidget;
    delete _gcnFont;
    delete _gcnImgLoader;
    delete _gcnGfx;
    delete _gcnInput;
    delete _gcnGui;
}


bool EditorEngine::Setup(void)
{
    // setup the VFS and the container to read from
    LVPAFile *basepak = new LVPAFileReadOnly;
    basepak->LoadFrom("basepak.lvpa", LVPALOAD_SOLID);
    resMgr.vfs.LoadBase(basepak, true);
    resMgr.vfs.LoadFileSysRoot();
    resMgr.vfs.Prepare();

    _gcnGui->setGraphics(_gcnGfx);
    _gcnGui->setInput(_gcnInput);
    _gcnGfx->setTarget(GetSurface());
    gcn::Image::setImageLoader(_gcnImgLoader);

    _gcnGui->setTop(_topWidget);
    _gcnFont = new gcn::ImageFont("font/fixedfont.png", " abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
    gcn::Widget::setGlobalFont(_gcnFont);

    _layermgr->Clear();

    SetupInterface();
    SetupEditorLayers();


    return true;
}

bool EditorEngine::OnRawEvent(SDL_Event &evt)
{
    _gcnInput->pushInput(evt);
    return true;
}

void EditorEngine::OnKeyDown(SDLKey key, SDLMod mod)
{
    bool handled = true;
    switch(key)
    {
        case SDLK_t:
            ToggleTileWnd();
            break;

        case SDLK_g:
            ToggleTilebox();
            break;

        case SDLK_p:
            ToggleSelPreviewLayer();
            break;

        case SDLK_l:
            ToggleLayerPanel();
            break;


        default:
            handled = false;
    }

    if(!handled)
    {
        Engine::OnKeyDown(key, mod);
    }
}

void EditorEngine::OnKeyUp(SDLKey key, SDLMod mod)
{
    Engine::OnKeyUp(key, mod);
}

void EditorEngine::OnWindowResize(uint32 newx, uint32 newy)
{
    SDL_SetVideoMode(newx,newy,GetBPP(), GetSurface()->flags);
    SetupInterface();
}

void EditorEngine::_Process(uint32 ms)
{
    _gcnGui->logic();
    Engine::_Process(ms);

    // update this one only if necessary, we do not have to animate tiles there that are not shown anyway
    if(wndTilesLayer->visible)
        wndTilesLayer->Update(GetCurFrameTime());
    if(panTileboxLayer->visible)
        panTileboxLayer->Update(GetCurFrameTime());
    if(_selLayerShow && _selLayer->visible)
        _selLayer->Update(GetCurFrameTime());
}


void EditorEngine::_Render(void)
{
    SDL_FillRect(GetSurface(), NULL, 0); // blank the whole screen

    // draw the layer manager, which is responsible to display the tiles on the main panel
    // do not draw if there is a fullscren overlay.
    if(!wndTiles->isVisible())
        _layermgr->Render();

    // draw everything related to guichan
    _gcnGui->draw();

    // if the tile window is visible, draw its layer. the visibility check is performed inside Render();
    // the layer's visibility is controlled by ToggleTileWnd()
    wndTilesLayer->Render();

    // darw the tilebox layer, unless there is a fullscreen overlay
    if(!wndTiles->isVisible())
        panTileboxLayer->Render();

    // this draws the grey/white selection box (check performed inside function)
    _DrawSelOverlay();

    // the max. 4x4 preview box showing the currently selected tiles
    if(_selLayerShow && _selLayer->visible)
    {
        // limit the preview box area so that it can not fill the whole screen
        // draw it always in the top-left corner of the main panel
        gcn::Rectangle clip(_selLayerBorderRect.x,
                            _selLayerBorderRect.y,
                            std::min(_selLayerBorderRect.width, PREVIEWLAYER_MAX_SIZE * 16),
                            std::min(_selLayerBorderRect.height, PREVIEWLAYER_MAX_SIZE * 16));
        _gcnGfx->pushClipArea(clip);
        _selLayer->Render();
        _gcnGfx->setColor(gcn::Color(255,0,255,150));

        // this will put the rect into the right position
        clip.x = 0;
        clip.y = 0;
        _gcnGfx->drawRectangle(clip);
        _gcnGfx->popClipArea();
    }

    SDL_Flip(_screen);
}

void EditorEngine::_DrawSelOverlay(void)
{
    if(_selOverlayShow)
    {
        _gcnGfx->setColor(_selOverlayHighlight ? gcn::Color(255,255,255,255) : gcn::Color(255,255,255,150));
        _gcnGfx->pushClipArea(_selOverlayClip);
        _gcnGfx->drawRectangle(_selOverlayRect);
        _gcnGfx->popClipArea();
    }
}

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

void EditorEngine::SetupEditorLayers(void)
{
    _layermgr->Clear();
    _layermgr->SetMaxDim(128); // TODO: make this changeable later

    for(uint32 i = LAYER_REARMOST_BACKGROUND; i < LAYER_MAX; i++)
    {
        _layermgr->SetLayer(_layermgr->CreateLayer(false, 0, 0), i);

    }
    SetActiveLayer(LAYER_MAX / 2);
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


void EditorEngine::action(const gcn::ActionEvent& actionEvent)
{
    gcn::Widget *src = actionEvent.getSource();
    if(src == btnToggleTilebox)
    {
         ToggleTilebox();
    }
    else if(src == btnTiles)
    {
        ToggleTileWnd();
    }
    else if(src == btnToggleLayers)
    {
        ToggleLayerPanel();
    }
    else if(src == btnQuit)
    {
        _quit = true; // TODO: "Do you really want to...? YES DAMNIT!"
    }
    else if(src == btnSaveAs)
    {
        SaveCurrentMapAs("output.lvpm"); // TODO: show filename dialog
    }
    else if(src == btnLoad)
    {
        LoadMapFile("output.lvpm"); // TODO: show filename dialog
    }

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


void EditorEngine::mousePressed(gcn::MouseEvent& mouseEvent)
{
    gcn::Widget *src = mouseEvent.getSource();
    if(mouseEvent.getButton() == gcn::MouseEvent::LEFT)
    {
        if(src == panTilebox || src == wndTiles)
        {
            _mouseStartX = mouseEvent.getX() - (mouseEvent.getX() % 16);
            _mouseStartY = mouseEvent.getY() - (mouseEvent.getY() % 16);
            _selOverlayHighlight = true;
        }
    }

    if( (src == panTilebox && mouseEvent.getButton() == gcn::MouseEvent::RIGHT)
     || (src == panMain && mouseEvent.getButton() == gcn::MouseEvent::LEFT))
    {
        TileLayer *target = _GetActiveLayerForWidget(src);
        gcn::Rectangle rect = GetTargetableLayerTiles(mouseEvent.getX(), mouseEvent.getY(),
            _selLayerBorderRect.width, _selLayerBorderRect.height,
            _selLayer->GetArraySize(), _selLayer->GetArraySize());
        for(uint32 y = 0; y < uint32(rect.height); y++)
        {
            for(uint32 x = 0; x < uint32(rect.width); x++)
            {
                BasicTile *tile = _selLayer->GetTile(x,y);
                target->SetTile(x + rect.x, y + rect.y, tile);
            }
        }
    }
}

void EditorEngine::mouseReleased(gcn::MouseEvent& mouseEvent)
{
    gcn::Widget *src = mouseEvent.getSource();
    if(mouseEvent.getButton() == gcn::MouseEvent::LEFT)
    {
        _selOverlayHighlight = false;
        if(src == panTilebox || src == wndTiles)
        {
            _mouseStartX = mouseEvent.getX() - (mouseEvent.getX() % 16);
            _mouseStartY = mouseEvent.getY() - (mouseEvent.getY() % 16);
            UpdateSelection(mouseEvent.getSource());
            UpdateSelectionFrame(mouseEvent.getSource(), mouseEvent.getX(), mouseEvent.getY());
        }
    }
}

void EditorEngine::mouseExited(gcn::MouseEvent& mouseEvent)
{
    gcn::Widget *src = mouseEvent.getSource();
    if(src == wndTiles || src == panTilebox || src == panMain)
    {
        _selOverlayShow = false;
    }
}

void EditorEngine::mouseClicked(gcn::MouseEvent& mouseEvent)
{
    gcn::Widget *src = mouseEvent.getSource();
    // poll the buttons for a click
    for(uint32 i = 0; i < LAYER_MAX; i++)
    {
        if(src == btnLayers[i])
        {
            if(mouseEvent.getButton() == gcn::MouseEvent::LEFT)
                SetActiveLayer(i);
            else if(mouseEvent.getButton() == gcn::MouseEvent::RIGHT)
                ToggleLayerVisible(i);

            return;
        }
    }
    
}

void EditorEngine::mouseMoved(gcn::MouseEvent& mouseEvent)
{
    gcn::Widget *src = mouseEvent.getSource();
    if(src == wndTiles || src == panTilebox || src == panMain)
    {
        _mouseStartX = mouseEvent.getX();
        _mouseStartY = mouseEvent.getY();
        UpdateSelectionFrame(src, mouseEvent.getX(), mouseEvent.getY());
    }
    else
        _selOverlayShow = false;
}

void EditorEngine::mouseDragged(gcn::MouseEvent& mouseEvent)
{
    gcn::Widget *src = mouseEvent.getSource();
    if(mouseEvent.getButton() == gcn::MouseEvent::LEFT)
    {
        if(src == wndTiles || src == panTilebox || src == panMain)
        {
            UpdateSelectionFrame(src, mouseEvent.getX(), mouseEvent.getY());
        }
    }

    if(src == panMain && mouseEvent.getButton() == gcn::MouseEvent::LEFT)
    {
        TileLayer *target = _GetActiveLayerForWidget(src);
        gcn::Rectangle rect = GetTargetableLayerTiles(mouseEvent.getX(), mouseEvent.getY(),
            _selLayerBorderRect.width, _selLayerBorderRect.height,
            _selLayer->GetArraySize(), _selLayer->GetArraySize());
        for(uint32 y = 0; y < uint32(rect.height); y++)
        {
            for(uint32 x = 0; x < uint32(rect.width); x++)
            {
                BasicTile *tile = _selLayer->GetTile(x,y);
                target->SetTile(x + rect.x, y + rect.y, tile);
            }
        }
    }
}

void EditorEngine::UpdateSelectionFrame(gcn::Widget *src, int x, int y)
{
    if(src == panMain)
    {
        if(_selCurrentSelRect.width < 16 && _selCurrentSelRect.height < 16)
            return;

        // dont call Get16pxAlignedFrame() here, because this would also change width,
        // height and other stuff, thats not supposed to happen here
        _selOverlayRect = gcn::Rectangle(x - (x % 16), y - (y % 16), _selCurrentSelRect.width, _selCurrentSelRect.height);
    }
    else
    {
        gcn::Rectangle rect(std::min(_mouseStartX, x),
                            std::min(_mouseStartY, y),
                            std::max(_mouseStartX, x),
                            std::max(_mouseStartY, y));
        _selOverlayRect = Get16pxAlignedFrame(rect);
    }
    
    _selOverlayClip = src->getDimension();
    _selOverlayShow = true;
}

// returns a rect defining the targetable tile indexes of a TileLayer.
// addX, addY are passed as relative coords from baseXY.
// baseXY must be the positions of the upper left start point.
// aligning by 16 will be done inside the function.
// maxwidth, maxheight is the max. amount of tiles in each direction that may be treated selectable. -1 for infinite.
gcn::Rectangle EditorEngine::GetTargetableLayerTiles(uint32 baseX, uint32 baseY, uint32 addX, uint32 addY, uint32 maxwidth, uint32 maxheight)
{
    uint32 w = addX / 16;
    uint32 h = addY / 16;

    // + 1 because there is at least 1 tile to target, the one we are pointing to
    // subtract the space 
    uint32 maxDimX = std::min(w, maxwidth);
    uint32 maxDimY = std::min(h, maxheight);

    gcn::Rectangle rect(baseX / 16, baseY / 16, maxDimX, maxDimY);

    return rect;
}

void EditorEngine::UpdateSelection(gcn::Widget *src)
{
    int maxX = std::min(_selOverlayRect.width / 16, (int)_selLayer->GetArraySize());
    int maxY = std::min(_selOverlayRect.height / 16, (int)_selLayer->GetArraySize());
    _selLayerBorderRect.x = _selLayer->xoffs;
    _selLayerBorderRect.y = _selLayer->yoffs;
    _selLayerBorderRect.width = maxX * 16;
    _selLayerBorderRect.height = maxY * 16;

    TileLayer *srcLayer = _GetActiveLayerForWidget(src);

    if(srcLayer)
    {
        for(int iy = 0; iy < (int)_selLayer->GetArraySize() ; iy++)
        {
            for(int ix = 0; ix < (int)_selLayer->GetArraySize(); ix++)
            {
                if(iy >= maxY || ix >= maxX)
                {
                    _selLayer->SetTile(ix, iy, NULL);
                    continue;
                }

                int tilex = (_selOverlayRect.x + (ix * 16)) / 16;
                int tiley = (_selOverlayRect.y + (iy * 16)) / 16;
                if(tilex < 0 || tiley < 0)
                    continue;

                BasicTile *tile = srcLayer->GetTile(tilex, tiley);
                _selLayer->SetTile(ix, iy, tile);
            }
        }

        // store the current selection size, it is needed to preview on the main surface where the new tiles will be put
        _selCurrentSelRect = _selOverlayRect;
    }
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

void EditorEngine::SaveCurrentMapAs(const char *fn)
{
    MapFile::SaveAs(fn, _layermgr);
}

bool EditorEngine::LoadMapFile(const char *fn)
{
    memblock *mb = resMgr.LoadFile((char*)fn);
    LayerMgr *mgr = MapFile::Load(mb, this);
    resMgr.Drop(mb, true); // have to delete this file from memory immediately
    if(!mgr)
        return false;

    delete _layermgr;
    _layermgr = mgr;

    return false;
}
