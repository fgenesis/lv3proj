#include "common.h"
#include "AnimParser.h"
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
#include "SharedDefines.h"

GameEngine::GameEngine()
: Engine()
{
    falcon = new AppFalconGame(this);

    _playerCount = 1;

    // test
    mouseRect.w = 32;
    mouseRect.h = 32;
    mouseCollision = 0;
    bigRect.x = 150;
    bigRect.y = 450;
    bigRect.w = 100;
    bigRect.h = 100;
    collRect.w = 32;
    collRect.h = 32;
    collRectGood = false;
    checkDirection = DIRECTION_RIGHT;
}

GameEngine::~GameEngine()
{
    delete objmgr;
    delete physmgr;
}

void GameEngine::Shutdown(void)
{
    objmgr->RemoveAll(); // this will unbind all objects BEFORE dropping falcon
    delete falcon;
    Falcon::Engine::PerformGC();
    Falcon::Engine::Shutdown();
    Engine::Shutdown();
}

bool GameEngine::Setup(void)
{
    // initialize the falcon scripting engine & VM
    Falcon::Engine::Init();

    // load the initialization script
    char *initscript = resMgr.LoadTextFile("scripts/init.fal");
    falcon->Init(initscript);

    AsciiLevel *level = LoadAsciiLevel("levels/testlevel.txt");
    _layermgr->LoadAsciiLevel(level);
    delete level;

    char *testscript = resMgr.LoadTextFile("scripts/test.fal");
    falcon->EmbedStringAsModule(testscript, "testscript");
    
    //sndCore.PlayMusic("lv1_snes_ship.ogg");

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

void GameEngine::OnJoystickEvent(uint32 type, uint32 device, uint32 id, int32 val)
{
    // Engine::OnJoystickEvent(type, device, id, val); // the default engine is not interested in joysticks, this call can be skipped

    // pass joystick event to Falcon
    // TODO: cache this on init and call then without invoking findGlobalItem() all the time
    Falcon::Item *item = falcon->GetVM()->findGlobalItem("EventHandler");
    if(item && item->isCallable())
    {
        uint32 evt;

        // translate into custom enum values
        switch(type)
        {
            case SDL_JOYBUTTONDOWN:
            case SDL_JOYBUTTONUP:
                evt = EVENT_TYPE_JOYSTICK_BUTTON;
                break;

            case SDL_JOYAXISMOTION:
                evt = EVENT_TYPE_JOYSTICK_AXIS;
                break;

            case SDL_JOYHATMOTION:
                evt = EVENT_TYPE_JOYSTICK_HAT;
                break;

            default:
                logerror("GameEngine::OnJoystickEvent(): unprocessed type %u", type);
                return;
        }

        try
        {
            Falcon::CoreArray *arr = new Falcon::CoreArray(4);
            arr->append(Falcon::int32(evt));
            arr->append(Falcon::int32(device));
            arr->append(Falcon::int32(id)); // button, axis or hat id
            arr->append(Falcon::int32(val)); // button/hat: 1=pressed, 0=released; axis: value in -(2^15)..+(2^15)
            falcon->GetVM()->pushParam(arr);
            falcon->GetVM()->callItem(*item, 1);
        }
        catch(Falcon::Error *err)
        {
            Falcon::AutoCString edesc( err->toString() );
            logerror("GameEngine::OnJoystickEvent: %s", edesc.c_str());
            err->decref();
        }
    }
}

void GameEngine::OnMouseEvent(uint32 type, uint32 button, uint32 state, uint32 x, uint32 y, int32 rx, int32 ry)
{
    // - TEST - this is all test stuff and will be removed later
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
        Point p = _layermgr->GetClosestNonCollidingPoint(&mouseRect, checkDirection);
        collRect.x = p.x;
        collRect.y = p.y;
        collRectGood = !_layermgr->CollisionWith(&collRect);

    }
    // on mouseclick, text for intersection with the big white rect
    if(type == SDL_MOUSEBUTTONDOWN)
    {
        if(button == 1)
        {
            uint8 side = mouseRect.CollisionWith(&bigRect);
            char *sidestr;
            switch(side)
            {
                case SIDE_TOP: sidestr = "top"; break;
                case SIDE_BOTTOM: sidestr = "bottom"; break;
                case SIDE_LEFT: sidestr = "left"; break;
                case SIDE_RIGHT: sidestr = "right"; break;
                default: sidestr = "none"; break;
            }
            logerror("(%d, %d) COLLISION: %s (%u)", int(mouseRect.x), int(mouseRect.y), sidestr, side);
        }
        else if(button == 3)
        {
            switch(checkDirection)
            {
                case DIRECTION_RIGHT: checkDirection = DIRECTION_DOWNRIGHT; break;
                case DIRECTION_DOWNRIGHT: checkDirection = DIRECTION_DOWN; break;
                case DIRECTION_DOWN: checkDirection = DIRECTION_DOWNLEFT; break;
                case DIRECTION_DOWNLEFT: checkDirection = DIRECTION_LEFT; break;
                case DIRECTION_LEFT: checkDirection = DIRECTION_UPLEFT; break;
                case DIRECTION_UPLEFT: checkDirection = DIRECTION_UP; break;
                case DIRECTION_UP: checkDirection = DIRECTION_UPRIGHT; break;
                case DIRECTION_UPRIGHT: checkDirection = DIRECTION_RIGHT; break;
            }
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

    // part 1 - draw a huge rect, centered
    SDL_Rect bigone;
    bigone.x = bigRect.x;
    bigone.y = bigRect.y;
    bigone.w = bigRect.w;
    bigone.h = bigRect.h;
    SDL_FillRect(GetSurface(), &bigone, 0xFFFFFFFF);

    // part 2 - draw the small rect that follows the mouse cursor
    SDL_Rect mrect;
    mrect.x = mouseRect.x;
    mrect.y = mouseRect.y;
    mrect.w = mouseRect.w;
    mrect.h = mouseRect.h;
    if(mouseCollision == 2) // collision
        SDL_FillRect(GetSurface(), &mrect, SDL_MapRGB(GetSurface()->format,0xFF,0,0));
    else if(mouseCollision == 1) // can stand
        SDL_FillRect(GetSurface(), &mrect, SDL_MapRGB(GetSurface()->format,0xFF,0xFF,0));
    else if(mouseCollision == 0) // floating
        SDL_FillRect(GetSurface(), &mrect, SDL_MapRGB(GetSurface()->format,0,0xFF,0));

    SDL_Rect cdrect;
    cdrect.x = collRect.x;
    cdrect.y = collRect.y;
    cdrect.w = collRect.w;
    cdrect.h = collRect.h;
    SDL_FillRect(GetSurface(), &cdrect, collRectGood ? 0xFF0000FF : 0xFFFF00CC);
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
