#include <stdarg.h>
#include <falcon/engine.h>
#include "common.h"
#include "AppFalconGame.h"

#undef LoadImage // this is due to some crappy win32 define included by windows.h included by falcon...

#include "GameEngine.h"
#include "TileLayer.h"
#include "ResourceMgr.h"
#include "LayerMgr.h"
#include "Objects.h"
#include "ObjectMgr.h"
#include "SoundCore.h"

#include "FalconGameModule.h"


// file-global defs
extern GameEngine g_engine;


FalconProxyObject::~FalconProxyObject()
{
    // remove cross-references
    self()->_falObj = NULL;
    self()->_obj = NULL;

    // allow the garbage collector to cleanup the remains (the fal_ObjectCarrier)
    delete gclock;
}

// Do NOT use other argument types than Falcon::Item
void FalconProxyObject::CallMethod(char *m, uint32 args /* = 0 */, ...)
{
    Falcon::Item method;
    if(self()->getMethod(m, method))
    {
        va_list ap;
        va_start(ap, args);
        vm->pushParam(va_arg(ap, Falcon::Item));
        va_end(ap);

        vm->callItem(method, args);
    }
}

void fal_ObjectCarrier::init(Falcon::VMachine *vm)
{
    fal_ObjectCarrier *self = Falcon::dyncast<fal_ObjectCarrier*>( vm->self().asObject() );
    FalconProxyObject *fobj = self->GetFalObj();
    BaseObject *obj = self->GetObj();
    fobj->vm = vm;
    fobj->gclock = new Falcon::GarbageLock(Falcon::Item(self));
    obj->Init(); // correctly set type of object
    obj->_falObj = fobj;
    g_engine.objmgr->Add(obj);
}

Falcon::CoreObject* fal_ObjectCarrier::factory( const Falcon::CoreClass *cls, void *user_data, bool )
{
    Falcon::String classname = cls->symbol()->name();

    // do not allow direct instantiation of the base classes, except Rect
    // with rect, it is good to replace member functions such as: rect.OnEnter = some_func
    if(classname == "Player" || classname == "Unit"
    || classname == "Item" || classname == "Object")
    {
        throw new Falcon::AccessError( Falcon::ErrorParam( Falcon::e_noninst_cls ) );
    }

    Falcon::ClassDef *clsdef = cls->symbol()->getClassDef();
    BaseObject *obj = NULL;

    if(clsdef->inheritsFrom("Player"))
    {
        obj = new Player;
    }
    else if(clsdef->inheritsFrom("Unit"))
    {
        obj = new Unit;
    }
    else if(clsdef->inheritsFrom("Item"))
    {
        obj = new Item;
    }
    else if(clsdef->inheritsFrom("Object"))
    {
        obj = new Object;
    }
    else if(clsdef->inheritsFrom("Rect") || classname == "Rect")
    {
        obj = new Rect;
    }

    if(!obj)
    {
        Falcon::AutoCString cclsname(classname);
        logerror("ObjectCarrier factory: Unable to instanciate an object of class '%s'", cclsname.c_str());
        throw new Falcon::AccessError( Falcon::ErrorParam( Falcon::e_noninst_cls ) );
    }

    FalconProxyObject *fobj = new FalconProxyObject(obj);
    return new fal_ObjectCarrier(cls, fobj);
}



bool fal_ObjectCarrier::setProperty( const Falcon::String &prop, const Falcon::Item &value )
{
    return false;
}

bool fal_ObjectCarrier::getProperty( const Falcon::String &prop, Falcon::Item &ret ) const
{
    if(!_obj)
    {
        throw new Falcon::AccessError( Falcon::ErrorParam( Falcon::e_invop_unb ).
            extra( "Object was already deleted!" ) );   
    }

    if(prop == "id")
    {
        ret = Falcon::int32(_obj->GetId());
        return true;
    }
    else if(prop == "type")
    {
        ret = Falcon::int32(_obj->GetType());
        return true;
    }
    else
    {
        return defaultProperty( prop, ret); // property not found
    }
}

