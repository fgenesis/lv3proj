#include "common.h"
#include "Animparser.h"
#include "ResourceMgr.h"
#include "LayerMgr.h"
#include "SoundCore.h"
#include "AsciiLevelParser.h"
#include "GameEngine.h"


GameEngine::GameEngine()
: Engine()
{
}

GameEngine::~GameEngine()
{
}

bool GameEngine::Setup(void)
{
    resMgr.LoadPropsInDir("music");


    AsciiLevel *level = LoadAsciiLevel("levels/testlevel.txt");
    _layermgr->LoadAsciiLevel(level);
    delete level;

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
