#include "common.h"
#include "AnimParser.h"
#include "ResourceMgr.h"
#include "LayerMgr.h"
#include "SoundCore.h"
#include "AsciiLevelParser.h"
#include "Engine.h"
#include "PhysicsSystem.h"
#include "ObjectMgr.h"


volatile uint32 Engine::s_curFrameTime;

Engine::Engine()
: _screen(NULL), _fps(0), _sleeptime(0), _quit(false), _framecounter(0)
{
    _layermgr = new LayerMgr(this);
    _fpsclock = s_curFrameTime = clock();

    physmgr = new PhysicsMgr;
    physmgr->SetLayerMgr(_layermgr);
    objmgr = new ObjectMgr(this);
    objmgr->SetLayerMgr(_layermgr);
    objmgr->SetPhysicsMgr(physmgr);
}

Engine::~Engine()
{
    if(_screen)
        SDL_FreeSurface(_screen);
}

void Engine::Shutdown(void)
{
}

void Engine::SetTitle(char *title)
{
    _wintitle = title;
}

void Engine::InitScreen(uint32 sizex, uint32 sizey, uint8 bpp /* = 0 */, uint32 extraflags /* = 0 */)
{
    _winsizex = sizex;
    _winsizey = sizey;
    _screenFlags = SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_ANYFORMAT | SDL_HWACCEL | extraflags;
    _screen = SDL_SetVideoMode(sizex, sizey, bpp, _screenFlags);
    _screenFlags &= ~SDL_FULLSCREEN; // this depends on current setting and should not be stored
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
    while(SDL_PollEvent(&evt))
    {
        if(!OnRawEvent(evt))
            continue;

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

            case SDL_VIDEORESIZE:
                OnWindowResize(evt.resize.w, evt.resize.h);
                break;

            case SDL_MOUSEMOTION:
                OnMouseEvent(evt.type, evt.motion.state, evt.motion.x, evt.motion.y, evt.motion.xrel, evt.motion.yrel);
                break;

            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                OnMouseEvent(evt.type, evt.button.state, evt.button.x, evt.button.y, 0, 0);
                break;

            case SDL_QUIT:
                _quit = true;
                break;
        }
    }
}

void Engine::_CalcFPS(void)
{
    ++_framecounter;
    uint32 ms = clock();
    if(ms - _fpsclock >= CLOCKS_PER_SEC >> 1)
    {
        char buf[100];
        _fpsclock = ms;
        _fps = _framecounter << 1;
        _framecounter = 0;
        sprintf(buf, "%s - %u FPS - %u sleep", _wintitle.c_str(), _fps, _sleeptime);
        SDL_WM_SetCaption((const char*)buf, NULL);
        if(_fps > 70)
        {
            ++_sleeptime;
        }
        else if(_sleeptime && _fps < 60)
        {
            --_sleeptime;
        }
    }
    SDL_Delay(_sleeptime);
}

bool Engine::Setup(void)
{
    return true;
}

void Engine::_Process(uint32 ms)
{
    _layermgr->Update(GetCurFrameTime());
    objmgr->Update(ms);
}

// Handle a raw SDL_Event before anything else. return true for further processing,
// false to drop the event and NOT pass it to other On..Event functions
bool Engine::OnRawEvent(SDL_Event& evt)
{
    return true;
}

void Engine::OnWindowEvent(bool active)
{
    // TODO: pause if lost focus?
}

void Engine::OnMouseEvent(uint32 type, uint32 button, uint32 x, uint32 y, int32 rx, int32 ry)
{
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
        uint32 flags = _screen->flags | _screenFlags;

        SDL_FreeSurface(_screen);

        // toggle between fullscreen, preserving other flags
        if(flags & SDL_FULLSCREEN)
            flags &= ~SDL_FULLSCREEN; // remove fullscreen flag
        else
            flags |= SDL_FULLSCREEN; // add fullscreen flag

        InitScreen(x, y, bpp, flags);
    }

}

void Engine::OnKeyUp(SDLKey key, SDLMod mod)
{
}

void Engine::OnWindowResize(uint32 newx, uint32 newy)
{
    SDL_SetVideoMode(newx,newy,GetBPP(), GetSurface()->flags);
}

void Engine::_Render(void)
{
    //RenderBackground();
    _layermgr->Render();
    // TODO: render sprites
    SDL_Flip(_screen);
}

// returns the boundaries of the currently visible 16x16 pixel blocks
SDL_Rect *Engine::GetVisibleBlockRect(void)
{
    _visibleBlockRect.x = GetCameraPos().x >> 4; // (x / 16) (16 = block size in pixels)
    _visibleBlockRect.y = GetCameraPos().y >> 4;
    _visibleBlockRect.w = (GetResX() >> 4) + 1;
    _visibleBlockRect.h = (GetResY() >> 4) + 1;
    return &_visibleBlockRect;
}
