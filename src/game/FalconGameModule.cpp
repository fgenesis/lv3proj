#include <stdarg.h>
#include <falcon/engine.h>
#include "common.h"
#include "AppFalconGame.h"

#include "main.h"
#include "GameEngine.h"
#include "TileLayer.h"
#include "ResourceMgr.h"
#include "LayerMgr.h"
#include "Objects.h"
#include "ObjectMgr.h"
#include "SoundCore.h"
#include "PhysicsSystem.h"
#include "AsciiLevelParser.h"

#include "FalconBaseModule.h"
#include "FalconObjectModule.h"
#include "FalconGameModule.h"


class fal_TileLayer;

GameEngine *g_engine_ptr = NULL;

void FalconGameModule_SetEnginePtr(Engine *e)
{
    g_engine_ptr = (GameEngine*)e;
}



FALCON_FUNC fal_Screen_GetLayer(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS(1);
    uint32 layerId = vm->param(0)->forceInteger();
    TileLayer *layer = g_engine_ptr->_GetLayerMgr()->GetLayer(layerId);
    if(layer)
    {
        Falcon::CoreClass *cls = vm->findWKI("TileLayer")->asClass();
        vm->retval(new fal_TileLayer(cls, layer));
    }
    else
        vm->retnil();
}

FALCON_FUNC fal_Screen_GetSize(Falcon::VMachine *vm)
{
    Falcon::CoreArray *arr = new Falcon::CoreArray(2);
    arr->append((Falcon::int32)g_engine_ptr->GetResX());
    arr->append((Falcon::int32)g_engine_ptr->GetResY());
    vm->retval(arr);
}

FALCON_FUNC fal_Screen_GetLayerSize(Falcon::VMachine *vm)
{
    vm->retval((int64)g_engine_ptr->_GetLayerMgr()->GetMaxDim());
}

FALCON_FUNC fal_Screen_GetTileInfo(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS_EXTRA(2, "N, N");
    uint32 x = vm->param(0)->forceIntegerEx();
    uint32 y = vm->param(1)->forceIntegerEx();
    LayerMgr *lm = g_engine_ptr->_GetLayerMgr();
    if(!lm->GetInfoLayer())
    {
        throw new GameError( Falcon::ErrorParam( Falcon::e_undef_state ).
            extra( "TileInfoLayer not created" ) );
    }
    uint32 m = lm->GetMaxDim();
    if( !(x < m && y < m) )
    {
        throw new Falcon::AccessError( Falcon::ErrorParam( Falcon::e_arracc ) );
    }
    vm->retval((int64)lm->GetTileInfo(x,y));
}

FALCON_FUNC fal_Screen_SetTileInfo(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS_EXTRA(3, "N, N, N");
    uint32 x = vm->param(0)->forceIntegerEx();
    uint32 y = vm->param(1)->forceIntegerEx();
    uint32 info = vm->param(2)->forceIntegerEx();
    LayerMgr *lm = g_engine_ptr->_GetLayerMgr();
    if(!lm->GetInfoLayer())
    {
        throw new GameError( Falcon::ErrorParam( Falcon::e_undef_state ).
            extra( "TileInfoLayer not created" ) );
    }
    uint32 m = lm->GetMaxDim();
    if( !(x < m && y < m) )
    {
        throw new Falcon::AccessError( Falcon::ErrorParam( Falcon::e_arracc ) );
    }
    lm->SetTileInfo(x,y,info);
}

FALCON_FUNC fal_Screen_CreateInfoLayer(Falcon::VMachine *vm)
{
    LayerMgr *lm = g_engine_ptr->_GetLayerMgr();
    lm->CreateInfoLayer();
}

FALCON_FUNC fal_Game_GetTime(Falcon::VMachine *vm)
{
    vm->retval(Falcon::int64(g_engine_ptr->GetCurFrameTime()));
}

FALCON_FUNC fal_Game_GetObject(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS_EXTRA(1, "N id");
    BaseObject *obj = g_engine_ptr->objmgr->Get(vm->param(0)->forceInteger());
    if(obj)
    {
        vm->retval(obj->_falObj->self());
    }
    else
    {
        vm->retnil();
    }
}

