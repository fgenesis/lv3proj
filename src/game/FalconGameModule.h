#ifndef FALCON_GAME_MODULE_H
#define FALCON_GAME_MODULE_H

#include <falcon/engine.h>
#include "Objects.h"

Falcon::Module *FalconGameModule_create(void);
void FalconGameModule_SetEnginePtr(Engine *e);

enum CoreEventTypes
{
    EVENT_TYPE_KEYBOARD = 0,
    EVENT_TYPE_JOYSTICK_BUTTON = 1,
    EVENT_TYPE_JOYSTICK_AXIS = 2,
    EVENT_TYPE_JOYSTICK_HAT = 3
};



class GameError: public Falcon::Error
{
public:
    GameError():
      Falcon::Error( "GameError" )
      {}

      GameError( const Falcon::ErrorParam &params  ):
      Falcon::Error( "GameError", params )
      {}
};

#endif
