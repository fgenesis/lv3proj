#include "common.h"
#include "Engine.h"


int main(int argc, char **argv)
{
    log_prepare("game_log.txt", "w");
    SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_TIMER);

    atexit(log_close);
    atexit(SDL_Quit);

    mtRandSeed(time(NULL));

    Engine game;

    game.Setup();
    game.InitScreen(800,600);
    game.SetTitle("Lost Vikings 3 Project");

    game.Run();

    return 0;
}
