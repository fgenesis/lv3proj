#include "common.h"
#include "GameEngine.h"


int main(int argc, char **argv)
{
    log_prepare("game_log.txt", "w");
    log_setloglevel(3);
    SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_TIMER);
    SDL_EnableUNICODE(1);

    atexit(log_close);
    atexit(SDL_Quit);

    mtRandSeed(time(NULL));

    GameEngine game;

    game.InitScreen(1000,600);
    game.SetTitle("Lost Vikings 3 Project");
    game.Setup();
    game.Run();

    return 0;
}
