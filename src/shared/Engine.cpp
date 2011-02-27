#include "common.h"
#include "SDLImageLoaderManaged.h"
#include "SDLImageManaged.h"
#include "AnimParser.h"
#include "ResourceMgr.h"
#include "LayerMgr.h"
#include "SoundCore.h"
#include "AsciiLevelParser.h"
#include "Engine.h"
#include "PhysicsSystem.h"
#include "ObjectMgr.h"
#include "MyCrc32.h"


volatile uint32 Engine::s_curFrameTime; // game time
volatile uint32 Engine::s_lastFrameTime; // last frame's SDL_GetTicks()
bool Engine::_quit;
std::vector<SDL_Joystick*> Engine::s_joysticks;
Engine *Engine::s_instance = NULL;

Engine::Engine()
: _screen(NULL), _fps(0), _sleeptime(0), _framecounter(0), _paused(false),
_debugFlags(EDBG_NONE), _reset(false), _bgcolor(0), _drawBackground(true),
_fpsMin(60), _fpsMax(70), falcon(NULL), _mouseX(0), _mouseY(0)
{
    log("Game Engine start.");
    s_instance = this;

    _gcnImgLoader = new gcn::SDLImageLoaderManaged();
    _gcnGfx = new gcn::SDLGraphics();
    gcn::Image::setImageLoader(_gcnImgLoader);

    _quit = false;
    _layermgr = new LayerMgr(this);
    _fpsclock = s_lastFrameTime = SDL_GetTicks();
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
    for(uint32 i = 0; i < s_joysticks.size(); ++i)
        if(s_joysticks[i] && SDL_JoystickOpened(SDL_JoystickIndex(s_joysticks[i]))) // this is maybe a bit overcomplicated, but safe at least
            SDL_JoystickClose(s_joysticks[i]);

    delete objmgr;
    delete physmgr;
    delete _layermgr;
    resMgr.pool.Cleanup(true); // force deletion of everything
    resMgr.DropUnused(); // at this point, all resources should have a refcount of 0, so this removes all.
    sndCore.Destroy(); // must be deleted after all sounds were dropped by the ResourceMgr
    if(_screen)
        SDL_FreeSurface(_screen);
}

void Engine::Shutdown(void)
{
    // this should not be called from inside Engine::Run()
}

void Engine::_OnSignal(int s)
{
    switch(s)
    {
    case SIGINT:
    case SIGQUIT:
    case SIGTERM:
    case SIGABRT:
#ifdef _WIN32
    case SIGBREAK:
#endif
        SetQuit(true);
        break;
    }
}

void Engine::HookSignals(void)
{
    signal(SIGINT, &Engine::_OnSignal);
    signal(SIGQUIT, &Engine::_OnSignal);
    signal(SIGTERM, &Engine::_OnSignal);
    signal(SIGABRT, &Engine::_OnSignal);
#ifdef _WIN32
    signal(SIGBREAK, &Engine::_OnSignal);
#endif
}

void Engine::UnhookSignals(void)
{
    signal(SIGINT, 0);
    signal(SIGQUIT, 0);
    signal(SIGTERM, 0);
    signal(SIGABRT, 0);
#ifdef _WIN32
    signal(SIGBREAK, 0);
#endif
}

void Engine::SetTitle(char *title)
{
    _wintitle = title;
}

void Engine::InitScreen(uint32 sizex, uint32 sizey, uint8 bpp /* = 0 */, uint32 extraflags /* = 0 */)
{
    if(sizex == GetResX() && sizey == GetResY() && (!bpp || bpp == GetBPP()) && ((_screen->flags | extraflags) == _screen->flags))
        return; // no change, nothing to do

    if(_screen)
        SDL_FreeSurface(_screen);
    _winsizex = sizex;
    _winsizey = sizey;
    _screenFlags = SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_ANYFORMAT | SDL_HWACCEL | extraflags;
    _screen = SDL_SetVideoMode(sizex, sizey, bpp, _screenFlags);
    _screenFlags &= ~SDL_FULLSCREEN; // this depends on current setting and should not be stored

    _gcnGfx->setTarget(GetSurface());
}

void Engine::_InitJoystick(void)
{
    uint32 num = SDL_NumJoysticks();
    s_joysticks.resize(num);
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
            s_joysticks[i] = jst;
        }
    }
    else
    {
        logdetail("No joysticks found.");
    }
}

bool Engine::_InitFalcon(void)
{
    return true; // nothing to do here
}

