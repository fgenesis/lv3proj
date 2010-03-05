#include "common.h"
#include "Animparser.h"
#include "ResourceMgr.h"
#include "LayerMgr.h"
#include "SoundCore.h"
#include "AsciiLevelParser.h"
#include "AppFalconGame.h"
#include "Objects.h"
#include "ObjectMgr.h"
#include "GameEngine.h"
#include <falcon/engine.h>
#include "FalconGameModule.h"

GameEngine::GameEngine()
: Engine()
{
    objmgr = new ObjectMgr;
    falcon = new AppFalconGame;
    _playerCount = 1;

    // test
    mouseRect.w = 32;
    mouseRect.h = 32;
    mouseCollision = 0;
}

GameEngine::~GameEngine()
{
    objmgr->RemoveAll(true, &BaseObject::unbind);
    delete objmgr;
    delete falcon;
}

bool GameEngine::Setup(void)
{
    // initialize the falcon scripting engine & VM
    falcon->Init();

    // load the initialization script
    char *initscript = resMgr.LoadTextFile("scripts/init.fal", "rb");
    falcon->EmbedStringAsModule(initscript, "initscript");

    AsciiLevel *level = LoadAsciiLevel("levels/testlevel.txt");
    _layermgr->LoadAsciiLevel(level);
    delete level;

    char *testscript = resMgr.LoadTextFile("scripts/test.fal", "rb");
    falcon->EmbedStringAsModule(testscript, "testscript");
    
    sndCore.PlayMusic("lv1_snes_ship.ogg");

    return true;
}

void GameEngine::Quit(void)
{
    // TODO: save stuff like settings
    _quit = true;
}

void GameEngine::OnKeyDown(SDLKey key, SDLMod mod)
{
    Engine::OnKeyDown(key, mod);

    // pass keypress to Falcon
    // TODO: cache this on init and call then without invoking findGlobalItem() all the time
    Falcon::Item *item = falcon->GetVM()->findGlobalItem("EventHandler");
    if(item && item->isCallable())
    {
        try
        {
            Falcon::CoreArray *arr = new Falcon::CoreArray(4);
            arr->append(Falcon::int32(EVENT_TYPE_KEYBOARD));
            arr->append(Falcon::int32(0));
            arr->append(Falcon::int32(key));
            arr->append(Falcon::int32(1)); // pressed
            falcon->GetVM()->pushParam(arr);
            falcon->GetVM()->callItem(*item, 1);
        }
        catch(Falcon::Error *err)
        {
            Falcon::AutoCString edesc( err->toString() );
            logerror("GameEngine::OnKeyDown: %s", edesc.c_str());
            err->decref();
        }
    }
}

void GameEngine::OnKeyUp(SDLKey key, SDLMod mod)
{
    Engine::OnKeyUp(key, mod);

    // pass keypress to Falcon
    // TODO: cache this on init and call then without invoking findGlobalItem() all the time
    Falcon::Item *item = falcon->GetVM()->findGlobalItem("EventHandler");
    if(item && item->isCallable())
    {
        try
        {
            Falcon::CoreArray *arr = new Falcon::CoreArray(4);
            arr->append(Falcon::int32(EVENT_TYPE_KEYBOARD));
            arr->append(Falcon::int32(0));
            arr->append(Falcon::int32(key));
            arr->append(Falcon::int32(0)); // released
            falcon->GetVM()->pushParam(arr);
            falcon->GetVM()->callItem(*item, 1);
        }
        catch(Falcon::Error *err)
        {
            Falcon::AutoCString edesc( err->toString() );
            logerror("GameEngine::OnKeyDown: %s", edesc.c_str());
            err->decref();
        }
    }
}

void GameEngine::OnMouseEvent(uint32 button, uint32 x, uint32 y, int32 rx, int32 ry)
{
    if(x >= uint32(mouseRect.w / 2) && y >= uint32(mouseRect.h / 2))
    {
        mouseRect.x = x - mouseRect.w / 2;
        mouseRect.y = y - mouseRect.h / 2;
        mouseCollision = _layermgr->CollisionWith(&mouseRect) ? 2 : 0;
        if(!mouseCollision)
        {
            if(!_layermgr->CanFallDown(Point(x, y + (mouseRect.h / 2) - 1), 12))
                mouseCollision = 1;
        }
    }
}

void GameEngine::_Render(void)
{
    // blank screen
    SDL_FillRect(GetSurface(), NULL, 0); // TODO: remove this as soon as backgrounds are implemented!! (eats CPU)
    //RenderBackground();

    // render the layers
    _layermgr->Render();

    // TEST/DEBUG
    if(mouseCollision == 2) // collision
        SDL_FillRect(GetSurface(), &mouseRect.AsSDLRect(), SDL_MapRGB(GetSurface()->format,0xFF,0,0));
    else if(mouseCollision == 1) // can stand
        SDL_FillRect(GetSurface(), &mouseRect.AsSDLRect(), SDL_MapRGB(GetSurface()->format,0xFF,0xFF,0));
    else if(mouseCollision == 0) // floating
        SDL_FillRect(GetSurface(), &mouseRect.AsSDLRect(), SDL_MapRGB(GetSurface()->format,0,0xFF,0));
    // -end-

    SDL_Flip(_screen);
}

void GameEngine::_Process(uint32 ms)
{
    // TODO: cache this on init and call then without invoking findGlobalItem() all the time
    Falcon::Item *item = falcon->GetVM()->findGlobalItem("GameUpdate");
    if(item && item->isCallable())
    {
        try
        {
            falcon->GetVM()->pushParam(Falcon::int64(ms));
            falcon->GetVM()->callItem(*item, 1);
        }
        catch(Falcon::Error *err)
        {
            Falcon::AutoCString edesc( err->toString() );
            logerror("GameEngine::_Process: %s", edesc.c_str());
            err->decref();
        }

    }
    
    Engine::_Process(ms);
}