FALCON_FUNC fal_Game_GetSoundVolume(Falcon::VMachine *vm)
{
    vm->retval(Falcon::int32(255)); // TODO: implement this!
}

FALCON_FUNC fal_Game_SetSoundVolume(Falcon::VMachine *vm)
{
    // TODO: implement this!
}

FALCON_FUNC fal_Game_GetMusicVolume(Falcon::VMachine *vm)
{
    vm->retval(Falcon::int32(sndCore.GetMusicVolume()));
}

FALCON_FUNC fal_Game_SetMusicVolume(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS_EXTRA(1, "Int volume");
    sndCore.SetMusicVolume(vm->param(0)->forceInteger());
}

FALCON_FUNC fal_Game_GetPlayerCount(Falcon::VMachine *vm)
{
    vm->retval(Falcon::int32(g_engine_ptr->GetPlayerCount()));
}

FALCON_FUNC fal_Game_SetPlayerCount(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS(1);
    uint32 p = vm->param(0)->forceInteger();
    g_engine_ptr->SetPlayerCount(p);
}

FALCON_FUNC fal_Game_LoadLevel(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS_EXTRA(1,"S filename");
    Falcon::AutoCString cstr = vm->param(0)->asString();
    AsciiLevel *level = LoadAsciiLevel((char*)cstr.c_str());
    if(!level)
    {
        throw new GameError( Falcon::ErrorParam( Falcon::e_nofile ) );
    }
    g_engine_ptr->_GetLayerMgr()->LoadAsciiLevel(level);
}

FALCON_FUNC fal_Game_Exit(Falcon::VMachine *vm)
{
    g_engine_ptr->Quit();
}

FALCON_FUNC fal_Game_LoadPropsInDir(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS_EXTRA(1, "S path");
    Falcon::AutoCString cstr(vm->param(0)->asString());
    resMgr.LoadPropsInDir((char*)cstr.c_str());
}

FALCON_FUNC fal_Game_CreateCollisionMap(Falcon::VMachine *vm)
{
    bool update = false;
    if(vm->paramCount() >= 1)
        update = vm->param(0)->asBoolean();

    LayerMgr *lm = g_engine_ptr->_GetLayerMgr();
    lm->CreateCollisionMap();
    if(update)
        lm->UpdateCollisionMap();
}

FALCON_FUNC fal_Game_UpdateCollisionMap(Falcon::VMachine *vm)
{
    LayerMgr *lm = g_engine_ptr->_GetLayerMgr();
    if(!lm->HasCollisionMap())
    {
        throw new GameError( Falcon::ErrorParam( Falcon::e_undef_state ).
            extra( "CollisionMap not created" ) );
    }
    if(!vm->paramCount())
    {
       lm->UpdateCollisionMap();
    }
    else if(vm->paramCount() >= 2)
    {
        uint32 x = vm->param(0)->forceIntegerEx();
        uint32 y = vm->param(1)->forceIntegerEx();
        lm->UpdateCollisionMap(x,y);
    }
    else
    {
        throw new Falcon::ParamError( Falcon::ErrorParam( Falcon::e_missing_params ).
            extra( "No args or N,N" ) );
    }
}

FALCON_FUNC fal_Game_PlayMusic(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS_EXTRA(1,"S filename");
    Falcon::AutoCString cstr(vm->param(0)->asString());
    sndCore.PlayMusic((char*)cstr.c_str());
}

FALCON_FUNC fal_Game_StopMusic(Falcon::VMachine *vm)
{
    sndCore.StopMusic();
}


FALCON_FUNC fal_Physics_SetGravity(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS_EXTRA(1, "N");
    g_engine_ptr->physmgr->envPhys.gravity = float(vm->param(0)->forceNumeric());
}

FALCON_FUNC fal_Physics_GetGravity(Falcon::VMachine *vm)
{
    vm->retval(g_engine_ptr->physmgr->envPhys.gravity);
}


