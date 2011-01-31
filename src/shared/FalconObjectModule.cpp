#include <falcon/engine.h>
#include "common.h"
#include "AppFalcon.h"

#include "Engine.h"
#include "TileLayer.h"
#include "ResourceMgr.h"
#include "LayerMgr.h"
#include "Objects.h"
#include "ObjectMgr.h"
#include "SoundCore.h"
#include "PhysicsSystem.h"

#include "FalconBaseModule.h"
#include "FalconObjectModule.h"

Engine *g_engine_ptr_s = NULL;

void FalconObjectModule_SetEnginePtr(Engine *eng)
{
    g_engine_ptr_s = eng;
}

FalconProxyObject::~FalconProxyObject()
{
    // remove cross-references
    self()->_falObj = NULL;
    self()->_obj = NULL;

    // allow the garbage collector to cleanup the remains (the fal_ObjectCarrier)
    delete gclock;
}

Falcon::Item *FalconProxyObject::CallMethod(const char *m)
{
    Falcon::Item method;
    if(!_PrepareMethod(m, method))
        return NULL;
    return _CallReadyMethod(m, method, 0);
}

Falcon::Item *FalconProxyObject::CallMethod(const char *m, const Falcon::Item& a)
{
    Falcon::Item method;
    if(!_PrepareMethod(m, method))
        return NULL;
    vm->pushParam(a);
    return _CallReadyMethod(m, method, 1);
}

Falcon::Item *FalconProxyObject::CallMethod(const char *m, const Falcon::Item& a, const Falcon::Item& b)
{
    Falcon::Item method;
    if(!_PrepareMethod(m, method))
        return NULL;
    vm->pushParam(a);
    vm->pushParam(b);
    return _CallReadyMethod(m, method, 2);
}

Falcon::Item *FalconProxyObject::CallMethod(const char *m, const Falcon::Item& a, const Falcon::Item& b, const Falcon::Item& c)
{
    Falcon::Item method;
    if(!_PrepareMethod(m, method))
        return NULL;
    vm->pushParam(a);
    vm->pushParam(b);
    vm->pushParam(c);
    return _CallReadyMethod(m, method, 3);
}

bool FalconProxyObject::_PrepareMethod(const char *m, Falcon::Item &mth)
{
    return self()->getMethod(m, mth); // checking for mth.isCallable() is not required here, done by the VM
}

