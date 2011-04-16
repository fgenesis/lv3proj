#include "common.h"
#include "GameEngine.h"

int main(int argc, char **argv)
{
    uint32 loglevel = 1;
    DEBUG(loglevel = 3);
    log_setloglevel(loglevel);

    GameEngine::RelocateWorkingDir(); // a pity that this has to be done before opening the log file ...

    log_prepare("game_log.txt", "w");
    
    GameEngine::PrintSystemSpecs();

    SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);
    SDL_EnableUNICODE(1);

    atexit(log_close);
    atexit(SDL_Quit);

    mtRandSeed(time(NULL));

    // the engine must be created *after* SDL is fully initialized!
    GameEngine engine;

    engine.HookSignals();
    engine.InitScreen(320,240,0,SDL_RESIZABLE);
    engine.SetTitle("LV3p Engine");
    if(!engine.Setup())
    {
        logerror("Failed to setup game engine. Exiting.");
        engine.UnhookSignals();
        return 1;
    }
    engine.Run();
    engine.UnhookSignals();
    engine.Shutdown();

    return 0;
}
