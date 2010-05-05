#ifndef FALCON_GAME_MODULE_H
#define FALCON_GAME_MODULE_H

#include <falcon/engine.h>
#include "Objects.h"

Falcon::Module *FalconGameModule_create(void);
void FalconGameModule_SetEnginePtr(Engine *e);



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
