#ifndef ENGINE_H
#define ENGINE_H

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>


class LayerMgr;


struct Point
{
    Point() : x(0), y(0) {}
    Point(uint32 x_, uint32 y_) : x(x_), y(y_) {}
    uint32 x;
    uint32 y;
};

class Engine
{
public:
    Engine();
    virtual ~Engine();

    virtual void InitScreen(uint32 sizex, uint32 sizey, uint8 bpp = 0, bool fullscreen = false, uint32 extraflags = 0);
    virtual bool Setup(void);

    virtual void OnMouseEvent(uint32 button, uint32 x, uint32 y, int32 rx, uint32 ry);
    virtual void OnKeyDown(SDLKey key, SDLMod mod);
    virtual void OnKeyUp(SDLKey key, SDLMod mod);
    virtual void OnWindowEvent(bool active);
    virtual void OnWindowResize(uint32 newx, uint32 newy);
    virtual bool OnRawEvent(SDL_Event& evt);


    inline uint32 GetResX(void) { return _screen->w; }
    inline uint32 GetResY(void) { return _screen->h; }
    inline uint8 GetBPP(void) { return _screen->format->BitsPerPixel; }
    inline Point GetCameraPos(void) { return _cameraPos; }
    inline SDL_Surface *GetSurface(void) { return _screen; }
    SDL_Rect *GetVisibleBlockRect(void);
    virtual void SetTitle(char *title);
    inline uint32 GetFPS(void) { return _fps; }
    inline static uint32 GetCurFrameTime(void) { return s_curFrameTime; }
    inline void SetSleepTime(uint32 t) { _sleeptime = t; }
    virtual void Run(void);

protected:

    LayerMgr *_layermgr;

    virtual void _ProcessEvents(void);
    virtual void _CalcFPS(void);
    virtual void _Render(void);
    virtual void _Process(uint32 ms);

    std::string _wintitle;
    SDL_Surface *_screen;
    static volatile uint32 s_curFrameTime;
    SDL_Rect _visibleBlockRect;
    uint32 _winsizex;
    uint32 _winsizey;
    uint32 _fps;
    uint32 _framecounter;
    uint32 _fpsclock;
    uint32 _sleeptime;
    Point _cameraPos; // camera / "screen anchor" position in 2D-space, top-left corner (starts with (0,0) )
    bool _quit;

};

#endif