// unbind the BaseObject from Falcon
// the FalconProxyObject destructor will do all the work required
// note that this does NOT delete the object itself!
void BaseObject::unbind(void)
{
    delete _falObj;
    _falObj = NULL;
}

// -- object proxy calls start --

// these are called from C++ like regular overloaded member functions, and proxy the call to be handled inside falcon
// on the correct member overloads.

void Rect::OnEnter(uint8 side, Object *who)
{
    DEBUG_ASSERT_RETURN_VOID(_falObj);
    _falObj->CallMethod("OnEnter", 2, Falcon::Item(Falcon::int32(side)), Falcon::Item(who->_falObj->self()));
}

void Rect::OnLeave(uint8 side, Object *who)
{
    DEBUG_ASSERT_RETURN_VOID(_falObj);
    _falObj->CallMethod("OnLeave", 2, Falcon::Item(Falcon::int32(side)), Falcon::Item(who->_falObj->self()));
}

void Rect::OnTouch(uint8 side, Object *who)
{
    DEBUG_ASSERT_RETURN_VOID(_falObj);
    _falObj->CallMethod("OnTouch", 2, Falcon::Item(Falcon::int32(side)), Falcon::Item(who->_falObj->self()));
}

void Object::OnUpdate(uint32 ms)
{
    DEBUG_ASSERT_RETURN_VOID(_falObj);
    _falObj->CallMethod("OnUpdate", 1, Falcon::Item(Falcon::int64(ms)));
}

void Item::OnUse(Object *who)
{
    DEBUG_ASSERT_RETURN_VOID(_falObj);
    _falObj->CallMethod("OnUse", 1, Falcon::Item(who->_falObj->self()));
}

// -- end object proxy calls --


// correctly remove an object from the Mgr, clean up its Falcon bindings, and delete it
FALCON_FUNC fal_BaseObject_Remove(Falcon::VMachine *vm)
{
    fal_ObjectCarrier *self = Falcon::dyncast<fal_ObjectCarrier*>( vm->self().asObject() );
    BaseObject *obj = self->GetObj();
    g_engine.objmgr->Remove(obj->GetId());
    obj->unbind();
    delete obj;
}

class fal_TileLayer : public Falcon::CoreObject
{
public:
    fal_TileLayer( const Falcon::CoreClass* generator, TileLayer *obj )
    : Falcon::CoreObject( generator ), _layer(obj)
    {
    }

    virtual bool setProperty( const Falcon::String &prop, const Falcon::Item &value )
    {
        return false;
    }

    virtual bool getProperty( const Falcon::String &prop, Falcon::Item &ret ) const
    {
        return defaultProperty( prop, ret); // property not found
    }

    Falcon::CoreObject *fal_TileLayer::clone() const
    {
        return NULL; // not cloneable
    }
    inline TileLayer *GetLayer(void) { return _layer; }
    
private:
    TileLayer *_layer;
};

class fal_Tile : public Falcon::CoreObject
{
public:
    fal_Tile( const Falcon::CoreClass* generator, BasicTile *obj )
        : Falcon::CoreObject( generator ), _tile(obj)
    {
    }

    static Falcon::CoreObject* factory( const Falcon::CoreClass *cls, void *user_data, bool )
    {
        return new fal_Tile(cls, NULL);
    }

    static void init(Falcon::VMachine *vm)
    {
        FALCON_REQUIRE_PARAMS_EXTRA(1, "S filename");
        Falcon::AutoCString file(vm->param(0)->asString());
        fal_Tile *self = Falcon::dyncast<fal_Tile*>( vm->self().asObject() );
        BasicTile *tile = NULL;

        std::string ext(FileGetExtension(file.c_str()));

        if(ext == ".anim")
        {
            if(Anim *ani = resMgr.LoadAnim((char*)file.c_str()))
            {
                tile = new AnimatedTile(ani);
                tile->filename = file;   
                ((AnimatedTile*)tile)->Init(Engine::GetCurFrameTime());
            }
        }
        else
        {
            if(SDL_Surface *img = resMgr.LoadImage((char*)file.c_str()))
            {
                tile = new BasicTile;
                tile->surface = img;
                tile->filename = file;   
            }
        }

        if(tile)
            self->_tile = tile;
        else
            vm->self().setNil();
    }

