#ifndef GAMEENGINE_H
#define GAMEENGINE_H

#include "Engine.h"


class GameEngine : public Engine
{
public:
    GameEngine();
    ~GameEngine();

    //virtual void OnMouseEvent(uint32 button, uint32 x, uint32 y, int32 rx, uint32 ry);
    virtual void OnKeyDown(SDLKey key, SDLMod mod);
    virtual void OnKeyUp(SDLKey key, SDLMod mod);
    //virtual void OnWindowEvent(bool active);

    virtual bool Setup(void);

};

#endif
