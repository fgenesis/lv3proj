#ifndef ENGINE_H
#define ENGINE_H

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <guichan.hpp>
#include <guichan/sdl.hpp>
#include "SDLImageLoaderManaged.h"
#include "SDLImageManaged.h"
#include "SharedStructs.h"


class LayerMgr;
class ObjectMgr;
class PhysicsMgr;
class AppFalcon;
class BaseObject;

enum EngineDebugFlags
{
    EDBG_NONE                   = 0x00,
    EDBG_COLLISION_MAP_OVERLAY  = 0x01,
    EDBG_HIDE_SPRITES           = 0x02,
    EDBG_HIDE_LAYERS            = 0x04,
    EDBG_SHOW_BBOXES            = 0x08
};

class Engine
{
public:
    Engine();
    virtual ~Engine();

    static void HookSignals(void);
    static void UnhookSignals(void);
    static void PrintSystemSpecs(void);
    static bool RelocateWorkingDir(void);
    inline static bool IsQuit(void) { return s_quit; }
    inline static void SetQuit(bool q = true) { s_quit = q; }
    inline static SDL_Joystick *GetJoystick(uint32 i) { return i < s_joysticks.size() ? s_joysticks[i] : NULL; }
    inline static uint32 GetJoystickCount(void) { return s_joysticks.size(); }
    inline static Engine *GetInstance(void) { return s_instance; }
    inline static void SetSpeed(float s) { s_speed = s; }
    inline static float GetSpeed(void) { return s_speed; }
    inline static uint32 GetCurFrameTime(void) { return s_curFrameTime; }
    inline static double GetCurFrameTimeF(void) { return s_curFrameTime + (double)s_accuTime; } // float is not precise enough, as this can get very high
    inline static uint32 GetTimeDiff(void) { return s_diffTime; } // 1000 == 1 second (scaled by speed)
    inline static float GetTimeDiffF(void) { return s_fracTime; } // 1.0f == 1 second (scaled by speed)
    inline static float GetTimeDiffReal(void) { return s_diffTimeReal; } // 1000 = 1 second (real)
    inline static uint32 GetTicks(void) { return SDL_GetTicks() - s_ignoredTicks; }
    static void ResetTime(void);

    virtual void InitScreen(uint32 sizex, uint32 sizey, uint8 bpp = 0, uint32 extraflags = 0);
    virtual bool Setup(void);
    virtual void Shutdown(void);
    virtual const char *GetName(void) { return "engine"; } // must be overloaded
    virtual void Run(void);

    virtual void OnMouseEvent(uint32 type, uint32 button, uint32 state, uint32 x, uint32 y, int32 rx, int32 ry);
    virtual void OnKeyDown(SDLKey key, SDLMod mod);
    virtual void OnKeyUp(SDLKey key, SDLMod mod);
    virtual void OnJoystickEvent(uint32 type, uint32 device, uint32 id, int32 val);
    virtual void OnWindowEvent(bool active);
    virtual void OnWindowResize(uint32 newx, uint32 newy);
    virtual bool OnRawEvent(SDL_Event& evt); // return true to pass this event to the following internal event handlers, false to proceed with next event
    virtual void OnObjectCreated(BaseObject *obj); // called in FalconObjectModule.cpp, fal_ObjectCarrier::init()
    virtual bool LoadMapFile(const char *fn);