void Engine::Run(void)
{
    uint32 ms;
    uint32 diff;
    while(!_quit)
    {
        if(IsReset())
            _Reset();

        ms = SDL_GetTicks();
        diff = ms - s_lastFrameTime;
        if(diff > 127) // 127 ms max. allowed diff time
            diff = 127;
        _ProcessEvents();
        if(!_paused)
        {
            s_curFrameTime += diff;
            _Process(diff);
            if(_screen)
                _Render();
        }

        _CalcFPS();
        s_lastFrameTime = ms;
    }
}

void Engine::_ProcessEvents(void)
{
    SDL_Event evt;
    while(!_quit && SDL_PollEvent(&evt))
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
                SetQuit(true);
                break;
        }
    }
}

void Engine::_CalcFPS(void)
{
    ++_framecounter;
    uint32 ms = SDL_GetTicks();
    if(ms - _fpsclock >= 1000 >> 2)
    {
        char buf[100];
        _fpsclock = ms;
        _fps = _framecounter << 2;
        _framecounter = 0;
        sprintf(buf, "%s - %u FPS - %u sleep [%u .. %u]%s", _wintitle.c_str(), _fps, _sleeptime, _fpsMin, _fpsMax, FrameLimit() ? "" : " (no limit)");
        SDL_WM_SetCaption((const char*)buf, NULL);
        if(FrameLimit())
        {
            if(_fps > _fpsMax)
            {
                ++_sleeptime;
            }
            else if(_sleeptime && _fps < _fpsMin)
            {
                --_sleeptime;
            }
        }
        else
            _sleeptime = 0;
    }
    if(FrameLimit())
        SDL_Delay(_sleeptime);
}

bool Engine::Setup(void)
{
    return _InitFalcon();
}

void Engine::_Process(uint32 ms)
{
    _layermgr->Update(GetCurFrameTime());
    objmgr->Update(ms);

    // TODO: do not call this every frame!
    resMgr.pool.Cleanup();
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
    _mouseX = x;
    _mouseY = y;
}

void Engine::OnJoystickEvent(uint32 type, uint32 device, uint32 id, int32 val)
{
}

void Engine::OnKeyDown(SDLKey key, SDLMod mod)
{
    if(mod & KMOD_LALT)
    {
        if(key == SDLK_F4)
            SetQuit(true);
        else if(key == SDLK_RETURN)
            ToggleFullscreen();
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
        TogglePause();
    }
}

void Engine::OnKeyUp(SDLKey key, SDLMod mod)
{
}

void Engine::OnWindowResize(uint32 newx, uint32 newy)
{
    SDL_SetVideoMode(newx,newy,GetBPP(), GetSurface()->flags);
}

void Engine::OnObjectCreated(BaseObject *obj)
{
}

void Engine::_Render(void)
{
    if(_drawBackground)
        SDL_FillRect(GetSurface(), NULL, _bgcolor);

    _layermgr->Render();

    _PostRender();
}

void Engine::_PostRender(void)
{
    SDL_Flip(_screen);
}

// returns the boundaries of the currently visible 16x16 pixel blocks
SDL_Rect *Engine::GetVisibleBlockRect(void)
{
    uint32 posx = GetCameraPos().x < 0 ? 0 : GetCameraPos().x;
    uint32 posy = GetCameraPos().y < 0 ? 0 : GetCameraPos().y;
    _visibleBlockRect.x = posx / 16;
    _visibleBlockRect.y = posy / 16;
    _visibleBlockRect.w = ((GetResX() + posx) / 16) + 1;
    _visibleBlockRect.h = ((GetResY() + posy) / 16) + 1;
    return &_visibleBlockRect;
}

gcn::Font *Engine::LoadFont(const char *infofile, const char *gfxfile)
{
    memblock *fontinfo = resMgr.LoadTextFile((char*)infofile);
    if(!fontinfo)
    {
        logerror("EditorEngine::Setup: Can't load font infos (%s)", infofile);
        return false;
    }
    std::string glyphs((char*)(fontinfo->ptr));
    logdetail("Using font glyphs for '%s':", gfxfile);
    logdetail("%s", glyphs.c_str());

    gcn::Font *font = NULL;
    try
    {
        font = new gcn::ImageFont(gfxfile, glyphs);
    }
    catch(gcn::Exception ex)
    {
        delete font;
        font = NULL;
    }
    resMgr.Drop(fontinfo);

    return font;
}

void Engine::_Reset(void)
{
    DEBUG(logdetail("Before Reset Cleanup: Memory leak detector says: %u", MLD_COUNTER));
    _reset = false;
    objmgr->RemoveAll();
    _layermgr->Clear();
    physmgr->SetDefaults();
    resMgr.pool.Cleanup();
    resMgr.DropUnused();
    DEBUG(logdetail("After Reset Cleanup: Memory leak detector says: %u", MLD_COUNTER));
    resMgr.vfs.Prepare(true);
    resMgr.vfs.Reload(true);
}

