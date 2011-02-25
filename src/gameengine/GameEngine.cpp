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
#include "FalconObjectModule.h"
#include "SharedDefines.h"

#include "LVPAFile.h"

GameEngine::GameEngine()
: Engine(), _wasInit(false)
{
    falcon = new AppFalconGame(this);

    // test
#ifdef _DEBUG
    SetDebugFlag(EDBG_SHOW_BBOXES);
    mouseRect.w = 32;
    mouseRect.h = 32;
    mouseCollision = 0;
    collRect.w = 32;
    collRect.h = 32;
    collRectGood = false;
    checkDirection = DIRECTION_RIGHT;
#endif
}

GameEngine::~GameEngine()
{
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
    if(!Engine::Setup())
        return false;

    if(!_wasInit)
    {
        logdetail("Initializing Falcon...");
        // initialize the falcon scripting engine & VM
        Falcon::Engine::Init();

        _wasInit = true;
    }

    // setup the VFS and the container to read from
    logdetail("Initializing virtual file system...");
    LVPAFile *basepak = new LVPAFileReadOnly;
    basepak->LoadFrom("basepak.lvpa", LVPALOAD_SOLID);
    resMgr.vfs.LoadBase(basepak, true);
    resMgr.vfs.LoadFileSysRoot();
    resMgr.vfs.Prepare();

    // load "loading" background
    if(GetSurface())
    {
        SDL_Surface *loadingbg = resMgr.LoadImg("misc/loadingbg.png");
        if(loadingbg)
        {
            SDL_BlitSurface(loadingbg, NULL, GetSurface(), NULL);
            SDL_Flip(GetSurface());
            resMgr.Drop(loadingbg);
        }
        else
            logerror("Init: \"Loading\" background not found!");
    }

    // load the initialization script
    logdetail("Initializing scripts...");
    memblock *mb = resMgr.LoadTextFile("scripts/init.fal");
    if(!mb)
    {
        logerror("Failed to load init script: 'scripts/init.fal'");
        logerror("It is required for starting up, so the engine will now terminate.");
        return false;
    }
    falcon->Init((char*)mb->ptr);
    resMgr.Drop(mb);

    log("Game Engine ready.");
    return true;
}

void GameEngine::OnKeyDown(SDLKey key, SDLMod mod)
{
    Engine::OnKeyDown(key, mod);

    // pass keypress to Falcon
    // TODO: cache this on init and call then without invoking findGlobalItem() all the time
    Falcon::Item *item = falcon->GetVM()->findGlobalItem("InputEventHandler");
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
    Falcon::Item *item = falcon->GetVM()->findGlobalItem("InputEventHandler");
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
    Falcon::Item *item = falcon->GetVM()->findGlobalItem("InputEventHandler");
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
#ifdef _DEBUG
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
        Point p = _layermgr->GetNonCollidingPoint(&mouseRect, checkDirection);
        collRect.x = p.x;
        collRect.y = p.y;
        collRectGood = !_layermgr->CollisionWith(&collRect);

    }
#endif
    if(type == SDL_MOUSEBUTTONDOWN)
    {
        if(button == 1)
        {
            logcustom(0, YELLOW, "Mouse position: (%u, %u), tile position: (%u, %u)", x, y, x / 16, y / 16);
            BaseRect r;
            r.x = x;
            r.y = y;
            r.h = r.w = 1;
            ObjectWithSideSet objs;
            objmgr->GetAllObjectsIn(r, objs);
            for(ObjectWithSideSet::iterator it = objs.begin(); it != objs.end(); it++)
            {
                ActiveRect *obj = (ActiveRect*)it->first;
                logcustom(0, YELLOW, "-> Object(%u): type %u, xywh=(%d,%d,%u,%u), layer=%d [%c%c%c]",
                    obj->GetId(), obj->GetType(), int32(obj->x), int32(obj->y), obj->w, obj->h,
                    obj->GetType() >= OBJTYPE_OBJECT ? ((Object*)obj)->GetLayer() : -1,
                    obj->IsCollisionEnabled() ? 'C' : '-', 
                    obj->GetType() >= OBJTYPE_OBJECT && ((Object*)obj)->IsAffectedByPhysics() ? 'P' : '-',
                    obj->GetType() >= OBJTYPE_OBJECT && ((Object*)obj)->IsBlocking() ? 'B' : '-');
            }
        }
#ifdef _DEBUG
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
#endif
    }
}

void GameEngine::OnObjectCreated(BaseObject *obj)
{
    // TODO: cache this on init and call then without invoking findGlobalItem() all the time
    Falcon::Item *item = falcon->GetVM()->findGlobalItem("ObjectCreated");
    if(item && item->isCallable())
    {
        try
        {
            Falcon::Item iobj(obj->_falObj->self());
            falcon->GetVM()->pushParam(iobj);
            falcon->GetVM()->callItem(*item, 1);
        }
        catch(Falcon::Error *err)
        {
            Falcon::AutoCString edesc( err->toString() );
            logerror("GameEngine::OnObjectCreated: %s", edesc.c_str());
            err->decref();
        }

    }
}

void GameEngine::_Render(void)
{
    // blank screen
    if(_drawBackground)
        SDL_FillRect(GetSurface(), NULL, _bgcolor);

    // render the layers
    _layermgr->Render();

    // TEST/DEBUG
#ifdef _DEBUG

    // draw the small rect that follows the mouse cursor
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
#endif

    _PostRender();
}

void GameEngine::_PostRender(void)
{
    // TODO: cache this on init and call then without invoking findGlobalItem() all the time
    Falcon::Item *item = falcon->GetVM()->findGlobalItem("PostRender");
    if(item && item->isCallable())
    {
        try
        {
            falcon->GetVM()->callItem(*item, 0);
        }
        catch(Falcon::Error *err)
        {
            Falcon::AutoCString edesc( err->toString() );
            logerror("GameEngine::Render: %s", edesc.c_str());
            err->decref();
        }

    }

    Engine::_PostRender();
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

void GameEngine::_Reset(void)
{
    Engine::_Reset();
    falcon->DeleteVM();
    Falcon::Engine::PerformGC();
    
    this->Setup();
}
