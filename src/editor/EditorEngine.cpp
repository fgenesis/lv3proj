#include "common.h"
#include "ResourceMgr.h"
#include "LayerMgr.h"
#include "SoundCore.h"
#include "GuichanExt.h"
#include "LVPAFile.h"
#include "SDLImageLoaderManaged.h"
#include "SDL_func.h"
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
    SaveData();
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
    _layermgr->SetMaxDim(64);

    // default config
    tileboxCols = 8;

    LoadData();
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

            // TODO: below, fix for tile size != 16
        case SDLK_UP:
            PanDrawingArea(0, -16);
            break;

        case SDLK_DOWN:
            PanDrawingArea(0, 16);
            break;

        case SDLK_LEFT:
            PanDrawingArea(-16, 0);
            break;

        case SDLK_RIGHT:
            PanDrawingArea(16, 0);
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
    GetVisibleBlockRect(); // trigger recalc
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
    {
        _layermgr->Render();

        // draw box around the drawing area
        uint32 pixdim = _layermgr->GetMaxPixelDim();
        gcn::Rectangle clip(
            -_cameraPos.x,
            -_cameraPos.y,
            pixdim,
            pixdim);


        _gcnGfx->setColor(gcn::Color(255, 0, 0, 180));
        _gcnGfx->pushClipArea(gcn::Rectangle(0,0,GetResX(), GetResY()));
        _gcnGfx->drawRectangle(clip);
        // draw 2nd rect around it (makes it thicker)
        clip.x--;
        clip.y--;
        clip.width += 2;
        clip.height+= 2;
        _gcnGfx->drawRectangle(clip);
        _gcnGfx->popClipArea();
    }

    // draw everything related to guichan
    _gcnGui->draw();

    // if the tile window is visible, draw its layer. the visibility check is performed inside Render();
    // the layer's visibility is controlled by ToggleTileWnd()
    wndTilesLayer->Render();

    // draw the tilebox layer, unless there is a fullscreen overlay
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




void EditorEngine::UpdateSelectionFrame(gcn::Widget *src, int x, int y)
{
    if(src == panMain)
    {
        if(_selCurrentSelRect.width < 16 && _selCurrentSelRect.height < 16)
            return;

        // dont call Get16pxAlignedFrame() here, because this would also change width,
        // height and other stuff, thats not supposed to happen here
        _selOverlayRect = gcn::Rectangle(
            x - ((_cameraPos.x + x) % 16),
            y - ((_cameraPos.y + y) % 16),
            _selCurrentSelRect.width,
            _selCurrentSelRect.height);
    }
    else
    {
        gcn::Rectangle rect(std::min(_mouseLeftStartX, x),
                            std::min(_mouseLeftStartY, y),
                            std::max(_mouseLeftStartX, x),
                            std::max(_mouseLeftStartY, y));
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
gcn::Rectangle EditorEngine::GetTargetableLayerTiles(uint32 baseX, uint32 baseY, uint32 addX, uint32 addY,
                                                     uint32 maxwidth, uint32 maxheight, TileLayer *layer)
{
    uint32 w = addX / 16;
    uint32 h = addY / 16;

    // + 1 because there is at least 1 tile to target, the one we are pointing to
    // subtract the space 
    uint32 maxDimX = std::min(w, maxwidth);
    uint32 maxDimY = std::min(h, maxheight);

    Point *camera = layer->camera;
    if(camera)
    {
        return gcn::Rectangle((baseX + camera->x) / 16, (baseY + camera->y) / 16, maxDimX, maxDimY);
    }
    else
    {
        return gcn::Rectangle(baseX / 16, baseY / 16, maxDimX, maxDimY);
    }
    
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

void EditorEngine::PanDrawingArea(int32 x, int32 y)
{
    _cameraPos.x += x;
    _cameraPos.y += y;

    int32 pixdim = _layermgr->GetMaxPixelDim();
    int32 halfx = GetResX() / 2;
    int32 halfy = GetResY() / 2;

    // limit view
    if(_cameraPos.x < -halfx)
        _cameraPos.x = -halfx;
    else if(_cameraPos.x > pixdim - halfx)
        _cameraPos.x = pixdim - halfx;

    if(_cameraPos.y < -halfy)
        _cameraPos.y = -halfy;
    else if(_cameraPos.y > pixdim - halfy)
        _cameraPos.y = pixdim - halfy;

    GetVisibleBlockRect(); // to trigger recalc
}
