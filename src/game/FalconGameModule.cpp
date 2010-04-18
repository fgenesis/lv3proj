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

#include "FalconGameModule.h"


FalconProxyObject::~FalconProxyObject()
{
    // remove cross-references
    self()->_falObj = NULL;
    self()->_obj = NULL;

    // allow the garbage collector to cleanup the remains (the fal_ObjectCarrier)
    delete gclock;
}

// Do NOT use other argument types than Falcon::Item
Falcon::Item *FalconProxyObject::CallMethod(char *m, uint32 args /* = 0 */, ...)
{
    Falcon::Item method;
    if(self()->getMethod(m, method))
    {
        va_list ap;
        va_start(ap, args);
        for(uint32 i = 0; i < args; ++i)
        {
            Falcon::Item& itm = va_arg(ap, Falcon::Item);
            vm->pushParam(itm);
        }
        va_end(ap);

        try
        {
            vm->callItem(method, args);
        }
        catch(Falcon::Error *err)
        {
            Falcon::AutoCString edesc( err->toString() );
            logerror("FalconProxyObject::CallMethod(%s): %s", m, edesc.c_str());
            err->decref();
            return NULL;
        }
        return &(vm->regA());
    }
    return NULL;
}

class fal_PhysProps : public Falcon::CoreObject
{
public:

    fal_PhysProps( const Falcon::CoreClass* generator, PhysProps& prop, bool asRef)
        : Falcon::CoreObject( generator ), _referenced(asRef)
    {
        if(asRef)
            _phys = &prop; // just pass ptr
        else
        {
            _phys = new PhysProps;
            *_phys = prop; // clone
        }
    }

    fal_PhysProps(const fal_PhysProps& other, bool asRef)
        : Falcon::CoreObject(other), _referenced(asRef)
    {
        if(asRef)
            _phys = other._phys; // just pass ptr
        else
        {
            _phys = new PhysProps;
            *_phys = *(other._phys); // clone
        }
    }

    Falcon::CoreObject *clone(void) const
    {
        fal_PhysProps *p = new fal_PhysProps(*this, false);
        return p;
    }

    bool finalize(void)
    {
        if(!_referenced)
            delete _phys;
        return false;
    }

    virtual bool setProperty( const Falcon::String &prop, const Falcon::Item &value )
    {
        if(prop == "weight")      { _phys->weight      = value.forceNumeric(); return true; }
        if(prop == "xspeed")      { _phys->xspeed      = value.forceNumeric(); return true; }
        if(prop == "yspeed")      { _phys->yspeed      = value.forceNumeric(); return true; }
        if(prop == "xmaxspeed")   { _phys->xmaxspeed   = value.forceNumeric(); return true; }
        if(prop == "ymaxspeed")   { _phys->ymaxspeed   = value.forceNumeric(); return true; }
        if(prop == "xaccel")      { _phys->xaccel      = value.forceNumeric(); return true; }
        if(prop == "yaccel")      { _phys->yaccel      = value.forceNumeric(); return true; }
        if(prop == "xfriction")   { _phys->xfriction   = value.forceNumeric(); return true; }
        if(prop == "yfriction")   { _phys->yfriction   = value.forceNumeric(); return true; }

        if(prop == "isRef")
            throw new Falcon::AccessError( Falcon::ErrorParam( Falcon::e_prop_ro ).
                extra( prop ) );

        return false;
    }

    virtual bool getProperty( const Falcon::String &prop, Falcon::Item &ret ) const
    {
        if(prop == "weight")      { ret.setNumeric(_phys->weight);      return true; }
        if(prop == "xspeed")      { ret.setNumeric(_phys->xspeed);      return true; }
        if(prop == "yspeed")      { ret.setNumeric(_phys->yspeed);      return true; }
        if(prop == "xmaxspeed")   { ret.setNumeric(_phys->xmaxspeed);   return true; }
        if(prop == "ymaxspeed")   { ret.setNumeric(_phys->ymaxspeed);   return true; }
        if(prop == "xaccel")      { ret.setNumeric(_phys->xaccel);      return true; }
        if(prop == "yaccel")      { ret.setNumeric(_phys->yaccel);      return true; }
        if(prop == "xfriction")   { ret.setNumeric(_phys->xfriction);   return true; }
        if(prop == "yfriction")   { ret.setNumeric(_phys->yfriction);   return true; }
        if(prop == "isRef")       { ret.setBoolean(_referenced);        return true; }

        return defaultProperty( prop, ret); // property not found
    }

