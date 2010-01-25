#ifndef ENGINE_H
#define ENGINE_H

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#include "TileMgr.h"

/* // -- not used, maybe later?
enum SurfaceLayer
{
    LAYER_BACKGROUND = 0, // background, can be animated (spaceship, for example)
    LAYER_STATICTILES,    // foreground, static tiles (to be drawn on a singe surface and only updated if needed)
    LAYER_DYNAMICTILES,   // animated tiles, to be drawn on every frame
    LAYER_SPRITES,        // sprite layer (monsters, items, switches, vikings, effects, etc)

    LAYER_MAX             // do not use
};
*/

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
    ~Engine();

    void InitScreen(uint32 sizex, uint32 sizey, uint8 bpp = 0, bool fullscreen = false);
    bool Setup(void);

    void OnMouseEvent(uint32 button, uint32 x, uint32 y, int32 rx, uint32 ry);
    void OnKeyDown(SDLKey key, SDLMod mod);
    void OnKeyUp(SDLKey key, SDLMod mod);
    void OnWindowEvent(bool active);


    inline uint32 GetResX(void) { return _screen->w; }
    inline uint32 GetResY(void) { return _screen->h; }
    inline uint8 GetBPP(void) { return _screen->format->BitsPerPixel; }
    inline Point GetCameraPos(void) { return _cameraPos; }
    inline SDL_Surface *GetSurface(void) { return _screen; }
    void SetTitle(char *title);
    inline uint32 GetFPS(void) { return _fps; }
    inline static uint32 GetCurFrameTime(void) { return s_curFrameTime; }
    inline void SetSleepTime(uint32 t) { _sleeptime = t; }
    void Run(void);

private:

    TileMgr *_tilemgr;

    void _ProcessEvents(void);
    void _CalcFPS(void);
    void _Render(void);
    void _Process(uint32 ms);

    std::string _wintitle;
    SDL_Surface *_screen;
    static volatile uint32 s_curFrameTime;
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
