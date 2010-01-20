#ifndef ENGINE_H
#define ENGINE_H

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#include "TileMgr.h"

class Engine
{
public:
    Engine();
    ~Engine();

    void InitScreen(uint32 sizex, uint32 sizey, uint8 bpp = 0, bool fullscreen = false);
    bool Setup(void);

    void OnMouseEvent(uint32 button, uint32 x, uint32 y, int32 rx, uint32 ry);
    void OnKeyDown(uint32 key);
    void OnKeyUp(uint32 key);
    void OnWindowEvent(bool active);


    inline SDL_Surface *GetSurface(void) { return _screen; }
    void SetTitle(char *title);
    inline uint32 GetFPS(void) { return _fps; }
    inline void SetSleepTime(uint32 t) { _sleeptime = t; }
    void Run(void);

private:

    TileMgr *_tilemgr;

    void _ProcessEvents(void);
    void _CalcFPS(void);
    void _Render(void);
    void _Process(uint32 ms);

    SDL_Event _event;
    std::string _wintitle;
    SDL_Surface *_screen;
    uint32 _winsizex;
    uint32 _winsizey;
    uint32 _fps;
    uint32 _framecounter;
    uint32 _fpsclock;
    uint32 _sleeptime;
    bool _quit;

};

#endif
