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

GameEngine::GameEngine()
: Engine()
{
    objmgr = new ObjectMgr;
    falcon = new AppFalconGame;
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

    resMgr.LoadPropsInDir("music");


    AsciiLevel *level = LoadAsciiLevel("levels/testlevel.txt");
    _layermgr->LoadAsciiLevel(level);
    delete level;

    char *testscript = resMgr.LoadTextFile("scripts/test.fal", "rb");
    falcon->EmbedStringAsModule(testscript, "testscript");
    
    sndCore.PlayMusic("lv1_snes_ship.ogg");

    return true;
}

void GameEngine::OnKeyDown(SDLKey key, SDLMod mod)
{
    Engine::OnKeyDown(key, mod);
}

void GameEngine::OnKeyUp(SDLKey key, SDLMod mod)
{
    Engine::OnKeyUp(key, mod);
}

void GameEngine::_Render(void)
{
    SDL_FillRect(GetSurface(), NULL, 0); // TODO: remove this as soon as backgrounds are implemented!! (eats CPU)
    Engine::_Render();
}

void GameEngine::_Process(uint32 ms)
{
    // TODO: cache this on init and call then without invoking findGlobalItem() all the time
    Falcon::Item *item = falcon->GetVM()->findGlobalItem("GameUpdate");
    if(item && item->isCallable())
    {
        falcon->GetVM()->pushParam(Falcon::int64(ms));
        falcon->GetVM()->callItem(*item, 1);
    }
    
    Engine::_Process(ms);
}
