#include "common.h"
#include "AnimParser.h"
#include "ResourceMgr.h"
#include "LayerMgr.h"
#include "SoundCore.h"
#include "AsciiLevelParser.h"
#include "Engine.h"
#include "PhysicsSystem.h"
#include "ObjectMgr.h"
#include "Crc32.h"


volatile uint32 Engine::s_curFrameTime; // game time
volatile uint32 Engine::s_lastFrameTime; // last frame's clock()

Engine::Engine()
: _screen(NULL), _fps(0), _sleeptime(0), _quit(false), _framecounter(0), _paused(false),
_debugFlags(EDBG_NONE)
{
    log("Game Engine start.");

    _layermgr = new LayerMgr(this);
    _fpsclock = s_lastFrameTime = clock();
    s_curFrameTime = 0;

    physmgr = new PhysicsMgr;
    physmgr->SetLayerMgr(_layermgr);
    objmgr = new ObjectMgr(this);
    physmgr->SetObjMgr(objmgr);
    objmgr->SetLayerMgr(_layermgr);
    objmgr->SetPhysicsMgr(physmgr);
    _InitJoystick();
}

Engine::~Engine()
{
    delete objmgr;
    delete physmgr;
    delete _layermgr;
    sndCore.Destroy();
    resMgr.DropUnused(); // at this point, all resources should have a refcount of 0, so this removes all.
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

void Engine::_InitJoystick(void)
{
    uint32 num = SDL_NumJoysticks();
    SDL_Joystick *jst = NULL;
    if(num)
    {
        logdetail("Found %u joysticks, enumerating...", num);
        SDL_JoystickEventState(SDL_ENABLE);
        for(uint32 i = 0; i < num; ++i)
        {
            jst = SDL_JoystickOpen(i); // TODO: do opened joysticks have to be closed explicitly? my guess is that SDL does it during cleanup/shutdown...
            if(jst)
            {
                log("Found joystick #%u (%s) with %u axes, %u buttons, %u hats, %u balls", i, SDL_JoystickName(i),
                    SDL_JoystickNumAxes(jst), SDL_JoystickNumButtons(jst), SDL_JoystickNumHats(jst), SDL_JoystickNumBalls(jst));
                SDL_JoystickEventState(SDL_ENABLE); // there is at least 1 active joystick, activate event mode
            }
            else
            {
                logerror("Found joystick #%u (%s), but failed to initialize!", i, SDL_JoystickName(i));
            }
        }
    }
    else
    {
        logdetail("No joysticks found.");
    }
}

void Engine::Run(void)
{
    uint32 ms;
    uint32 diff;
    while(!_quit)
    {
        ms = clock();
        diff = ms - s_lastFrameTime;
        diff &= 0x7F; // 127 ms max. allowed diff time
        _ProcessEvents();
        if(!_paused)
        {
            s_curFrameTime += diff;
            _Process(diff);
            _Render();
        }

        _CalcFPS();
        s_lastFrameTime = ms;
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

            case SDL_JOYAXISMOTION:
                OnJoystickEvent(evt.jaxis.type, evt.jaxis.which, evt.jaxis.axis, evt.jaxis.value);
                break;

            case SDL_JOYBUTTONDOWN:
            case SDL_JOYBUTTONUP:
                OnJoystickEvent(evt.jbutton.type, evt.jbutton.which, evt.jbutton.button, evt.jbutton.state);
                break;

            case SDL_JOYHATMOTION:
                OnJoystickEvent(evt.jhat.type, evt.jhat.which, evt.jhat.hat, evt.jhat.value);
                break;

            case SDL_ACTIVEEVENT:
                OnWindowEvent(evt.active.gain);
                break;

            case SDL_VIDEORESIZE:
                OnWindowResize(evt.resize.w, evt.resize.h);
                break;

            case SDL_MOUSEMOTION:
                OnMouseEvent(evt.type, 0, evt.motion.state, evt.motion.x, evt.motion.y, evt.motion.xrel, evt.motion.yrel);
                break;

            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                OnMouseEvent(evt.type, evt.button.button, evt.button.state, evt.button.x, evt.button.y, 0, 0);
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
    if(ms - _fpsclock >= CLOCKS_PER_SEC >> 2)
    {
        char buf[100];
        _fpsclock = ms;
        _fps = _framecounter << 2;
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

void Engine::OnMouseEvent(uint32 type, uint32 button, uint32 state, uint32 x, uint32 y, int32 rx, int32 ry)
{
}

void Engine::OnJoystickEvent(uint32 type, uint32 device, uint32 id, int32 val)
{
}

void Engine::OnKeyDown(SDLKey key, SDLMod mod)
{
    if(mod & KMOD_LALT)
    {

        if(key == SDLK_F4)
            _quit = true;
        if(key == SDLK_RETURN)
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
    if(mod & KMOD_CTRL)
    {
        if(key == SDLK_F1)
            ToggleDebugFlag(EDBG_COLLISION_MAP_OVERLAY);
        else if(key == SDLK_F2)
            ToggleDebugFlag(EDBG_HIDE_SPRITES);
        else if(key == SDLK_F3)
            ToggleDebugFlag(EDBG_HIDE_LAYERS);
        else if(key == SDLK_F4)
            ToggleDebugFlag(EDBG_SHOW_BBOXES);
    }


    if(key == SDLK_PAUSE)
    {
        _paused = !_paused;
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
