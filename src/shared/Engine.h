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
    void HookSignals(void);
    void UnhookSignals(void);

    virtual void InitScreen(uint32 sizex, uint32 sizey, uint8 bpp = 0, uint32 extraflags = 0);
    virtual bool Setup(void);
    virtual void Shutdown(void);
    inline bool IsQuit(void) { return _quit; }
    inline static void SetQuit(bool q = true) { _quit = q; }

    virtual void OnMouseEvent(uint32 type, uint32 button, uint32 state, uint32 x, uint32 y, int32 rx, int32 ry);
    virtual void OnKeyDown(SDLKey key, SDLMod mod);
    virtual void OnKeyUp(SDLKey key, SDLMod mod);
    virtual void OnJoystickEvent(uint32 type, uint32 device, uint32 id, int32 val);
    virtual void OnWindowEvent(bool active);
    virtual void OnWindowResize(uint32 newx, uint32 newy);
    virtual bool OnRawEvent(SDL_Event& evt); // return true to pass this event to the following internal event handlers, false to proceed with next event


    inline uint32 GetResX(void) { return _screen->w; }
    inline uint32 GetResY(void) { return _screen->h; }
    inline uint8 GetBPP(void) { return _screen->format->BitsPerPixel; }
    inline Point GetCameraPos(void) { return _cameraPos; }
    inline Point *GetCameraPosPtr(void) { return &_cameraPos; }
    inline SDL_Surface *GetSurface(void) { return _screen; }
    SDL_Rect *GetVisibleBlockRect(void);
    virtual void SetTitle(char *title);
    inline uint32 GetFPS(void) { return _fps; }
    inline static uint32 GetCurFrameTime(void) { return s_curFrameTime; }
    inline void SetSleepTime(uint32 t) { _sleeptime = t; }
    virtual void Run(void);
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
    virtual void _Process(uint32 ms);

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
    uint32 _fpsclock;
    uint32 _sleeptime;
    Point _cameraPos; // camera / "screen anchor" position in 2D-space, top-left corner (starts with (0,0) )
    bool _paused;
    uint32 _debugFlags;

private:
    static void _OnSignal(int s);
    void _InitJoystick(void); // this does nothing if joystick support was not explicitly initialized in SDL_Init()

    static bool _quit;
    
    

};

#endif
