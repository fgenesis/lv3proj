#include "common.h"
#include "Animparser.h"
#include "ResourceMgr.h"
#include "SoundCore.h"
#include "AsciiLevelParser.h"
#include "Engine.h"


volatile uint32 Engine::s_curFrameTime;

Engine::Engine()
: _screen(NULL), _fps(0), _sleeptime(0), _quit(false), _framecounter(0)
{
    _tilemgr = new TileMgr(this);
    _fpsclock = s_curFrameTime = clock();
}

Engine::~Engine()
{
    if(_screen)
        SDL_FreeSurface(_screen);
}

void Engine::SetTitle(char *title)
{
    _wintitle = title;
}

void Engine::InitScreen(uint32 sizex, uint32 sizey, uint8 bpp /* = 0 */, bool fullscreen /* = false */)
{
    _winsizex = sizex;
    _winsizey = sizey;
    uint32 flags = SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_ANYFORMAT | SDL_HWACCEL;
    if(fullscreen)
        flags |= SDL_FULLSCREEN;
    _screen = SDL_SetVideoMode(sizex, sizey, bpp, flags);

    // the tile mgr uses an additional surface to pre-render static objects to save CPU,
    // which copies information from the screen surface and must be created after it
    _tilemgr->InitStaticSurface();
}

void Engine::Run(void)
{
    uint32 ms = clock();
    uint32 oldms;
    while(!_quit)
    {
        s_curFrameTime = ms;
        _ProcessEvents();
        oldms = ms;
        ms = clock();
        _Process(ms - oldms);
        _Render();
        _CalcFPS();
    }
}

void Engine::_ProcessEvents(void)
{
    SDL_Event evt;
    SDL_PollEvent(&evt);
    switch(evt.type)
    {
        case SDL_KEYDOWN:
            OnKeyDown(evt.key.keysym.sym, evt.key.keysym.mod);
            break;

        case SDL_KEYUP:
            OnKeyUp(evt.key.keysym.sym, evt.key.keysym.mod);
            break;

        case SDL_ACTIVEEVENT:
            OnWindowEvent(evt.active.gain);
            break;

        case SDL_QUIT:
            _quit = true;
            break;
    }
}

void Engine::_CalcFPS(void)
{
    ++_framecounter;
    uint32 ms = clock();
    if(ms - _fpsclock >= CLOCKS_PER_SEC)
    {
        char buf[100];
        _fpsclock = ms;
        _fps = _framecounter;
        _framecounter = 0;
        sprintf(buf, "%s - %u FPS", _wintitle.c_str(), _fps);
        SDL_WM_SetCaption((const char*)buf, NULL);
        if(_fps > 80)
        {
            ++_sleeptime;
        }
        else if(_sleeptime)
        {
            --_sleeptime;
        }
    }
    SDL_Delay(_sleeptime);
}

bool Engine::Setup(void)
{
    // load all *.anim files, and all additional files referenced in them
    std::deque<std::string> files = GetFileList("gfx");
    for(std::deque<std::string>::iterator it = files.begin(); it != files.end(); it++)
    {
        if(!memicmp(it->c_str() + (it->size() - 5), ".anim", 5))
        {
            Anim *ani = resMgr.LoadAnim((char*)it->c_str(), true);

            for(AnimMap::iterator am = ani->anims.begin(); am != ani->anims.end(); am++)
                for(AnimFrameStore::iterator af = am->second.begin(); af != am->second.end(); af++)
                    af->surface = resMgr.LoadImage((char*)af->filename.c_str(), true); // get all images referenced
        }
    }

    AsciiLevel *level = LoadAsciiLevel("levels/testlevel.txt");
    _tilemgr->LoadAsciiLevel(level);
    delete level;

    sndCore.PlayMusic("lv1_snes_ship.ogg");

    return true;
}

void Engine::_Process(uint32 ms)
{
    _tilemgr->HandleAnimation();
}

void Engine::OnWindowEvent(bool active)
{
    // TODO: pause if lost focus?
}

void Engine::OnKeyDown(SDLKey key, SDLMod mod)
{
    if(key == SDLK_F4 && (mod & KMOD_LALT))
        _quit = true;
    if(key == SDLK_RETURN && (mod & KMOD_LALT))
    {
        uint32 x = GetResX();
        uint32 y = GetResY();
        uint8 bpp = GetBPP();
        bool fullscreen = _screen->flags & SDL_FULLSCREEN;
        SDL_FreeSurface(_screen);
        InitScreen(x,y,bpp, !fullscreen);
    }

}

void Engine::OnKeyUp(SDLKey key, SDLMod mod)
{
}

void Engine::_Render(void)
{
    //_tilemgr->RenderBackground();
    _tilemgr->RenderStaticTiles();
    _tilemgr->RenderAnimatedTiles();
    // TODO: render sprites
    SDL_Flip(_screen);
}