    inline PhysProps *GetPhysProps(void) { return _phys; }


    bool _referenced;

private:
    PhysProps *_phys;


};

void fal_ObjectCarrier::init(Falcon::VMachine *vm)
{
    fal_ObjectCarrier *self = Falcon::dyncast<fal_ObjectCarrier*>( vm->self().asObject() );
    FalconProxyObject *fobj = self->GetFalObj();
    BaseObject *obj = self->GetObj();
    fobj->vm = vm;
    fobj->gclock = new Falcon::GarbageLock(Falcon::Item(self));
    obj->SetLayerMgr(g_engine_ptr->_GetLayerMgr());
    obj->Init(); // correctly set type of object
    obj->_falObj = fobj;
    g_engine_ptr->objmgr->Add(obj);
}

Falcon::CoreObject* fal_ObjectCarrier::factory( const Falcon::CoreClass *cls, void *user_data, bool )
{
    Falcon::String classname = cls->symbol()->name();

    // do not allow direct instantiation of the base classes, except ActiveRect
    // with rect, it is good to replace member functions such as: rect.OnEnter = some_func
    if(classname == "Player" || classname == "Unit"
    || classname == "Item" || classname == "Object")
    {
        throw new Falcon::AccessError( Falcon::ErrorParam( Falcon::e_noninst_cls ) );
    }

    // TODO: automatic class selection does NOT work for chain inheritance (no idea why):
    // e.g. Olaf from PlayerEx from Player

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
    else if(clsdef->inheritsFrom("ActiveRect") || classname == "ActiveRect")
    {
        obj = new ActiveRect;
    }

    if(!obj)
    {
        Falcon::AutoCString cclsname(classname);
        logerror("ObjectCarrier factory: Unable to instantiate an object of class '%s'", cclsname.c_str());
        throw new Falcon::AccessError( Falcon::ErrorParam( Falcon::e_noninst_cls ) );
    }

    FalconProxyObject *fobj = new FalconProxyObject(obj);
    return new fal_ObjectCarrier(cls, fobj);
}



bool fal_ObjectCarrier::setProperty( const Falcon::String &prop, const Falcon::Item &value )
{
    if(!_obj)
    {
        throw new Falcon::AccessError( Falcon::ErrorParam( Falcon::e_invop_unb ).
            extra( "Object was already deleted!" ) );   
    }

    bool rectChanged = false;

    if(prop == "x") { ((ActiveRect*)_obj)->x = float(value.forceNumeric()); rectChanged = true; }
    else if(prop == "y") { ((ActiveRect*)_obj)->y = float(value.forceNumeric()); rectChanged = true; }
    else if(prop == "w") { ((ActiveRect*)_obj)->w = value.forceInteger(); rectChanged = true; }
    else if(prop == "h") { ((ActiveRect*)_obj)->h = value.forceInteger(); rectChanged = true; }
    if(rectChanged)
    {
        if(_obj->GetType() >= OBJTYPE_OBJECT)
            ((Object*)_obj)->UpdateAnchor();
        ((ActiveRect*)_obj)->HasMoved();
        return true;
    }

    // faster check for x2, y2, x2f, y2f
    if(prop.length() > 1 && prop.length() <= 3 && prop.getCharAt(1) == '2' && (prop.getCharAt(0) == 'x' || prop.getCharAt('y')))
    {
        throw new Falcon::AccessError( Falcon::ErrorParam( Falcon::e_prop_ro ).
            extra( prop ) );   
    }

    if(prop == "phys")
    {
        if(!value.isOfClass("PhysProps"))
        {
            throw new Falcon::AccessError( Falcon::ErrorParam( Falcon::e_param_type ).
                extra( "Object.phys can only be of type 'PhysProps'" ) );   
        }

        if(_obj->GetType() >= OBJTYPE_OBJECT)
        {
            ((Object*)_obj)->phys = *((fal_PhysProps*)value.asObject())->GetPhysProps(); // clone phys
        }

        return true; // if trying to assign phys to an ActiveRect, simply nothing will happen
    }

    return FalconObject::setProperty(prop, value);
}

