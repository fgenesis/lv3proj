#include <SDL/SDL.h>
#include "common.h"

#include "EditorEngine.h"


int main(int argc, char *argv[])
{
    log_prepare("editor_log.txt", "w");
    log_setloglevel(3);
    SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO);
    SDL_EnableUNICODE(1);
    SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

    atexit(log_close);
    atexit(SDL_Quit);

    mtRandSeed(time(NULL));

    EditorEngine editor;

    editor.InitScreen(1000,600,0,false,SDL_RESIZABLE);
    editor.SetTitle("Lost Vikings 3 Project - Level Editor");
    editor.Setup();
    editor.Run();
    return 0;
}