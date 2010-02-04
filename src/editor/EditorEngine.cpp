#include <guichan.hpp>
#include <guichan/sdl.hpp>

#include "common.h"
#include "Animparser.h"
#include "ResourceMgr.h"
#include "LayerMgr.h"
#include "SoundCore.h"
#include "AsciiLevelParser.h"
#include "EditorEngine.h"


EditorEngine::EditorEngine()
: Engine()
{
    _gcnImgLoader = new gcn::SDLImageLoader();
    _gcnGfx = new gcn::SDLGraphics();
    _gcnInput = new gcn::SDLInput();
    _gcnGui = new gcn::Gui();
    _topWidget = new gcn::Container();
}

EditorEngine::~EditorEngine()
{
    for(std::set<gcn::Widget*>::iterator it = _widgets.begin(); it != _widgets.end(); it++)
    {
        delete *it;
    }

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
    resMgr.LoadPropsInDir("music");
    sndCore.PlayMusic("lv1_amiga_intro.ogg");

    _topWidget->setDimension(gcn::Rectangle(0, 0, GetSurface()->w, GetSurface()->h));
    _gcnGui->setTop(_topWidget);
    _gcnFont = new gcn::ImageFont("gfx/fixedfont.png", " abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
    gcn::Widget::setGlobalFont(_gcnFont);

    gcn::Label* label;
    label = new gcn::Label("Hello World");
    label->setPosition(280, 220);
    gcn::Color col(255,255,255,255);
    label->setBaseColor(col);
    _topWidget->add(label);
    _widgets.insert(label);


    return true;
}

bool EditorEngine::OnRawEvent(SDL_Event &evt)
{
    _gcnInput->pushInput(evt);
    return true;
}

void EditorEngine::OnKeyDown(SDLKey key, SDLMod mod)
{
    Engine::OnKeyDown(key, mod);
}

void EditorEngine::OnKeyUp(SDLKey key, SDLMod mod)
{
    Engine::OnKeyUp(key, mod);
}

void EditorEngine::OnWindowResize(uint32 newx, uint32 newy)
{
    SDL_SetVideoMode(newx,newy,GetBPP(), GetSurface()->flags);
    _topWidget->setDimension(gcn::Rectangle(0, 0, GetSurface()->w, GetSurface()->h));
}

void EditorEngine::_Process(uint32 ms)
{
    _gcnGui->logic();
    Engine::_Process(ms);
}


void EditorEngine::_Render(void)
{
    _layermgr->Render();
    _gcnGui->draw();
    SDL_Flip(_screen);
}
