#include "common.h"
#include "ResourceMgr.h"
#include "LayerMgr.h"
#include "SoundCore.h"
#include "GuichanExt.h"
#include "LVPAFile.h"
#include "SDLImageLoaderManaged.h"
#include "SDL_func.h"
#include "LayerPanel.h"
#include "EditorEngine.h"
#include "DrawAreaPanel.h"

const char *defaultLayerNames[LAYER_MAX] =
{
    /* 00 */ "Background 1",
    /* 01 */ "Background 2",
    /* 02 */ "Background 3",
    /* 03 */ "Background 4",
    /* 04 */ "background 5",
    /* 05 */ "Sub 1",
    /* 06 */ "Sub 2",
    /* 07 */ "Sub 3",
    /* 08 */ "Sub 4",
    /* 09 */ "Base 1",
    /* 10 */ "Base 2",
    /* 11 */ "Base 3",
    /* 12 */ "Base 4",
    /* 13 */ "Base 5",
    /* 14 */ "Base 6",
    /* 15 */ "Base 7",
    /* 16 */ "Object 1",
    /* 17 */ "Object 2",
    /* 18 */ "Object 3",
    /* 19 */ "Object 4",
    /* 20 */ "Object 5",
    /* 21 */ "Object 6",
    /* 22 */ "Object 7",
    /* 23 */ "Object 8",
    /* 24 */ "Effect 1",
    /* 25 */ "Effect 2",
    /* 26 */ "Effect 3",
    /* 27 */ "Overlay 1",
    /* 28 */ "Overlay 2",
    /* 29 */ "Overlay 3",
    /* 30 */ "Overlay 4",
    /* 31 */ "Overlay 5"
};


EditorEngine::EditorEngine()
: GameEngine()
{
    _gcnImgLoader = new gcn::SDLImageLoaderManaged();
    _gcnGfx = new gcn::SDLGraphics();
    _gcnInput = new gcn::SDLInput();
    _gcnGui = new gcn::Gui();
    _topWidget = new gcn::Container();
    _fileDlg = NULL;
    _drawBackground = false; // here, guichan draws the black background, no reason to do it twice
}

EditorEngine::~EditorEngine()
{
    SaveData();
    
    // delete all widgets from the top widget
    while(_topWidget->mWidgets.size())
    {
        delete _topWidget->mWidgets.back();
        // nothing else to do, the widgets unregister themselves
    }

    _topWidget->mWidgets.clear();

    delete _topWidget;
    delete _gcnFont;
    delete _largeFont;
    delete _gcnImgLoader;
    delete _gcnGfx;
    delete _gcnInput;
    delete _gcnGui;
}


bool EditorEngine::Setup(void)
{
    if(!GameEngine::Setup())
        return false;

    _gcnGui->setGraphics(_gcnGfx);
    _gcnGui->setInput(_gcnInput);

    _gcnGui->setTop(_topWidget);
    _gcnGui->addLateGlobalKeyListener(this);

    // fixedfont.png
    _gcnFont = LoadFont("gfx/font/fixedfont.txt", "font/fixedfont.png");
    gcn::Widget::setGlobalFont(_gcnFont);

    // rpgfont.png
    _largeFont = LoadFont("gfx/font/rpgfont.txt", "font/rpgfont.png");

    _layermgr->Clear();
    _layermgr->SetMaxDim(64);

    // default config
    tileboxCols = 8;

    LoadPackages();
    _CreateInterfaceWidgets();
    SetupInterface();
    SetupEditorLayers();
    LoadData();
    panLayers->UpdateSelection(); // after layers are created, update buttons to assign text correctly
    FillUseableTiles();

    // TEMP -- DEBUG
    _layermgr->stringdata["__PACKAGES"] = "lostvikings_data";

    logdetail("EditorEngine setup completed.");

    return true;
}

bool EditorEngine::OnRawEvent(SDL_Event &evt)
{
    _gcnInput->pushInput(evt);
    return true;
}

void EditorEngine::OnWindowResize(uint32 newx, uint32 newy)
{
    SDL_SetVideoMode(newx,newy,GetBPP(), GetSurface()->flags);
    SetupInterface();
    GetVisibleBlockRect(); // trigger recalc
    panLayers->UpdateSelection();
}

void EditorEngine::_Process(uint32 ms)
{
    _gcnGui->logic();
    GameEngine::_Process(ms);
}


void EditorEngine::_Render(void)
{
    // draw everything related to guichan
    _gcnGui->draw();

    GameEngine::_PostRender();
}

void EditorEngine::SetupEditorLayers(void)
{
    _layermgr->Clear();
    _layermgr->SetMaxDim(128); // TODO: make this changeable later

    for(uint32 i = LAYER_REARMOST_BACKGROUND; i < LAYER_MAX; i++)
    {
        TileLayer *layer = _layermgr->CreateLayer(false, 0, 0);
        layer->name = defaultLayerNames[i];
        _layermgr->SetLayer(layer, i);

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