bool fal_ObjectCarrier::getProperty( const Falcon::String &prop, Falcon::Item &ret ) const
{
    if(!_obj)
    {
        throw new Falcon::AccessError( Falcon::ErrorParam( Falcon::e_invop_unb ).
            extra( "Object was already deleted!" ) );   
    }

    if(prop == "id") { ret = Falcon::int32(_obj->GetId()); return true; }
    if(prop == "type") { ret = Falcon::int32(_obj->GetType()); return true; }
    if(prop == "x") { ret = Falcon::numeric(((ActiveRect*)_obj)->x); return true; }
    if(prop == "y") { ret = Falcon::numeric(((ActiveRect*)_obj)->y); return true; }
    if(prop == "w") { ret = Falcon::int32(((ActiveRect*)_obj)->w); return true; }
    if(prop == "h") { ret = Falcon::int32(((ActiveRect*)_obj)->h); return true; }
    if(prop == "x2") { ret = Falcon::int32(((ActiveRect*)_obj)->x2()); return true; }
    if(prop == "y2") { ret = Falcon::int32(((ActiveRect*)_obj)->y2()); return true; }
    if(prop == "x2f") { ret = Falcon::numeric(((ActiveRect*)_obj)->x2f()); return true; }
    if(prop == "y2f") { ret = Falcon::numeric(((ActiveRect*)_obj)->y2f()); return true; }
    if(prop == "phys")
    {
        if(_obj->GetType() < OBJTYPE_OBJECT)
            ret.setNil();
        else
        {
            Falcon::CoreClass *cls = _falObj->vm->findWKI("PhysProps")->asClass();
            fal_PhysProps *p = new fal_PhysProps(cls, ((Object*)_obj)->phys, true); // as reference
            ret.setObject(p);
        }
        return true;
    }

    return FalconObject::getProperty( prop, ret) || defaultProperty( prop, ret); // property not found
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

void ActiveRect::OnEnter(uint8 side, ActiveRect *who)
{
    DEBUG_ASSERT_RETURN_VOID(_falObj);
    _falObj->CallMethod("OnEnter", 2, Falcon::Item(Falcon::int32(side)), Falcon::Item(who->_falObj->self()));
}

void ActiveRect::OnLeave(uint8 side, ActiveRect *who)
{
    DEBUG_ASSERT_RETURN_VOID(_falObj);
    _falObj->CallMethod("OnLeave", 2, Falcon::Item(Falcon::int32(side)), Falcon::Item(who->_falObj->self()));
}

bool ActiveRect::OnTouch(uint8 side, ActiveRect *who)
{
    DEBUG_ASSERT_RETURN(_falObj, true); // no further processing
    Falcon::Item *result = _falObj->CallMethod("OnTouch", 2, Falcon::Item(Falcon::int32(side)), Falcon::Item(who->_falObj->self()));
    return result ? result->asBoolean() : false;
}

void Object::OnUpdate(uint32 ms)
{
    DEBUG_ASSERT_RETURN_VOID(_falObj);
    _falObj->CallMethod("OnUpdate", 1, Falcon::Item(Falcon::int64(ms)));
}

bool Item::OnUse(Object *who)
{
    DEBUG_ASSERT_RETURN(_falObj, false); // no further processing
    Falcon::Item *result = _falObj->CallMethod("OnUse", 1, Falcon::Item(who->_falObj->self()));
    return result ? result->asBoolean() : false;
}

// -- end object proxy calls --


// correctly remove an object from the Mgr, clean up its Falcon bindings, and delete it
FALCON_FUNC fal_BaseObject_Remove(Falcon::VMachine *vm)
{
    fal_ObjectCarrier *self = Falcon::dyncast<fal_ObjectCarrier*>( vm->self().asObject() );
    BaseObject *obj = self->GetObj();
    g_engine_ptr->objmgr->Remove(obj->GetId());
    obj->unbind();
    delete obj;
}

FALCON_FUNC fal_ActiveRect_SetBBox(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS_EXTRA(4, "Int x, Int y, Uint width, Uint height");
    int32 x = vm->param(0)->forceInteger();
    int32 y = vm->param(1)->forceInteger();
    uint32 w = vm->param(2)->forceInteger();
    uint32 h = vm->param(3)->forceInteger();
    fal_ObjectCarrier *self = Falcon::dyncast<fal_ObjectCarrier*>( vm->self().asObject() );
    ActiveRect *obj = (ActiveRect*)self->GetObj();
    obj->SetBBox(x,y,w,h);
    if(obj->GetType() >= OBJTYPE_OBJECT)
        ((Object*)obj)->UpdateAnchor();
    obj->HasMoved();
}

FALCON_FUNC fal_ActiveRect_SetPos(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS_EXTRA(2, "Int x, Int y");
    int32 x = vm->param(0)->forceInteger();
    int32 y = vm->param(1)->forceInteger();
    fal_ObjectCarrier *self = Falcon::dyncast<fal_ObjectCarrier*>( vm->self().asObject() );
    ActiveRect *obj = (ActiveRect*)self->GetObj();
    obj->SetPos(x,y);
    if(obj->GetType() >= OBJTYPE_OBJECT)
        ((Object*)obj)->UpdateAnchor();
    obj->HasMoved();
}

FALCON_FUNC fal_Object_SetAffectedByPhysics(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS(1);
    fal_ObjectCarrier *self = Falcon::dyncast<fal_ObjectCarrier*>( vm->self().asObject() );
    if(self->GetObj()->GetType() >= OBJTYPE_OBJECT)
    {
        Object *obj = (Object*)self->GetObj();
        obj->SetAffectedByPhysics(vm->param(0)->asBoolean());
    }
}

FALCON_FUNC fal_Object_IsAffectedByPhysics(Falcon::VMachine *vm)
{
    fal_ObjectCarrier *self = Falcon::dyncast<fal_ObjectCarrier*>( vm->self().asObject() );
    if(self->GetObj()->GetType() >= OBJTYPE_OBJECT)
    {
        Object *obj = (Object*)self->GetObj();
        vm->retval(obj->IsAffectedByPhysics());
    }
    vm->retval(false);
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

    Falcon::CoreObject *clone() const
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
            if(SDL_Surface *img = resMgr.LoadImg((char*)file.c_str()))
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

FALCON_FUNC fal_Object_SetSprite(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS_EXTRA(1, "S filename / Tile / AnimatedTile");
    Falcon::Item *param = vm->param(0);
    fal_ObjectCarrier *self = Falcon::dyncast<fal_ObjectCarrier*>( vm->self().asObject() );
    if(param->isString())
    {
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
            if(SDL_Surface *img = resMgr.LoadImg((char*)file.c_str()))
            {
                tile = new BasicTile;
                tile->surface = img;
                tile->filename = file;   
            }
        }
        if(tile)
        {
            ((Object*)self->GetObj())->SetSprite(tile);
            Falcon::CoreClass *cls = vm->findWKI("Tile")->asClass();
            vm->retval(new fal_Tile(cls,tile));
        }
        else
            vm->retnil();
    }
    else if(param->isOfClass("Tile"))
    {
        fal_Tile *ftile = Falcon::dyncast<fal_Tile*>(param->asObject());
        ((Object*)self->GetObj())->SetSprite(ftile->GetTile());
        vm->retval(ftile);
    }
    else
    {
        throw new Falcon::AccessError( Falcon::ErrorParam( Falcon::e_param_type ).
            extra( "Expected: S filename / Tile / AnimatedTile" ) );
    }
}

FALCON_FUNC fal_Object_GetSprite(Falcon::VMachine *vm)
{
    fal_ObjectCarrier *self = Falcon::dyncast<fal_ObjectCarrier*>( vm->self().asObject() );
    Falcon::CoreClass *cls = vm->findWKI("Tile")->asClass();
    vm->retval(new fal_Tile(cls, ((Object*)self->GetObj())->GetSprite()));
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

FALCON_FUNC fal_TileLayer_SetVisible(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS(1);
    fal_TileLayer *self = Falcon::dyncast<fal_TileLayer*>( vm->self().asObject() );
    self->GetLayer()->visible = (bool)vm->param(0)->asBoolean();
}

FALCON_FUNC fal_TileLayer_IsVisible(Falcon::VMachine *vm)
{
    fal_TileLayer *self = Falcon::dyncast<fal_TileLayer*>( vm->self().asObject() );
    vm->retval(self->GetLayer()->visible);
}

FALCON_FUNC fal_TileLayer_SetTile(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS_EXTRA(3, "u32 x, u32 y, S filename / tile-ref [, bool updateCollision = true]");
    fal_TileLayer *self = Falcon::dyncast<fal_TileLayer*>( vm->self().asObject() );
    uint32 x = vm->param(0)->forceInteger();
    uint32 y = vm->param(1)->forceInteger();


    if(x >= self->GetLayer()->GetArraySize() || x >= self->GetLayer()->GetArraySize())
    {
        vm->retval(false);
        return;
    }

    Falcon::Item *tileArg = vm->param(2);
    bool updateCollision = true; // true by default in LayerMgr::SetTile()
    if(vm->paramCount() > 3)
        updateCollision = vm->param(3)->asBoolean();

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
            if(SDL_Surface *img = resMgr.LoadImg((char*)file.c_str()))
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
        self->GetLayer()->SetTile(x,y,NULL,updateCollision);
        vm->retval(true);
    }

    if(tile)
    {
        self->GetLayer()->SetTile(x,y,tile,updateCollision);
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
        if(SDL_Surface *img = resMgr.LoadImg((char*)file.c_str()))
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
    vm->retval(Falcon::int64(g_engine_ptr->GetCurFrameTime()));
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
    else if(clsdef->inheritsFrom("ActiveRect") || vm->param(0)->asString()->compare("ActiveRect") == 0)
    {
        obj = new ActiveRect;
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

    g_engine_ptr->objmgr->Add(obj);

    vm->retval(carrier);
}
*/

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
    // TODO: implement this!
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

FALCON_FUNC fal_Physics_SetGravity(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS(1);
    g_engine_ptr->physmgr->envPhys.gravity = float(vm->param(0)->forceNumeric());
}

FALCON_FUNC fal_Physics_GetGravity(Falcon::VMachine *vm)
{
    vm->retval(g_engine_ptr->physmgr->envPhys.gravity);
}


void forbidden_init(Falcon::VMachine *vm)
{
    throw new Falcon::AccessError( Falcon::ErrorParam( Falcon::e_noninst_cls ) );
}

FALCON_FUNC fal_include_ex( Falcon::VMachine *vm )
{
    Falcon::Item *i_file = vm->param(0);
    Falcon::Item *i_enc = vm->param(1);
    Falcon::Item *i_path = vm->param(2);
    Falcon::Item *i_syms = vm->param(3);

    if( i_file == 0 || ! i_file->isString()
        || (i_syms != 0 && ! (i_syms->isDict() || i_syms->isNil())  )
        || (i_enc != 0 && !(i_enc->isString() || i_enc->isNil()) )
        || (i_path != 0 && !(i_path->isString() || i_path->isNil()) )
        )
    {
        throw new Falcon::ParamError(
            Falcon::ErrorParam( Falcon::e_inv_params, __LINE__ )
            .origin(Falcon::e_orig_runtime)
            .extra( "S,[S],[S],[D]" ) );
    }

    // create the loader/runtime pair.
    Falcon::ModuleLoader cpl( i_path == 0 || i_path->isNil() ? vm->appSearchPath() : Falcon::String(*i_path->asString()) );
    cpl.delayRaise(true);
    cpl.compileTemplate(false);
    cpl.compileInMemory(true);
    cpl.alwaysRecomp(true);
    cpl.saveModules(false);
    Falcon::Runtime rt( &cpl, vm );
    rt.hasMainModule( false );

    // minimal config
    if ( i_enc != 0 && ! i_enc->isNil() )
    {
        cpl.sourceEncoding( *i_enc->asString() );
    }

    bool execAtLink = vm->launchAtLink();

    //! Copy the filename so to be sure to display it correctly in an eventual error.
    Falcon::String fileName = *i_file->asString();
    fileName.bufferize();

    // load and link
    try
    {
        rt.loadFile( fileName, false );
        vm->launchAtLink( i_syms == 0 || i_syms->isNil() );
        Falcon::LiveModule *lmod = vm->link( &rt );

        // shall we read the symbols?
        if( lmod != 0 && ( i_syms != 0 && i_syms->isDict() ) )
        {
            Falcon::CoreDict *dict = i_syms->asDict();

            // traverse the dictionary
            Falcon::Iterator iter( &dict->items() );
            while( iter.hasCurrent() )
            {
                // if the key is a string and a corresponding item is found...
                Falcon::Item *ival;
                if ( iter.getCurrentKey().isString() &&
                    ( ival = lmod->findModuleItem( *iter.getCurrentKey().asString() ) ) != 0 )
                {
                    // copy it locally
                    iter.getCurrent() = *ival;
                }
                else {
                    iter.getCurrent().setNil();
                }

                iter.next();
            }
        }

        // reset launch status
        vm->launchAtLink( execAtLink );
    }
    catch(Falcon::Error* err)
    {
        Falcon::CodeError *ce = new Falcon::CodeError( Falcon::ErrorParam( Falcon::e_loaderror, __LINE__ ).
            extra( fileName ) );

        ce->appendSubError(err);
        err->decref();

        // reset launch status
        vm->launchAtLink( execAtLink );
        throw ce;
    }
}


Falcon::Module *FalconGameModule_create(void)
{
    Falcon::Module *m = new Falcon::Module;
    m->name("GameModule");

    m->addExtFunc("include_ex", fal_include_ex);

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
    m->addClassMethod(clsGame, "LoadPropsInDir", fal_Game_LoadPropsInDir);
    m->addConstant("MAX_VOLUME", Falcon::int64(MIX_MAX_VOLUME));

    Falcon::Symbol *symPhysics = m->addSingleton("Physics");
    Falcon::Symbol *clsPhysics = symPhysics->getInstance();
    m->addClassMethod(clsPhysics, "SetGravity", fal_Physics_SetGravity);
    m->addClassMethod(clsPhysics, "GetGravity", fal_Physics_GetGravity);


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

    Falcon::Symbol *clsRect = m->addClass("ActiveRect", &fal_ObjectCarrier::init);
    clsRect->getClassDef()->addInheritance(inhBaseObject);
    Falcon::InheritDef *inhRect = new Falcon::InheritDef(clsRect); // there are other classes that inherit from ActiveRect
    clsRect->setWKS(true);
    
    m->addClassMethod(clsRect, "OnEnter", fal_NullFunc);
    m->addClassMethod(clsRect, "OnLeave", fal_NullFunc);
    m->addClassMethod(clsRect, "OnTouch", fal_NullFunc);
    m->addClassMethod(clsRect, "SetBBox", fal_ActiveRect_SetBBox);
    m->addClassMethod(clsRect, "SetPos", fal_ActiveRect_SetPos);
    m->addClassProperty(clsRect, "x");
    m->addClassProperty(clsRect, "y");
    m->addClassProperty(clsRect, "w");
    m->addClassProperty(clsRect, "h");
    m->addClassProperty(clsRect, "x2");
    m->addClassProperty(clsRect, "y2");
    m->addClassProperty(clsRect, "x2f");
    m->addClassProperty(clsRect, "y2f");

    Falcon::Symbol *clsObject = m->addClass("Object", &fal_ObjectCarrier::init);
    clsObject->getClassDef()->addInheritance(inhRect);
    Falcon::InheritDef *inhObject = new Falcon::InheritDef(clsObject); // there are other classes that inherit from Object
    clsObject->setWKS(true);
    m->addClassMethod(clsObject, "OnUpdate", fal_NullFunc);
    m->addClassMethod(clsObject, "SetSprite", fal_Object_SetSprite);
    m->addClassMethod(clsObject, "GetSprite", fal_Object_GetSprite);
    m->addClassMethod(clsObject, "SetAffectedByPhysics", &fal_Object_SetAffectedByPhysics);
    m->addClassMethod(clsObject, "IsAffectedByPhysics", &fal_Object_IsAffectedByPhysics);
    m->addClassProperty(clsObject, "phys");

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

    Falcon::Symbol *clsPhysProps = m->addClass("PhysProps");
    clsPhysProps->setWKS(true);
    m->addClassProperty(clsPhysProps, "isRef");
    m->addClassProperty(clsPhysProps, "weight");
    m->addClassProperty(clsPhysProps, "xspeed");
    m->addClassProperty(clsPhysProps, "yspeed");
    m->addClassProperty(clsPhysProps, "xmaxspeed");
    m->addClassProperty(clsPhysProps, "ymaxspeed");
    m->addClassProperty(clsPhysProps, "xaccel");
    m->addClassProperty(clsPhysProps, "yaccel");
    m->addClassProperty(clsPhysProps, "xfriction");
    m->addClassProperty(clsPhysProps, "yfriction");


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