    Falcon::CoreObject *clone() const
    {
        return NULL; // not cloneable
    }

    virtual bool setProperty( const Falcon::String &prop, const Falcon::Item &value )
    {
        bool bad = false;

        if(prop == "name")
        {
            if(_tile->type == TILETYPE_ANIMATED)
            {
                Falcon::AutoCString cname(value);
                ((AnimatedTile*)_tile)->SetName((char*)cname.c_str());
                return true;
            }
            else
                bad = true;
        }
        else if(prop == "frame")
        {
            if(_tile->type == TILETYPE_ANIMATED)
            {
                ((AnimatedTile*)_tile)->SetFrame(value.forceInteger());
                return true;
            }
            else
                bad = true;
        }
        else if(prop == "type" || prop == "filename")
        {
            throw new Falcon::AccessError( Falcon::ErrorParam( Falcon::e_prop_ro ).
                extra( prop ) );           
        }

        if(bad)
        {
            throw new Falcon::AccessError( Falcon::ErrorParam( Falcon::e_miss_iface ).
            extra( prop ) );
        }

        return false;
    }

    virtual bool getProperty( const Falcon::String &prop, Falcon::Item &ret ) const
    {
        if(prop == "type")
        {
            ret = (Falcon::uint32)(_tile->type);
            return true;
        }
        else if(prop == "name")
        {
            if(_tile->type == TILETYPE_ANIMATED)
                ret = Falcon::String(((AnimatedTile*)_tile)->GetName());
            else
                ret.setNil();

            return true; // found the property
        }
        else if(prop == "frame")
        {
            if(_tile->type == TILETYPE_ANIMATED)
                ret = (Falcon::int32)(((AnimatedTile*)_tile)->GetFrame());
            else
                ret.setNil();

            return true; // found the property
        }
        else if(prop == "filename")
        {
            ret = Falcon::String(_tile->filename.c_str());
            return true;
        }
        else
        {
            return defaultProperty( prop, ret); // property not found
        }

        return false;
    }

    inline BasicTile *GetTile(void) { return _tile; }

private:
    BasicTile *_tile;
};


FALCON_FUNC fal_Screen_GetLayer(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS(1);
    uint32 layerId = vm->param(0)->forceInteger();
    TileLayer *layer = g_engine._GetLayerMgr()->GetLayer(layerId);
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
    arr->append((Falcon::int32)g_engine.GetResX());
    arr->append((Falcon::int32)g_engine.GetResY());
    vm->retval(arr);
}

FALCON_FUNC fal_TileLayer_SetVisible(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS(1);
    fal_TileLayer *self = Falcon::dyncast<fal_TileLayer*>( vm->self().asObject() );
    self->GetLayer()->visible = (bool)vm->param(0)->forceInteger();
}

FALCON_FUNC fal_TileLayer_IsVisible(Falcon::VMachine *vm)
{
    fal_TileLayer *self = Falcon::dyncast<fal_TileLayer*>( vm->self().asObject() );
    vm->retval(self->GetLayer()->visible);
}

FALCON_FUNC fal_TileLayer_SetTile(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS_EXTRA(3, "u32 x, u32 y, S filename");
    fal_TileLayer *self = Falcon::dyncast<fal_TileLayer*>( vm->self().asObject() );
    uint32 x = vm->param(0)->forceInteger();
    uint32 y = vm->param(1)->forceInteger();

    if(x >= self->GetLayer()->GetArraySize() || x >= self->GetLayer()->GetArraySize())
    {
        vm->retval(false);
        return;
    }

    Falcon::Item *tileArg = vm->param(2);

    BasicTile *tile = NULL;

    // support direct string-in and auto-convert to a tile
    if(tileArg->isString())
    {
        Falcon::AutoCString file(vm->param(2)->asString());
        std::string ext(FileGetExtension(file.c_str()));

        if(ext == ".anim")
        {
            if(Anim *ani = resMgr.LoadAnim((char*)file.c_str()))
            {
                tile = new AnimatedTile(ani);
                tile->filename = file;   
                ((AnimatedTile*)tile)->Init(Engine::GetCurFrameTime());
            }
        }
        else
        {
            if(SDL_Surface *img = resMgr.LoadImage((char*)file.c_str()))
            {
                tile = new BasicTile;
                tile->surface = img;
                tile->filename = file;   
            }
        }     
    }
    else if(tileArg->isOfClass("Tile"))
    {
        tile = (Falcon::dyncast<fal_Tile*>(tileArg->asObject()))->GetTile();
    }
    else if(tileArg->isNil())
    {
        self->GetLayer()->SetTile(x,y,NULL);
        vm->retval(true);
    }

    if(tile)
    {
        self->GetLayer()->SetTile(x,y,tile);
        vm->retval(true);
    }
    else
        vm->retval(false);
}

