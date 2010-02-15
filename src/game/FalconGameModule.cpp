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

FALCON_FUNC fal_Game_LoadTile(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS_EXTRA(1, "S filename");
    Falcon::AutoCString file(vm->param(0)->asString());
    BasicTile *tile = NULL;

    // support direct string-in and auto-convert to a tile

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
    m->addClassMethod(clsGame, "LoadTile", fal_Game_LoadTile);
    m->addClassMethod(clsGame, "GetTime", fal_Game_GetTime);
    //m->addClassMethod(clsGame, "CreateObject", fal_Game_CreateObject); // DEPRECATED - kept for reference

    Falcon::Symbol *clsTileLayer = m->addClass("TileLayer", &forbidden_init);
    clsTileLayer->setWKS(true);
    m->addClassMethod(clsTileLayer, "IsVisible", &fal_TileLayer_IsVisible);
    m->addClassMethod(clsTileLayer, "SetVisible", &fal_TileLayer_SetVisible);
    m->addClassMethod(clsTileLayer, "SetTile", &fal_TileLayer_SetTile);
    m->addClassMethod(clsTileLayer, "GetTile", &fal_TileLayer_GetTile);
    m->addClassMethod(clsTileLayer, "GetArraySize", &fal_TileLayer_GetArraySize);
    
    Falcon::Symbol *clsTile = m->addClass("Tile", &forbidden_init);
    clsTile->setWKS(true);
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


    return m;
};