void Engine::SetFullscreen(bool b)
{
    if(b == IsFullscreen())
        return; // no change required

    uint32 flags = _screen->flags | _screenFlags;

    // toggle between fullscreen, preserving other flags
    if(b)
        flags |= SDL_FULLSCREEN; // add fullscreen flag
    else
        flags &= ~SDL_FULLSCREEN; // remove fullscreen flag

    InitScreen(GetResX(), GetResY(), GetBPP(), flags);
}

void Engine::SetResizable(bool b)
{
    if(b == IsResizable())
        return; // no change required

    uint32 flags = _screen->flags | _screenFlags;

    // toggle between fullscreen, preserving other flags
    if(b)
        flags |= SDL_RESIZABLE; // add resizable flag
    else
        flags &= ~SDL_RESIZABLE; // remove resizable flag

    InitScreen(GetResX(), GetResY(), GetBPP(), flags);
}

void Engine::PrintSystemSpecs(void)
{
    logcustom(0, LGREEN, "System/Engine specs:");
    logcustom(0, LGREEN, "----------------------------------");
    logcustom(0, LGREEN, "Platform: %s", PLATFORM_NAME);
    logcustom(0, LGREEN, "Compiler: %s (" COMPILER_VERSION_OUT ")", COMPILER_NAME, COMPILER_VERSION);
    logcustom(0, LGREEN, "Endian:   %s", IS_LITTLE_ENDIAN ? "little" : "big");
    logcustom(0, LGREEN, "Bits:     %u", SYSTEM_BITS);
    logcustom(0, LGREEN, "----------------------------------");
#ifdef _DEBUG
    logcustom(0, LGREEN, "uint8 size:  %u%s", sizeof(uint8), sizeof(uint8) == 1 ? "" : " [WRONG, should be 1]");
    logcustom(0, LGREEN, "uint16 size: %u%s", sizeof(uint16), sizeof(uint16) == 2 ? "" : " [WRONG, should be 2]");
    logcustom(0, LGREEN, "uint32 size: %u%s", sizeof(uint32), sizeof(uint32) == 4 ? "" : " [WRONG, should be 4]");
    logcustom(0, LGREEN, "uint64 size: %u%s", sizeof(uint64), sizeof(uint64) == 8 ? "" : " [WRONG, should be 8]");
    logcustom(0, LGREEN, "int size:    %u", sizeof(int)); // int size is officially compiler/OS dependant, its here just for reference
    logcustom(0, LGREEN, "float size:  %u%s", sizeof(float), sizeof(float) == 4 ? "" : " [WRONG, should be 4]");
    logcustom(0, LGREEN, "double size: %u%s", sizeof(double), sizeof(double) == 8 ? "" : " [WRONG, should be 8]");
    logcustom(0, LGREEN, "----------------------------------");
    logcustom(0, LGREEN, "Falcon::uint8 size:   %u%s", sizeof(Falcon::uint8), sizeof(Falcon::uint8) == 1 ? "" : " [WRONG, should be 1]");
    logcustom(0, LGREEN, "Falcon::uint16 size:  %u%s", sizeof(Falcon::uint16), sizeof(Falcon::uint16) == 2 ? "" : " [WRONG, should be 2]");
    logcustom(0, LGREEN, "Falcon::uint32 size:  %u%s", sizeof(Falcon::uint32), sizeof(Falcon::uint32) == 4 ? "" : " [WRONG, should be 4]");
    logcustom(0, LGREEN, "Falcon::uint64 size:  %u%s", sizeof(Falcon::uint64), sizeof(Falcon::uint64) == 8 ? "" : " [WRONG, should be 8]");
    logcustom(0, LGREEN, "Falcon::numeric size: %u%s", sizeof(Falcon::numeric), sizeof(Falcon::numeric) == 8 ? "" : " [WRONG, should be 4]");
    logcustom(0, LGREEN, "----------------------------------");
#endif
}

bool Engine::RelocateWorkingDir(void)
{
    std::string procdir = GetProgramDir();
    if(procdir.length())
    {
        if(SetWorkingDir(procdir))
        {
            logdetail("Working directory successfully changed to program directory:");
            logdetail(" '%s'", procdir.c_str());
            return true;
        }
        else
        {
            logerror("Unable to change working directory to program directory:");
            logerror(" '%s'", procdir.c_str());
        }
    }
    else
    {
        logerror("WARNING: Unable to detect program directory! Be sure to run this from the correct path or set the working dir manually, "
            "otherwise the engine may not find its data and will be unable to start up!");
    }
    return false;
}