    void SetTitle(const char *title);
    inline void SetReset(bool r = true) { _reset = r; }
    inline bool IsReset(void) { return _reset; }
    inline uint32 GetResX(void) { return _screen ? _screen->w : 0; }
    inline uint32 GetResY(void) { return _screen ? _screen->h : 0; }
    inline uint8 GetBPP(void) { return _screen ? _screen->format->BitsPerPixel : 0; }
    inline Camera GetCamera(void) const { return _cameraPos; }
    inline Camera *GetCameraPtr(void) { return &_cameraPos; }
    inline SDL_Surface *GetSurface(void) { return _screen; }
    SDL_Rect *GetVisibleBlockRect(void);
    inline uint32 GetFPS(void) { return _fps; }
    inline int32 GetMouseX(void) { return _mouseX + _cameraPos.x; }
    inline int32 GetMouseY(void) { return _mouseY + _cameraPos.y; }
    inline void SetSleepTime(uint32 t) { _sleeptime = t; }
    inline bool HasDebugFlag(uint32 flag) { return _debugFlags & flag; }
    inline void SetDebugFlag(uint32 flag) { _debugFlags |= flag; }
    inline void UnsetDebugFlag(uint32 flag) { _debugFlags &= ~flag; }
    inline void ToggleDebugFlag(uint32 flag) // do no use multiple flags for this!
    {
        if(HasDebugFlag(flag))
            UnsetDebugFlag(flag);
        else
            SetDebugFlag(flag);
    }
    inline void ToggleFullscreen(void)
    {
        SetFullscreen(!IsFullscreen());
    }
    void SetFullscreen(bool b);
    inline bool IsFullscreen(void) { return GetSurface()->flags & SDL_FULLSCREEN; }
    void SetResizable(bool b);
    inline bool IsResizable(void) { return GetSurface()->flags & SDL_RESIZABLE; }
    inline void SetDrawBG(bool b) { _drawBackground = b; }
    inline bool GetDrawBG(void) { return _drawBackground; }
    inline void SetBGColor(uint8 r, uint8 g, uint8 b) { _bgcolor = SDL_MapRGB(GetSurface()->format, r,g,b); }
    inline uint32 GetBGColor(void) { return _bgcolor; }
    inline void TogglePause(void) { _paused = !_paused; }
    inline bool IsPaused(void) { return _paused; }
    inline void SetPaused(bool b) { _paused = b; }

    inline bool FrameLimit(void) { return _fpsMax && _fpsMin <= _fpsMax; }
    inline void FrameLimitMin(uint32 fps) { _fpsMin = fps; }
    inline void FrameLimitMax(uint32 fps) { _fpsMax = fps; }

    inline LayerMgr *_GetLayerMgr(void) const { return _layermgr; }
    inline gcn::Graphics *GetGcnGfx(void) { return _gcnGfx; }

    gcn::Font *LoadFont(const char *infofile, const char *gfxfile);

    ObjectMgr *objmgr;
    PhysicsMgr *physmgr;
    AppFalcon *falcon;

protected:

    LayerMgr *_layermgr;

    virtual void _ProcessEvents(void);
    virtual void _CalcFPS(void);
    virtual void _Render(void);
    virtual void _PostRender(void);
    virtual void _Process(void);
    virtual void _Reset(void);
    virtual bool _InitFalcon(void);
    virtual void _Idle(uint32 ms);

    gcn::SDLGraphics* _gcnGfx;
    gcn::SDLImageLoader* _gcnImgLoader;

    // timers
    IntervalTimer _resPoolTimer;

    std::string _wintitle;
    SDL_Surface *_screen;
    uint32 _screenFlags; // stores surface flags set on screen creation
    
    SDL_Rect _visibleBlockRect;
    uint32 _fps;
    uint32 _framecounter;
    uint32 _fpsMin, _fpsMax; // lower/upper bound of frame limiter, it will try to keep the FPS in between
    uint32 _fpsclock;
    uint32 _sleeptime;
    uint32 _debugFlags;
    uint32 _bgcolor;
    int32 _mouseX, _mouseY;
    Camera _cameraPos; // camera / "screen anchor" position in 2D-space, top-left corner (starts with (0,0) )
    bool _paused;
    bool _reset;
    bool _drawBackground;

    static std::vector<SDL_Joystick*> s_joysticks;
    static volatile uint32 s_curFrameTime; // game time (scaled by speed)
    static volatile uint32 s_lastFrameTimeReal; // last frame's SDL_GetTicks() -- not scaled by speed!
    static float s_speed; // speed multiplicator, 1.0 = normal speed. should NOT be negative!
    static float s_accuTime; // accumulated time, for cases when s_speed is very small and the resulting frame time less then 1 ms
    static uint32 s_diffTime; // time diff per tick [uint32(s_fracTime)]
    static uint32 s_diffTimeReal; // time diff per tick, real time (not scaled by s_speed)
    static float s_fracTime; // (_diffTime * _speed) / 1000.0f
    

private:
    static void _InitJoystick(void); // this does nothing if joystick support was not explicitly initialized in SDL_Init()
    static void _OnSignal(int s);
    static Engine *s_instance;
    static bool s_quit;
    static uint32 s_ignoredTicks; // this value is subtracted from SDL_GetTicks(), to make the time start at 0 for every new Engine state. updated on Engine::_Reset().
};

#endif