Falcon::Module *FalconGameModule_create(void)
{
    Falcon::Module *m = new Falcon::Module;
    m->name("GameModule");

    Falcon::Symbol *symScreen = m->addSingleton("Screen");
    Falcon::Symbol *clsScreen = symScreen->getInstance();
    m->addClassMethod(clsScreen, "GetLayer", &fal_Screen_GetLayer);
    m->addClassMethod(clsScreen, "GetSize", &fal_Screen_GetSize);
    m->addClassMethod(clsScreen, "GetLayerSize", &fal_Screen_GetLayerSize);
    m->addClassMethod(clsScreen, "SetTileInfo", &fal_Screen_SetTileInfo);
    m->addClassMethod(clsScreen, "GetTileInfo", &fal_Screen_GetTileInfo);
    m->addClassMethod(clsScreen, "CreateInfoLayer", &fal_Screen_CreateInfoLayer);

    Falcon::Symbol *symPhysics = m->addSingleton("Physics");
    Falcon::Symbol *clsPhysics = symPhysics->getInstance();
    m->addClassMethod(clsPhysics, "SetGravity", fal_Physics_SetGravity);
    m->addClassMethod(clsPhysics, "GetGravity", fal_Physics_GetGravity);

    Falcon::Symbol *symGame = m->addSingleton("Game");
    Falcon::Symbol *clsGame = symGame->getInstance();
    m->addClassMethod(clsGame, "GetTime", fal_Game_GetTime);
    m->addClassMethod(clsGame, "SetSoundVolume", fal_Game_SetSoundVolume);
    m->addClassMethod(clsGame, "GetSoundVolume", fal_Game_GetSoundVolume);
    m->addClassMethod(clsGame, "SetMusicVolume", fal_Game_SetMusicVolume);
    m->addClassMethod(clsGame, "GetMusicVolume", fal_Game_GetMusicVolume);
    m->addClassMethod(clsGame, "PlayMusic", fal_Game_PlayMusic);
    m->addClassMethod(clsGame, "StopMusic", fal_Game_PlayMusic);
    m->addClassMethod(clsGame, "GetPlayerCount", fal_Game_GetPlayerCount);
    m->addClassMethod(clsGame, "SetPlayerCount", fal_Game_SetPlayerCount);
    m->addClassMethod(clsGame, "LoadLevel", fal_Game_LoadLevel);
    m->addClassMethod(clsGame, "Exit", fal_Game_Exit);
    m->addClassMethod(clsGame, "LoadPropsInDir", fal_Game_LoadPropsInDir);
    m->addClassMethod(clsGame, "CreateCollisionMap", fal_Game_CreateCollisionMap);
    m->addClassMethod(clsGame, "UpdateCollisionMap", fal_Game_UpdateCollisionMap);
    m->addConstant("MAX_VOLUME", Falcon::int64(MIX_MAX_VOLUME));


    // the SDL key bindings
    Falcon::Symbol *c_sdlk = m->addClass( "SDLK", &forbidden_init);
    m->addClassProperty( c_sdlk, "BACKSPACE" ).setInteger( SDLK_BACKSPACE );
    m->addClassProperty( c_sdlk, "TAB" ).setInteger( SDLK_TAB );
    m->addClassProperty( c_sdlk, "CLEAR" ).setInteger( SDLK_CLEAR );
    m->addClassProperty( c_sdlk, "RETURN" ).setInteger( SDLK_RETURN );
    m->addClassProperty( c_sdlk, "PAUSE" ).setInteger( SDLK_PAUSE );
    m->addClassProperty( c_sdlk, "ESCAPE" ).setInteger( SDLK_ESCAPE );
    m->addClassProperty( c_sdlk, "SPACE" ).setInteger( SDLK_SPACE );
    m->addClassProperty( c_sdlk, "EXCLAIM" ).setInteger( SDLK_EXCLAIM );
    m->addClassProperty( c_sdlk, "QUOTEDBL" ).setInteger( SDLK_QUOTEDBL );
    m->addClassProperty( c_sdlk, "HASH" ).setInteger( SDLK_HASH );
    m->addClassProperty( c_sdlk, "DOLLAR" ).setInteger( SDLK_DOLLAR );
    m->addClassProperty( c_sdlk, "AMPERSAND" ).setInteger( SDLK_AMPERSAND );
    m->addClassProperty( c_sdlk, "QUOTE" ).setInteger( SDLK_QUOTE );
    m->addClassProperty( c_sdlk, "LEFTPAREN" ).setInteger( SDLK_LEFTPAREN );
    m->addClassProperty( c_sdlk, "RIGHTPAREN" ).setInteger( SDLK_RIGHTPAREN );
    m->addClassProperty( c_sdlk, "ASTERISK" ).setInteger( SDLK_ASTERISK );
    m->addClassProperty( c_sdlk, "PLUS" ).setInteger( SDLK_PLUS );
    m->addClassProperty( c_sdlk, "COMMA" ).setInteger( SDLK_COMMA );
    m->addClassProperty( c_sdlk, "MINUS" ).setInteger( SDLK_MINUS );
    m->addClassProperty( c_sdlk, "PERIOD" ).setInteger( SDLK_PERIOD );
    m->addClassProperty( c_sdlk, "SLASH" ).setInteger( SDLK_SLASH );
    m->addClassProperty( c_sdlk, "0" ).setInteger( SDLK_0 );
    m->addClassProperty( c_sdlk, "1" ).setInteger( SDLK_1 );
    m->addClassProperty( c_sdlk, "2" ).setInteger( SDLK_2 );
    m->addClassProperty( c_sdlk, "3" ).setInteger( SDLK_3 );
    m->addClassProperty( c_sdlk, "4" ).setInteger( SDLK_4 );
    m->addClassProperty( c_sdlk, "5" ).setInteger( SDLK_5 );
    m->addClassProperty( c_sdlk, "6" ).setInteger( SDLK_6 );
    m->addClassProperty( c_sdlk, "7" ).setInteger( SDLK_7 );
    m->addClassProperty( c_sdlk, "8" ).setInteger( SDLK_8 );
    m->addClassProperty( c_sdlk, "9" ).setInteger( SDLK_9 );
    m->addClassProperty( c_sdlk, "COLON" ).setInteger( SDLK_COLON );
    m->addClassProperty( c_sdlk, "SEMICOLON" ).setInteger( SDLK_SEMICOLON );
    m->addClassProperty( c_sdlk, "LESS" ).setInteger( SDLK_LESS );
    m->addClassProperty( c_sdlk, "EQUALS" ).setInteger( SDLK_EQUALS );
    m->addClassProperty( c_sdlk, "GREATER" ).setInteger( SDLK_GREATER );
    m->addClassProperty( c_sdlk, "QUESTION" ).setInteger( SDLK_QUESTION );
    m->addClassProperty( c_sdlk, "AT" ).setInteger( SDLK_AT );
    m->addClassProperty( c_sdlk, "LEFTBRACKET" ).setInteger( SDLK_LEFTBRACKET );
    m->addClassProperty( c_sdlk, "BACKSLASH" ).setInteger( SDLK_BACKSLASH );
    m->addClassProperty( c_sdlk, "RIGHTBRACKET" ).setInteger( SDLK_RIGHTBRACKET );
    m->addClassProperty( c_sdlk, "CARET" ).setInteger( SDLK_CARET );
    m->addClassProperty( c_sdlk, "UNDERSCORE" ).setInteger( SDLK_UNDERSCORE );
    m->addClassProperty( c_sdlk, "BACKQUOTE" ).setInteger( SDLK_BACKQUOTE );
    m->addClassProperty( c_sdlk, "a" ).setInteger( SDLK_a );
    m->addClassProperty( c_sdlk, "b" ).setInteger( SDLK_b );
    m->addClassProperty( c_sdlk, "c" ).setInteger( SDLK_c );
    m->addClassProperty( c_sdlk, "d" ).setInteger( SDLK_d );
    m->addClassProperty( c_sdlk, "e" ).setInteger( SDLK_e );
    m->addClassProperty( c_sdlk, "f" ).setInteger( SDLK_f );
    m->addClassProperty( c_sdlk, "g" ).setInteger( SDLK_g );
    m->addClassProperty( c_sdlk, "h" ).setInteger( SDLK_h );
    m->addClassProperty( c_sdlk, "i" ).setInteger( SDLK_i );
    m->addClassProperty( c_sdlk, "j" ).setInteger( SDLK_j );
    m->addClassProperty( c_sdlk, "k" ).setInteger( SDLK_k );
    m->addClassProperty( c_sdlk, "l" ).setInteger( SDLK_l );
    m->addClassProperty( c_sdlk, "m" ).setInteger( SDLK_m );
    m->addClassProperty( c_sdlk, "n" ).setInteger( SDLK_n );
    m->addClassProperty( c_sdlk, "o" ).setInteger( SDLK_o );
    m->addClassProperty( c_sdlk, "p" ).setInteger( SDLK_p );
    m->addClassProperty( c_sdlk, "q" ).setInteger( SDLK_q );
    m->addClassProperty( c_sdlk, "r" ).setInteger( SDLK_r );
    m->addClassProperty( c_sdlk, "s" ).setInteger( SDLK_s );
    m->addClassProperty( c_sdlk, "t" ).setInteger( SDLK_t );
    m->addClassProperty( c_sdlk, "u" ).setInteger( SDLK_u );
    m->addClassProperty( c_sdlk, "v" ).setInteger( SDLK_v );
    m->addClassProperty( c_sdlk, "w" ).setInteger( SDLK_w );
    m->addClassProperty( c_sdlk, "x" ).setInteger( SDLK_x );
    m->addClassProperty( c_sdlk, "y" ).setInteger( SDLK_y );
    m->addClassProperty( c_sdlk, "z" ).setInteger( SDLK_z );
    m->addClassProperty( c_sdlk, "DELETE" ).setInteger( SDLK_DELETE );
    m->addClassProperty( c_sdlk, "KP0" ).setInteger( SDLK_KP0 );
    m->addClassProperty( c_sdlk, "KP1" ).setInteger( SDLK_KP1 );
    m->addClassProperty( c_sdlk, "KP2" ).setInteger( SDLK_KP2 );
    m->addClassProperty( c_sdlk, "KP3" ).setInteger( SDLK_KP3 );
    m->addClassProperty( c_sdlk, "KP4" ).setInteger( SDLK_KP4 );
    m->addClassProperty( c_sdlk, "KP5" ).setInteger( SDLK_KP5 );
    m->addClassProperty( c_sdlk, "KP6" ).setInteger( SDLK_KP6 );
    m->addClassProperty( c_sdlk, "KP7" ).setInteger( SDLK_KP7 );
    m->addClassProperty( c_sdlk, "KP8" ).setInteger( SDLK_KP8 );
    m->addClassProperty( c_sdlk, "KP9" ).setInteger( SDLK_KP9 );
    m->addClassProperty( c_sdlk, "KP_PERIOD" ).setInteger( SDLK_KP_PERIOD );
    m->addClassProperty( c_sdlk, "KP_DIVIDE" ).setInteger( SDLK_KP_DIVIDE );
    m->addClassProperty( c_sdlk, "KP_MULTIPLY" ).setInteger( SDLK_KP_MULTIPLY );
    m->addClassProperty( c_sdlk, "KP_MINUS" ).setInteger( SDLK_KP_MINUS );
    m->addClassProperty( c_sdlk, "KP_PLUS" ).setInteger( SDLK_KP_PLUS );
    m->addClassProperty( c_sdlk, "KP_ENTER" ).setInteger( SDLK_KP_ENTER );
    m->addClassProperty( c_sdlk, "KP_EQUALS" ).setInteger( SDLK_KP_EQUALS );
    m->addClassProperty( c_sdlk, "UP" ).setInteger( SDLK_UP );
    m->addClassProperty( c_sdlk, "DOWN" ).setInteger( SDLK_DOWN );
    m->addClassProperty( c_sdlk, "RIGHT" ).setInteger( SDLK_RIGHT );
    m->addClassProperty( c_sdlk, "LEFT" ).setInteger( SDLK_LEFT );
    m->addClassProperty( c_sdlk, "INSERT" ).setInteger( SDLK_INSERT );
    m->addClassProperty( c_sdlk, "HOME" ).setInteger( SDLK_HOME );
    m->addClassProperty( c_sdlk, "END" ).setInteger( SDLK_END );
    m->addClassProperty( c_sdlk, "PAGEUP" ).setInteger( SDLK_PAGEUP );
    m->addClassProperty( c_sdlk, "PAGEDOWN" ).setInteger( SDLK_PAGEDOWN );
    m->addClassProperty( c_sdlk, "F1" ).setInteger( SDLK_F1 );
    m->addClassProperty( c_sdlk, "F2" ).setInteger( SDLK_F2 );
    m->addClassProperty( c_sdlk, "F3" ).setInteger( SDLK_F3 );
    m->addClassProperty( c_sdlk, "F4" ).setInteger( SDLK_F4 );
    m->addClassProperty( c_sdlk, "F5" ).setInteger( SDLK_F5 );
    m->addClassProperty( c_sdlk, "F6" ).setInteger( SDLK_F6 );
    m->addClassProperty( c_sdlk, "F7" ).setInteger( SDLK_F7 );
    m->addClassProperty( c_sdlk, "F8" ).setInteger( SDLK_F8 );
    m->addClassProperty( c_sdlk, "F9" ).setInteger( SDLK_F9 );
    m->addClassProperty( c_sdlk, "F10" ).setInteger( SDLK_F10 );
    m->addClassProperty( c_sdlk, "F11" ).setInteger( SDLK_F11 );
    m->addClassProperty( c_sdlk, "F12" ).setInteger( SDLK_F12 );
    m->addClassProperty( c_sdlk, "F13" ).setInteger( SDLK_F13 );
    m->addClassProperty( c_sdlk, "F14" ).setInteger( SDLK_F14 );
    m->addClassProperty( c_sdlk, "F15" ).setInteger( SDLK_F15 );
    m->addClassProperty( c_sdlk, "NUMLOCK" ).setInteger( SDLK_NUMLOCK );
    m->addClassProperty( c_sdlk, "CAPSLOCK" ).setInteger( SDLK_CAPSLOCK );
    m->addClassProperty( c_sdlk, "SCROLLOCK" ).setInteger( SDLK_SCROLLOCK );
    m->addClassProperty( c_sdlk, "RSHIFT" ).setInteger( SDLK_RSHIFT );
    m->addClassProperty( c_sdlk, "LSHIFT" ).setInteger( SDLK_LSHIFT );
    m->addClassProperty( c_sdlk, "RCTRL" ).setInteger( SDLK_RCTRL );
    m->addClassProperty( c_sdlk, "LCTRL" ).setInteger( SDLK_LCTRL );
    m->addClassProperty( c_sdlk, "RALT" ).setInteger( SDLK_RALT );
    m->addClassProperty( c_sdlk, "LALT" ).setInteger( SDLK_LALT );
    m->addClassProperty( c_sdlk, "RMETA" ).setInteger( SDLK_RMETA );
    m->addClassProperty( c_sdlk, "LMETA" ).setInteger( SDLK_LMETA );
    m->addClassProperty( c_sdlk, "LSUPER" ).setInteger( SDLK_LSUPER );
    m->addClassProperty( c_sdlk, "RSUPER" ).setInteger( SDLK_RSUPER );
    m->addClassProperty( c_sdlk, "MODE" ).setInteger( SDLK_MODE );
    m->addClassProperty( c_sdlk, "HELP" ).setInteger( SDLK_HELP );
    m->addClassProperty( c_sdlk, "PRINT" ).setInteger( SDLK_PRINT );
    m->addClassProperty( c_sdlk, "SYSREQ" ).setInteger( SDLK_SYSREQ );
    m->addClassProperty( c_sdlk, "BREAK" ).setInteger( SDLK_BREAK );
    m->addClassProperty( c_sdlk, "MENU" ).setInteger( SDLK_MENU );
    m->addClassProperty( c_sdlk, "POWER" ).setInteger( SDLK_POWER );
    m->addClassProperty( c_sdlk, "EURO" ).setInteger( SDLK_EURO );


    return m;
};