Falcon::Item *FalconProxyObject::_CallReadyMethod(const char *mthname, const Falcon::Item& mth, uint32 args)
{
    try
    {
        vm->callItem(mth, args);
        return &(vm->regA());
    }
    catch(Falcon::Error *err)
    {
        Falcon::AutoCString edesc( err->toString() );
        logerror("FalconProxyObject::CallMethod(%s): %s", mthname, edesc.c_str());
        err->decref();
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
        if(prop == "ubounce")     { _phys->ubounce     = value.forceNumeric(); return true; }
        if(prop == "dbounce")     { _phys->dbounce     = value.forceNumeric(); return true; }
        if(prop == "lbounce")     { _phys->lbounce     = value.forceNumeric(); return true; }
        if(prop == "rbounce")     { _phys->rbounce     = value.forceNumeric(); return true; }

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
        if(prop == "ubounce")     { ret.setNumeric(_phys->ubounce);     return true; }
        if(prop == "dbounce")     { ret.setNumeric(_phys->dbounce);     return true; }
        if(prop == "lbounce")     { ret.setNumeric(_phys->lbounce);     return true; }
        if(prop == "rbounce")     { ret.setNumeric(_phys->rbounce);     return true; }
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
    obj->SetLayerMgr(g_engine_ptr_s->_GetLayerMgr());
    obj->Init(); // correctly set type of object
    obj->_falObj = fobj;
    g_engine_ptr_s->objmgr->Add(obj);
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
    if(_obj->GetType() >= OBJTYPE_OBJECT)
    {
        if(prop == "gfxOffsX") { ((Object*)_obj)->gfxoffsx = int32(value.forceInteger()); return true; }
        if(prop == "gfxOffsY") { ((Object*)_obj)->gfxoffsy = int32(value.forceInteger()); return true; }
    }

    return FalconObject::setProperty(prop, value);
}

bool fal_ObjectCarrier::getProperty( const Falcon::String &prop, Falcon::Item &ret ) const
{
    if(prop == "valid")
    {
        ret = _obj != NULL;
        return true;
    }
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
    if(_obj->GetType() >= OBJTYPE_OBJECT)
    {
        if(prop == "gfxOffsX") { ret = Falcon::int32(((Object*)_obj)->gfxoffsx); return true; }
        if(prop == "gfxOffsY") { ret = Falcon::int32(((Object*)_obj)->gfxoffsy); return true; }

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
    _falObj->CallMethod("OnEnter", Falcon::int32(side), who->_falObj->self());
}

void ActiveRect::OnEnteredBy(uint8 side, ActiveRect *who)
{
    DEBUG_ASSERT_RETURN_VOID(_falObj);
    _falObj->CallMethod("OnEnteredBy", Falcon::int32(side), who->_falObj->self());
}

void ActiveRect::OnLeave(uint8 side, ActiveRect *who)
{
    DEBUG_ASSERT_RETURN_VOID(_falObj);
    _falObj->CallMethod("OnLeave", Falcon::int32(side), who->_falObj->self());
}

void ActiveRect::OnLeftBy(uint8 side, ActiveRect *who)
{
    DEBUG_ASSERT_RETURN_VOID(_falObj);
    _falObj->CallMethod("OnLeftBy", Falcon::int32(side), who->_falObj->self());
}

bool ActiveRect::OnTouch(uint8 side, ActiveRect *who)
{
    DEBUG_ASSERT_RETURN(_falObj, true); // no further processing
    Falcon::Item *result = _falObj->CallMethod("OnTouch", Falcon::int32(side), who->_falObj->self());
    return result && result->isTrue();
}

bool ActiveRect::OnTouchedBy(uint8 side, ActiveRect *who)
{
    DEBUG_ASSERT_RETURN(_falObj, true); // no further processing
    Falcon::Item *result = _falObj->CallMethod("OnTouchedBy", Falcon::int32(side), who->_falObj->self());
    return result && result->isTrue();
}

void Object::OnUpdate(uint32 ms)
{
    DEBUG_ASSERT_RETURN_VOID(_falObj);
    _falObj->CallMethod("OnUpdate", Falcon::int64(ms));
}

void Object::OnTouchWall(uint8 side, float xspeed, float yspeed)
{
    DEBUG_ASSERT_RETURN_VOID(_falObj);
    _falObj->CallMethod("OnTouchWall", Falcon::int64(side), Falcon::numeric(xspeed), Falcon::numeric(yspeed));
}

bool Item::OnUse(Object *who)
{
    DEBUG_ASSERT_RETURN(_falObj, false); // no further processing
    Falcon::Item *result = _falObj->CallMethod("OnUse", who->_falObj->self());
    return result && result->type() != FLC_ITEM_NIL ? result->asBoolean() : false;
}

// -- end object proxy calls --


// correctly remove an object from the Mgr, clean up its Falcon bindings, and delete it
FALCON_FUNC fal_BaseObject_Remove(Falcon::VMachine *vm)
{
    fal_ObjectCarrier *self = Falcon::dyncast<fal_ObjectCarrier*>( vm->self().asObject() );
    BaseObject *obj = self->GetObj();
    if(obj) // it can happen that the object is already deleted here, and the ptr is NULL
        obj->SetDelete();
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

FALCON_FUNC fal_ActiveRect_CanMoveToDir(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS_EXTRA(1, "N[,N]");
    uint8 d = vm->param(0)->forceIntegerEx();
    fal_ObjectCarrier *self = Falcon::dyncast<fal_ObjectCarrier*>( vm->self().asObject() );
    if(vm->paramCount() >= 2)
    {
        uint32 pixels = vm->param(1)->forceIntegerEx();
        vm->retval((int64)((ActiveRect*)self->GetObj())->CanMoveToDirection(d, pixels));
    }
    else
    {
        vm->retval((int64)((ActiveRect*)self->GetObj())->CanMoveToDirection(d)); // use builtin default value
    }
}

FALCON_FUNC fal_ActiveRect_SetCollisionEnabled(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS(1);
    fal_ObjectCarrier *self = Falcon::dyncast<fal_ObjectCarrier*>( vm->self().asObject() );
    ActiveRect *obj = (ActiveRect*)self->GetObj();
    obj->SetCollisionEnabled(vm->param(0)->asBoolean());
}

FALCON_FUNC fal_ActiveRect_IsCollisionEnabled(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS(1);
    fal_ObjectCarrier *self = Falcon::dyncast<fal_ObjectCarrier*>( vm->self().asObject() );
    ActiveRect *obj = (ActiveRect*)self->GetObj();
    vm->retval(obj->IsCollisionEnabled());
}

FALCON_FUNC fal_ActiveRect_GetDistanceX(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS(1);
    fal_ObjectCarrier *self = Falcon::dyncast<fal_ObjectCarrier*>( vm->self().asObject() );
    fal_ObjectCarrier *other = Falcon::dyncast<fal_ObjectCarrier*>( vm->param(0)->asObject() );
    ActiveRect *obj = (ActiveRect*)self->GetObj();
    ActiveRect *otherobj = (ActiveRect*)other->GetObj();
    vm->retval(obj->GetDistanceX(otherobj));
}

FALCON_FUNC fal_ActiveRect_GetDistanceY(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS(1);
    fal_ObjectCarrier *self = Falcon::dyncast<fal_ObjectCarrier*>( vm->self().asObject() );
    fal_ObjectCarrier *other = Falcon::dyncast<fal_ObjectCarrier*>( vm->param(0)->asObject() );
    ActiveRect *obj = (ActiveRect*)self->GetObj();
    ActiveRect *otherobj = (ActiveRect*)other->GetObj();
    vm->retval(obj->GetDistanceY(otherobj));
}

FALCON_FUNC fal_ActiveRect_GetDistance(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS(1);
    fal_ObjectCarrier *self = Falcon::dyncast<fal_ObjectCarrier*>( vm->self().asObject() );
    fal_ObjectCarrier *other = Falcon::dyncast<fal_ObjectCarrier*>( vm->param(0)->asObject() );
    ActiveRect *obj = (ActiveRect*)self->GetObj();
    ActiveRect *otherobj = (ActiveRect*)other->GetObj();
    vm->retval(obj->GetDistance(otherobj));
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

fal_Tile::fal_Tile( const Falcon::CoreClass* generator, BasicTile *obj )
: Falcon::CoreObject( generator ), _tile(obj)
{
    if(obj)
        obj->ref++;
}

bool fal_Tile::finalize(void)
{
    if(_tile)
        _tile->ref--; // the GC will eat the carrier, making this tile unreferencable
    return false; // this tells the GC to call the destructor
}

void fal_Tile::init(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS_EXTRA(1, "S filename");
    Falcon::AutoCString file(vm->param(0)->asString());
    fal_Tile *self = Falcon::dyncast<fal_Tile*>( vm->self().asObject() );

    if(BasicTile *tile = AnimatedTile::New(file.c_str())) // new tile, refcount is initialized with 1
    {
        // we are picking up a newly created tile, but nobody else holds it, must NOT increase refcount
        ASSERT(self->_tile == NULL); // at an init() call, we must not hold a tile yet
        self->_tile = tile;
    }
    else
        vm->self().setNil();
}

bool fal_Tile::setProperty( const Falcon::String &prop, const Falcon::Item &value )
{
    bool bad = false;

    if(prop == "name")
    {
        if(_tile->GetType() == TILETYPE_ANIMATED)
        {
            Falcon::AutoCString cname(value);
            ((AnimatedTile*)_tile)->SetName((char*)cname.c_str());
            return true;
        }
        else
            bad = true;
    }
    else if(prop == "type" || prop == "filename" || prop == "frame")
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

bool fal_Tile::getProperty( const Falcon::String &prop, Falcon::Item &ret ) const
{
    if(!_tile)
    {
        ret.setNil();
        return true;
    }
    if(prop == "type")
    {
        ret = (Falcon::uint32)(_tile->GetType());
        return true;
    }
    else if(prop == "name")
    {
        if(_tile->GetType() == TILETYPE_ANIMATED)
            ret = Falcon::String(((AnimatedTile*)_tile)->GetName());
        else
            ret.setNil();

        return true; // found the property
    }
    else if(prop == "frame")
    {
        if(_tile->GetType() == TILETYPE_ANIMATED)
            ret = (Falcon::int32)(((AnimatedTile*)_tile)->GetFrame());
        else
            ret.setNil();

        return true; // found the property
    }
    else if(prop == "filename")
    {
        ret = Falcon::String(_tile->GetFilename());
        return true;
    }
    else
    {
        return defaultProperty( prop, ret); // property not found
    }

    return false;
}

FALCON_FUNC fal_Object_SetSprite(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS_EXTRA(1, "nil / S filename / Tile / AnimatedTile");
    Falcon::Item *param = vm->param(0);
    fal_ObjectCarrier *self = Falcon::dyncast<fal_ObjectCarrier*>( vm->self().asObject() );
    if(param->isString())
    {
        Falcon::AutoCString file(vm->param(0)->asString());

        if(BasicTile *tile = AnimatedTile::New(file.c_str()))
        {
            ((Object*)self->GetObj())->SetSprite(tile);
            Falcon::CoreClass *cls = vm->findWKI("Tile")->asClass();
            vm->retval(new fal_Tile(cls,tile));
            tile->ref--; // the tile was passed to the carrier, means it isn't referenced here anymore
        }
        else
            vm->retnil();
    }
    else if(param->isNil())
    {
        ((Object*)self->GetObj())->SetSprite(NULL); // refcounting done in SetSprite()
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

FALCON_FUNC fal_Object_SetLayerId(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS_EXTRA(1, "N in [0..31]");
    fal_ObjectCarrier *self = Falcon::dyncast<fal_ObjectCarrier*>( vm->self().asObject() );
    uint32 id = vm->param(0)->forceIntegerEx();
    if(id >= LAYER_MAX)
    {
        throw new Falcon::AccessError( Falcon::ErrorParam( Falcon::e_arracc ).
            extra( "N in [0..31]" ) );
    }

    BaseObject *obj = self->GetObj();
    if(obj->GetType() >= OBJTYPE_OBJECT)
        ((Object*)obj)->SetLayer(id);
}

FALCON_FUNC fal_Object_GetLayerId(Falcon::VMachine *vm)
{
    fal_ObjectCarrier *self = Falcon::dyncast<fal_ObjectCarrier*>( vm->self().asObject() );
    BaseObject *obj = self->GetObj();
    if(obj->GetType() >= OBJTYPE_OBJECT)
        vm->retval((Falcon::int64)((Object*)obj)->GetLayer());
    else
        vm->retnil();
}

FALCON_FUNC fal_Object_GetSprite(Falcon::VMachine *vm)
{
    fal_ObjectCarrier *self = Falcon::dyncast<fal_ObjectCarrier*>( vm->self().asObject() );
    Falcon::CoreClass *cls = vm->findWKI("Tile")->asClass();
    if(BasicTile *tile = ((Object*)self->GetObj())->GetSprite())
        vm->retval(new fal_Tile(cls, tile));
    else
        vm->retnil();
}

FALCON_FUNC fal_Object_CanFallDown(Falcon::VMachine *vm)
{
    fal_ObjectCarrier *self = Falcon::dyncast<fal_ObjectCarrier*>( vm->self().asObject() );
    vm->retval(((Object*)self->GetObj())->CanFallDown());
}

FALCON_FUNC fal_Object_IsBlocking(Falcon::VMachine *vm)
{
    fal_ObjectCarrier *self = Falcon::dyncast<fal_ObjectCarrier*>( vm->self().asObject() );
    vm->retval(((Object*)self->GetObj())->IsBlocking());
}

FALCON_FUNC fal_Object_SetBlocking(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS_EXTRA(1,"B");
    bool block = vm->param(0)->asBoolean();
    fal_ObjectCarrier *self = Falcon::dyncast<fal_ObjectCarrier*>( vm->self().asObject() );
    ((Object*)self->GetObj())->SetBlocking(block);
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


    if(x >= self->GetLayer()->GetArraySize() || y >= self->GetLayer()->GetArraySize())
    {
        throw new Falcon::AccessError( Falcon::ErrorParam( Falcon::e_arracc ) );
    }

    Falcon::Item *tileArg = vm->param(2);
    bool updateCollision = true; // true by default in LayerMgr::SetTile()
    if(vm->paramCount() > 3)
        updateCollision = vm->param(3)->asBoolean();

    BasicTile *tile = NULL;
    bool isNewTile = false;

    // support direct string-in and auto-convert to a tile
    if(tileArg->isString())
    {
        Falcon::AutoCString file(vm->param(2)->asString());
        tile = AnimatedTile::New(file.c_str());
        isNewTile = true;
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
        if(isNewTile)
            tile->ref--; // we created this tile, now passed to TileLayer -> we are not holding it anymore.
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

FALCON_FUNC fal_TileLayer_SetCollisionEnabled(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS_EXTRA(1, "B");
    fal_TileLayer *self = Falcon::dyncast<fal_TileLayer*>( vm->self().asObject() );
    self->GetLayer()->collision = vm->param(0)->asBoolean();
}

FALCON_FUNC fal_TileLayer_IsCollisionEnabled(Falcon::VMachine *vm)
{
    fal_TileLayer *self = Falcon::dyncast<fal_TileLayer*>( vm->self().asObject() );
    vm->retval(self->GetLayer()->collision);
}

FALCON_FUNC fal_Objects_GetLastId(Falcon::VMachine *vm)
{
    vm->retval((Falcon::int64)g_engine_ptr_s->objmgr->GetLastId());
}

FALCON_FUNC fal_Objects_GetCount(Falcon::VMachine *vm)
{
    vm->retval((Falcon::int64)g_engine_ptr_s->objmgr->GetCount());
}

FALCON_FUNC fal_Objects_Get(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS_EXTRA(1, "N");
    BaseObject *obj = g_engine_ptr_s->objmgr->Get(vm->param(0)->forceIntegerEx());
    if(!obj)
    {
        vm->retnil();
        return;
    }
    DEBUG(ASSERT(obj->_falObj && obj->_falObj->coreCls));
    fal_ObjectCarrier *co = new fal_ObjectCarrier(obj->_falObj->coreCls, obj->_falObj);
    vm->retval(co);
}

FALCON_FUNC fal_Objects_GetAllInRect(Falcon::VMachine *vm)
{
    Falcon::Item *p0 = vm->param(0);
    BaseRect rect;
    if(p0 && p0->isOfClass("ActiveRect"))
    {
        fal_ObjectCarrier *carrier = Falcon::dyncast<fal_ObjectCarrier*>(p0->asObject());
        ActiveRect *arect = Falcon::dyncast<ActiveRect*>(carrier->GetObj());
        rect = arect->cloneRect();
    }
    else
    {
        Falcon::Item *p1 = vm->param(1);
        Falcon::Item *p2 = vm->param(2);
        Falcon::Item *p3 = vm->param(3);
        if(p0 && p1 && p2 && p3)
        {
            rect.x = float(p0->forceNumeric());
            rect.y = float(p1->forceNumeric());
            rect.w = uint32(p2->forceInteger());
            rect.h = uint32(p3->forceInteger());
        }
        else
        {
            throw new Falcon::ParamError(Falcon::ErrorParam( Falcon::e_inv_params, __LINE__ )
                .extra("Obj or {N x, N y, N width, N height}") );
        }
    }
    
    ObjectWithSideSet li;
    g_engine_ptr_s->objmgr->GetAllObjectsIn(rect, li);
    if(li.empty())
    {
        vm->retnil();
        return;
    }
    Falcon::CoreArray *arr = new Falcon::CoreArray(li.size());
    for(ObjectWithSideSet::iterator it = li.begin(); it != li.end(); it++)
    {
        BaseObject *obj = it->first;
        DEBUG(ASSERT(obj->_falObj && obj->_falObj->coreCls));
        fal_ObjectCarrier *co = new fal_ObjectCarrier(obj->_falObj->coreCls, obj->_falObj);
        arr->append(co);
    }
    vm->retval(arr);
}




Falcon::Module *FalconObjectModule_create(void)
{
    Falcon::Module *m = new Falcon::Module;
    m->name("ObjectModule");

    Falcon::Symbol *symObjects = m->addSingleton("Objects");
    Falcon::Symbol *clsObjects = symObjects->getInstance();
    m->addClassMethod(clsObjects, "GetAllInRect", fal_Objects_GetAllInRect);
    m->addClassMethod(clsObjects, "Get", fal_Objects_Get);
    m->addClassMethod(clsObjects, "GetLastId", fal_Objects_GetLastId);
    m->addClassMethod(clsObjects, "GetCount", fal_Objects_GetCount);

    Falcon::Symbol *clsTileLayer = m->addClass("TileLayer", &forbidden_init);
    clsTileLayer->setWKS(true);
    m->addClassMethod(clsTileLayer, "IsVisible", &fal_TileLayer_IsVisible);
    m->addClassMethod(clsTileLayer, "SetVisible", &fal_TileLayer_SetVisible);
    m->addClassMethod(clsTileLayer, "SetTile", &fal_TileLayer_SetTile);
    m->addClassMethod(clsTileLayer, "GetTile", &fal_TileLayer_GetTile);
    m->addClassMethod(clsTileLayer, "GetArraySize", &fal_TileLayer_GetArraySize);
    m->addClassMethod(clsTileLayer, "SetCollisionEnabled", &fal_TileLayer_SetCollisionEnabled);
    m->addClassMethod(clsTileLayer, "IstCollisionEnabled", &fal_TileLayer_IsCollisionEnabled);
    m->addConstant("TILEFLAG_SOLID", (Falcon::int64)TILEFLAG_SOLID, true);

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
    m->addClassProperty(clsBaseObject, "valid");
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
    m->addClassMethod(clsRect, "OnEnteredBy", fal_NullFunc);
    m->addClassMethod(clsRect, "OnLeftBy", fal_NullFunc);
    m->addClassMethod(clsRect, "OnTouchedBy", fal_NullFunc);
    m->addClassMethod(clsRect, "SetBBox", fal_ActiveRect_SetBBox);
    m->addClassMethod(clsRect, "SetPos", fal_ActiveRect_SetPos);
    m->addClassMethod(clsRect, "CanMoveToDir", fal_ActiveRect_CanMoveToDir);
    m->addClassMethod(clsRect, "IsCollisionEnabled", fal_ActiveRect_IsCollisionEnabled);
    m->addClassMethod(clsRect, "SetCollisionEnabled", fal_ActiveRect_SetCollisionEnabled);
    m->addClassMethod(clsRect, "GetDistance", fal_ActiveRect_GetDistance);
    m->addClassMethod(clsRect, "GetDistanceX", fal_ActiveRect_GetDistanceX);
    m->addClassMethod(clsRect, "GetDistanceY", fal_ActiveRect_GetDistanceY);
    m->addClassMethod(clsRect, "SetBlocking", fal_NullFunc); // ActiveRect does not have this
    m->addClassMethod(clsRect, "IsBlocking", fal_FalseFunc); // ActiveRect does not have this, return false
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
    m->addClassMethod(clsObject, "OnTouchWall", fal_NullFunc);
    m->addClassMethod(clsObject, "SetSprite", fal_Object_SetSprite);
    m->addClassMethod(clsObject, "GetSprite", fal_Object_GetSprite);
    m->addClassMethod(clsObject, "SetLayerId", fal_Object_SetLayerId);
    m->addClassMethod(clsObject, "GetLayerId", fal_Object_GetLayerId);
    m->addClassMethod(clsObject, "SetAffectedByPhysics", &fal_Object_SetAffectedByPhysics);
    m->addClassMethod(clsObject, "IsAffectedByPhysics", &fal_Object_IsAffectedByPhysics);
    m->addClassMethod(clsObject, "CanFallDown", &fal_Object_CanFallDown);
    m->addClassMethod(clsRect, "SetBlocking", fal_Object_SetBlocking);
    m->addClassMethod(clsRect, "IsBlocking", fal_Object_IsBlocking);
    m->addClassProperty(clsObject, "phys");
    m->addClassProperty(clsObject, "gfxOffsX");
    m->addClassProperty(clsObject, "gfxOffsY");

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
    m->addClassProperty(clsPhysProps, "ubounce");
    m->addClassProperty(clsPhysProps, "dbounce");
    m->addClassProperty(clsPhysProps, "lbounce");
    m->addClassProperty(clsPhysProps, "rbounce");


    return m;
}
