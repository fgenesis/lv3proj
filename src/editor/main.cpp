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

    try
    {
        editor.InitScreen(1000,600,0,SDL_RESIZABLE);
        editor.SetTitle("Lost Vikings 3 Project - Level Editor");
        editor.Setup();
        editor.Run();
    }
    catch(gcn::Exception ex)
    {
        logerror("An unhandled gcn::Exception occurred! Infos:");
        logerror("File: %s:%u", ex.getFilename().c_str(), ex.getLine());
        logerror("Function: %s", ex.getFunction().c_str());
        logerror("Message: %s", ex.getMessage().c_str());
        getchar();
    }
    /*catch(...)
    {
        logerror("Unhandled unknown Exception");
        getchar();
    }*/
    return 0;
}