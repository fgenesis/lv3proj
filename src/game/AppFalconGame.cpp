#include "common.h"
#include "GameEngine.h"
#include "AppFalconGame.h"

#include "FalconGameModule.h"

AppFalconGame::AppFalconGame(Engine *e) : AppFalcon(e)
{
}

void AppFalconGame::_LoadModules(void)
{
    AppFalcon::_LoadModules();
    vm->link(FalconGameModule_create());
}