FALCON_FUNC fal_TileLayer_GetTile(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS_EXTRA(2, "u32 x, u32 y");
    fal_TileLayer *self = Falcon::dyncast<fal_TileLayer*>( vm->self().asObject() );
    uint32 x = vm->param(0)->forceInteger();
    uint32 y = vm->param(1)->forceInteger();
    if(BasicTile *tile = self->GetLayer()->GetTile(x,y))
    {
        Falcon::CoreClass *cls = vm->findWKI("Tile")->asClass();
        vm->retval(new fal_Tile(cls, tile));
    }
    else
        vm->retnil();
}

FALCON_FUNC fal_TileLayer_GetArraySize(Falcon::VMachine *vm)
{
    fal_TileLayer *self = Falcon::dyncast<fal_TileLayer*>( vm->self().asObject() );
    vm->retval((Falcon::int32)self->GetLayer()->GetArraySize());
}

/*
// this function is deprecated. left in the source for reference.
// called in falcon via: Game.LoadTile("sprites/en.anim") for example

FALCON_FUNC fal_Game_LoadTile(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS_EXTRA(1, "S filename");
    Falcon::AutoCString file(vm->param(0)->asString());
    BasicTile *tile = NULL;

    std::string ext(FileGetExtension(file.c_str()));

    if(ext == ".anim")
    {
        if(Anim *ani = resMgr.LoadAnim((char*)file.c_str()))
        {
            tile = new AnimatedTile(ani);
            tile->filename = file;   
            ((AnimatedTile*)tile)->Init(Engine::GetCurFrameTime());
        }
    }
    else
    {
        if(SDL_Surface *img = resMgr.LoadImage((char*)file.c_str()))
        {
            tile = new BasicTile;
            tile->surface = img;
            tile->filename = file;
        }
    }
    
    if(tile)
    {
        Falcon::CoreClass *cls = vm->findWKI("Tile")->asClass();
        vm->retval(new fal_Tile(cls, tile));
    }
    else
        vm->retnil();
}
*/

FALCON_FUNC fal_Game_GetTime(Falcon::VMachine *vm)
{
    vm->retval(Falcon::int64(g_engine.GetCurFrameTime()));
}

// this does absolutely nothing.
FALCON_FUNC fal_NullFunc(Falcon::VMachine *vm)
{
}

/*
// this function is deprecated. left in the source for reference.
// called in falcon via: Game.CreateObject("Erik") for example

FALCON_FUNC fal_Game_CreateObject(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS(1);
    if(!vm->param(0)->isString())
    {
        throw new Falcon::ParamError( Falcon::ErrorParam( Falcon::e_inv_params, __LINE__ )
            .extra( "S classname" ) );
    }

    Falcon::Item *icls = vm->findGlobalItem(*vm->param(0)->asString());
    if(!icls)
    {
        vm->retnil();
        return;

    }
    Falcon::CoreClass *cls = icls->asClass();
    Falcon::ClassDef *clsdef = cls->symbol()->getClassDef();
    BaseObject *obj = NULL;

    if(clsdef->inheritsFrom("Player"))
    {
        obj = new Player;
    }
    else if(clsdef->inheritsFrom("Unit"))
    {
        obj = new Unit;
    }
    else if(clsdef->inheritsFrom("Item"))
    {
        obj = new Item;
    }
    else if(clsdef->inheritsFrom("Object"))
    {
        obj = new Object;
    }
    else if(clsdef->inheritsFrom("Rect") || vm->param(0)->asString()->compare("Rect") == 0)
    {
        obj = new Rect;
    }

    if(!obj)
    {
        vm->retnil();
        return;
    }

    obj->Init(); // correctly set type of object

    FalconProxyObject *fobj = new FalconProxyObject(obj);
    obj->_falObj = fobj;

    fal_ObjectCarrier *carrier = new fal_ObjectCarrier(cls, fobj);
    fobj->vm = vm;
    fobj->gclock = new Falcon::GarbageLock(Falcon::Item(carrier));

    g_engine.objmgr->Add(obj);

    vm->retval(carrier);
}
*/

