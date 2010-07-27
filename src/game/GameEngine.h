#ifndef GAMEENGINE_H
#define GAMEENGINE_H

#include "Engine.h"
#include "Objects.h"

class AppFalconGame;
class ObjectMgr;
class PhysicsMgr;


class GameEngine : public Engine
{
public:
    GameEngine();
    ~GameEngine();

    virtual void OnMouseEvent(uint32 type, uint32 button, uint32 state, uint32 x, uint32 y, int32 rx, int32 ry);
    virtual void OnKeyDown(SDLKey key, SDLMod mod);
    virtual void OnKeyUp(SDLKey key, SDLMod mod);
    virtual void OnJoystickEvent(uint32 type, uint32 device, uint32 id, int32 val);
    //virtual void OnWindowEvent(bool active);

    virtual bool Setup(void);
    virtual void Quit(void);
    virtual void Shutdown(void);


protected:

    virtual void _Process(uint32 ms);
    virtual void _Render(void);

    // TEMP: for debugging/testing
#ifdef _DEBUG
    ActiveRect mouseRect;
    ActiveRect collRect;
    bool collRectGood;
    uint8 mouseCollision; // 0: floating; 1: standing; 2: collision
    uint8 checkDirection;
#endif

};

#endif
