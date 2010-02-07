#include "common.h"
#include "Animparser.h"
#include "ResourceMgr.h"
#include "LayerMgr.h"
#include "SoundCore.h"
#include "AsciiLevelParser.h"
#include "GuichanExt.h"
#include "EditorEngine.h"


EditorEngine::EditorEngine()
: Engine()
{
    _gcnImgLoader = new gcn::SDLImageLoader();
    _gcnGfx = new gcn::SDLGraphics();
    _gcnInput = new gcn::SDLInput();
    _gcnGui = new gcn::Gui();
    _topWidget = new gcn::Container();
    _selOverlayShow = false;
    _selLayerShow = true;
    _selOverlayHighlight = false;
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
    _gcnGui->setGraphics(_gcnGfx);
    _gcnGui->setInput(_gcnInput);
    _gcnGfx->setTarget(GetSurface());
    gcn::Image::setImageLoader(_gcnImgLoader);

    // blah.. test
    //resMgr.LoadPropsInDir("music");
    //sndCore.PlayMusic("lv1_amiga_intro.ogg");

    _gcnGui->setTop(_topWidget);
    _gcnFont = new gcn::ImageFont("gfx/fixedfont.png", " abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
    gcn::Widget::setGlobalFont(_gcnFont);

    SetupInterface();


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


        default:
            handled = false;
    }
    Engine::OnKeyDown(key, mod);
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
    _layermgr->Render();
    _gcnGui->draw();

    wndTilesLayer->Render();
    panTileboxLayer->Render();
    _DrawSelOverlay();
    if(_selLayerShow)
    {
        _selLayer->Render();
        _gcnGfx->setColor(gcn::Color(255,0,255,150));
        _gcnGfx->pushClipArea(_selLayerBorderRect);
        _gcnGfx->drawRectangle(_selLayerBorderRect);
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

void EditorEngine::SetupInterfaceLayers(void)
{
    // _layermgr already created in Engine::Engine()
    _layermgr->Clear();

    _layermgr->SetMaxDim(PREVIEWLAYER_MAX_SIZE);
    _selLayer = (TileLayer*)_layermgr->CreateLayer(LAYERTYPE_ANIMATED, false, 0, 0); // this is explicitly created in the upper left corner

    uint32 resmax = std::max(GetResX(), GetResY());
    _layermgr->SetMaxDim(resmax / 16); // TODO: this should be done less hacklike, maybe...

    int xo, yo;
    panTilebox->getAbsolutePosition(xo,yo);
    panTileboxLayer = (TileLayer*)_layermgr->CreateLayer(LAYERTYPE_ANIMATED, false, xo, yo);

    wndTilesLayer = (TileLayer*)_layermgr->CreateLayer(LAYERTYPE_ANIMATED, false);
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
            BasicTile *tile = NULL;
            if(FileGetExtension(*fi) == ".anim")
            {
                Anim *ani = resMgr.LoadAnim((char*)AddPathIfNecessary(*fi,*idir).c_str());
                if(ani)
                    tile = new AnimatedTile(ani);
            }
            else if(FileGetExtension(*fi) == ".png")
            {
                SDL_Surface *img = resMgr.LoadImage((char*)AddPathIfNecessary(*fi,*idir).c_str());
                if(img)
                {
                    tile = new BasicTile;
                    tile->surface = img;
                }
            }

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


// place widgets based on resolution.
// to be called on each resize event
void EditorEngine::SetupInterface(void)
{
    ClearWidgets();
    _topWidget->setDimension(gcn::Rectangle(0, 0, GetResX(), GetResY()));
    _topWidget->setOpaque(true);
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

    // add panel to top widget
    AddWidgetTop(panel)->setPosition(GetResX() - panel->getWidth(), 0);
    // -- right panel end --

    // -- tile window start --
    wndTiles = new gcn::Window("Tiles");
    wndTiles->setSize(GetResX() - panTilebox->getWidth(), GetResY() - panBottom->getHeight());
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
    else if(src == btnQuit)
    {
        _quit = true; // TODO: "Do you really want to...? YES DAMNIT!"
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

void EditorEngine::mousePressed(gcn::MouseEvent& mouseEvent)
{
    gcn::Widget *src = mouseEvent.getSource();
    _mouseStartX = mouseEvent.getX() - (mouseEvent.getX() % 16);
    _mouseStartY = mouseEvent.getY() - (mouseEvent.getY() % 16);
    _selOverlayHighlight = true;
}

void EditorEngine::mouseReleased(gcn::MouseEvent& mouseEvent)
{
    _selOverlayHighlight = false;
    _mouseStartX = mouseEvent.getX() - (mouseEvent.getX() % 16);
    _mouseStartY = mouseEvent.getY() - (mouseEvent.getY() % 16);
    UpdateSelection(mouseEvent.getSource());
    UpdateSelectionFrame(mouseEvent.getSource(), mouseEvent.getX(), mouseEvent.getY());
}

void EditorEngine::mouseExited(gcn::MouseEvent& mouseEvent)
{
    gcn::Widget *src = mouseEvent.getSource();
    if(src == wndTiles || src == panTilebox)
    {
        _selOverlayShow = false;
    }
}

void EditorEngine::mouseMoved(gcn::MouseEvent& mouseEvent)
{
    gcn::Widget *src = mouseEvent.getSource();
    if(src == wndTiles || src == panTilebox)
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
    if(src == wndTiles || src == panTilebox)
    {
        UpdateSelectionFrame(src, mouseEvent.getX(), mouseEvent.getY());
    }
}

void EditorEngine::UpdateSelectionFrame(gcn::Widget *src, int x, int y)
{
    gcn::Rectangle rect(std::min(_mouseStartX, x),
                        std::min(_mouseStartY, y),
                        std::max(_mouseStartX, x),
                        std::max(_mouseStartY, y));
    
    _selOverlayRect = Get16pxAlignedFrame(rect);
    _selOverlayClip = src->getDimension();
    _selOverlayShow = true;
}

void EditorEngine::UpdateSelection(gcn::Widget *src)
{
    int maxX = std::min(_selOverlayRect.width / 16, (int)_selLayer->GetArraySize());
    int maxY = std::min(_selOverlayRect.height / 16, (int)_selLayer->GetArraySize());
    _selLayerBorderRect.x = _selLayer->xoffs;
    _selLayerBorderRect.y = _selLayer->yoffs;
    _selLayerBorderRect.width = maxX * 16;
    _selLayerBorderRect.height = maxY * 16;

    TileLayer *srcLayer = NULL;

    // TODO: add support for more source layers if required
    if(src == wndTiles)
        srcLayer = wndTilesLayer;
    else if(src == panTilebox)
        srcLayer = panTileboxLayer;

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