FALCON_FUNC fal_Game_GetObject(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS_EXTRA(1, "N id");
    BaseObject *obj = g_engine.objmgr->Get(vm->param(0)->forceInteger());
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
    vm->retval(Falcon::int32(g_engine.GetPlayerCount()));
}

FALCON_FUNC fal_Game_SetPlayerCount(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS(1);
    uint32 p = vm->param(0)->forceInteger();
    g_engine.SetPlayerCount(p);
}

FALCON_FUNC fal_Game_LoadLevel(Falcon::VMachine *vm)
{
    // TODO: implement this!
}

FALCON_FUNC fal_Game_Exit(Falcon::VMachine *vm)
{
    g_engine.Quit();
}



void forbidden_init(Falcon::VMachine *vm)
{
    throw new Falcon::AccessError( Falcon::ErrorParam( Falcon::e_noninst_cls ) );
}


Falcon::Module *FalconGameModule_create(void)
{
    Falcon::Module *m = new Falcon::Module;
    m->name("GameModule");

    Falcon::Symbol *symScreen = m->addSingleton("Screen");
    Falcon::Symbol *clsScreen = symScreen->getInstance();
    m->addClassMethod(clsScreen, "GetLayer", &fal_Screen_GetLayer);
    m->addClassMethod(clsScreen, "GetSize", &fal_Screen_GetSize);

    Falcon::Symbol *symGame = m->addSingleton("Game");
    Falcon::Symbol *clsGame = symGame->getInstance();
    //m->addClassMethod(clsGame, "LoadTile", fal_Game_LoadTile); // DEPRECATED - kept for reference
    m->addClassMethod(clsGame, "GetTime", fal_Game_GetTime);
    //m->addClassMethod(clsGame, "CreateObject", fal_Game_CreateObject); // DEPRECATED - kept for reference
    m->addClassMethod(clsGame, "SetSoundVolume", fal_Game_SetSoundVolume);
    m->addClassMethod(clsGame, "GetSoundVolume", fal_Game_GetSoundVolume);
    m->addClassMethod(clsGame, "SetMusicVolume", fal_Game_SetMusicVolume);
    m->addClassMethod(clsGame, "GetMusicVolume", fal_Game_GetMusicVolume);
    m->addClassMethod(clsGame, "GetPlayerCount", fal_Game_GetPlayerCount);
    m->addClassMethod(clsGame, "SetPlayerCount", fal_Game_SetPlayerCount);
    m->addClassMethod(clsGame, "LoadLevel", fal_Game_LoadLevel);
    m->addClassMethod(clsGame, "Exit", fal_Game_Exit);
    m->addConstant("MAX_VOLUME", Falcon::int64(MIX_MAX_VOLUME));

    Falcon::Symbol *clsTileLayer = m->addClass("TileLayer", &forbidden_init);
    clsTileLayer->setWKS(true);
    m->addClassMethod(clsTileLayer, "IsVisible", &fal_TileLayer_IsVisible);
    m->addClassMethod(clsTileLayer, "SetVisible", &fal_TileLayer_SetVisible);
    m->addClassMethod(clsTileLayer, "SetTile", &fal_TileLayer_SetTile);
    m->addClassMethod(clsTileLayer, "GetTile", &fal_TileLayer_GetTile);
    m->addClassMethod(clsTileLayer, "GetArraySize", &fal_TileLayer_GetArraySize);
    
    Falcon::Symbol *clsTile = m->addClass("Tile", &fal_Tile::init);
    clsTile->setWKS(true);
    clsTile->getClassDef()->factory(&fal_Tile::factory);
    m->addClassProperty(clsTile, "type");
    m->addClassProperty(clsTile, "frame");
    m->addClassProperty(clsTile, "name");
    m->addClassProperty(clsTile, "filename");
    m->addConstant("TILETYPE_STATIC", (Falcon::int64)TILETYPE_STATIC, true);
    m->addConstant("TILETYPE_ANIMATED", (Falcon::int64)TILETYPE_ANIMATED, true);

    Falcon::Symbol *clsBaseObject = m->addClass("BaseObject", &forbidden_init);
    Falcon::InheritDef *inhBaseObject = new Falcon::InheritDef(clsBaseObject); // there are other classes that inherit from BaseObject
    // this is NOT a WKS
    clsBaseObject->getClassDef()->factory(&fal_ObjectCarrier::factory);
    m->addClassProperty(clsBaseObject, "id");
    m->addClassProperty(clsBaseObject, "type");
    m->addClassMethod(clsBaseObject, "remove", fal_BaseObject_Remove);
    m->addConstant("OBJTYPE_RECT", (Falcon::int64)OBJTYPE_RECT, true);
    m->addConstant("OBJTYPE_OBJECT", (Falcon::int64)OBJTYPE_OBJECT, true);
    m->addConstant("OBJTYPE_ITEM", (Falcon::int64)OBJTYPE_ITEM, true);
    m->addConstant("OBJTYPE_UNIT", (Falcon::int64)OBJTYPE_UNIT, true);
    m->addConstant("OBJTYPE_PLAYER", (Falcon::int64)OBJTYPE_PLAYER, true);

    Falcon::Symbol *clsRect = m->addClass("Rect", &fal_ObjectCarrier::init);
    clsRect->getClassDef()->addInheritance(inhBaseObject);
    Falcon::InheritDef *inhRect = new Falcon::InheritDef(clsRect); // there are other classes that inherit from Rect
    clsRect->setWKS(true);
    
    m->addClassMethod(clsRect, "OnEnter", fal_NullFunc);
    m->addClassMethod(clsRect, "OnLeave", fal_NullFunc);
    m->addClassMethod(clsRect, "OnTouch", fal_NullFunc);

    Falcon::Symbol *clsObject = m->addClass("Object", &fal_ObjectCarrier::init);
    clsObject->getClassDef()->addInheritance(inhRect);
    Falcon::InheritDef *inhObject = new Falcon::InheritDef(clsObject); // there are other classes that inherit from Object
    clsObject->setWKS(true);
    m->addClassMethod(clsObject, "OnUpdate", fal_NullFunc);

    Falcon::Symbol *clsItem = m->addClass("Item", &fal_ObjectCarrier::init);
    clsItem->getClassDef()->addInheritance(inhRect);
    clsItem->getClassDef()->addInheritance(inhObject);
    clsItem->setWKS(true);
    m->addClassMethod(clsItem, "OnUse", fal_NullFunc);

    Falcon::Symbol *clsUnit = m->addClass("Unit", &fal_ObjectCarrier::init);
    Falcon::InheritDef *inhUnit = new Falcon::InheritDef(clsUnit);
    clsUnit->getClassDef()->addInheritance(inhObject);
    clsUnit->setWKS(true);

    Falcon::Symbol *clsPlayer = m->addClass("Player", &fal_ObjectCarrier::init);
    clsPlayer->getClassDef()->addInheritance(inhUnit);
    clsPlayer->setWKS(true);


    m->addConstant("EVENT_TYPE_KEYBOARD", Falcon::int64(EVENT_TYPE_KEYBOARD));
    m->addConstant("EVENT_TYPE_JOYSTICK_BUTTON", Falcon::int64(EVENT_TYPE_JOYSTICK_BUTTON));
    m->addConstant("EVENT_TYPE_JOYSTICK_AXIS", Falcon::int64(EVENT_TYPE_JOYSTICK_AXIS));
    m->addConstant("EVENT_TYPE_JOYSTICK_HAT", Falcon::int64(EVENT_TYPE_JOYSTICK_HAT));


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


