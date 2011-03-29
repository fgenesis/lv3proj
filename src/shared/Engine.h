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
    inline bool IsQuit(void) { return _quit; }
    inline static void SetQuit(bool q = true) { _quit = q; }
    inline static SDL_Joystick *GetJoystick(uint32 i) { return i < s_joysticks.size() ? s_joysticks[i] : NULL; }
    inline static uint32 GetJoystickCount(void) { return s_joysticks.size(); }
    inline static Engine *GetInstance(void) { return s_instance; }
    inline static uint32 GetCurFrameTime(void) { return s_curFrameTime; }

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
    virtual void _Process(uint32 ms);
    virtual void _Reset(void);
    virtual bool _InitFalcon(void);

    gcn::SDLGraphics* _gcnGfx;
    gcn::SDLImageLoader* _gcnImgLoader;

    std::string _wintitle;
    SDL_Surface *_screen;
    uint32 _screenFlags; // stores surface flags set on screen creation
    static volatile uint32 s_curFrameTime, s_lastFrameTime;
    SDL_Rect _visibleBlockRect;
    uint32 _winsizex;
    uint32 _winsizey;
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
    

private:
    static void _InitJoystick(void); // this does nothing if joystick support was not explicitly initialized in SDL_Init()
    static void _OnSignal(int s);
    static Engine *s_instance;
    static bool _quit;
};

#endif
