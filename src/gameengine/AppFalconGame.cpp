#include "common.h"
#include "GameEngine.h"
#include "AppFalconGame.h"

#include "FalconGameModule.h"

AppFalconGame::AppFalconGame() : AppFalcon()
{
}

void AppFalconGame::_LoadModules(void)
{
    AppFalcon::_LoadModules();
    _LinkModule(FalconGameModule_create());
}
