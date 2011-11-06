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

void fal_ObjectCarrier::init(Falcon::VMachine *vm)
{
    fal_ObjectCarrier *self = Falcon::dyncast<fal_ObjectCarrier*>( vm->self().asObject() );
    FalconProxyObject *fobj = self->GetFalObj();
    BaseObject *obj = self->GetObj();
    fobj->vm = vm;
    fobj->gclock = new Falcon::GarbageLock(Falcon::Item(self));
    obj->SetLayerMgr(Engine::GetInstance()->_GetLayerMgr());
    obj->Init(); // correctly set type of object
    obj->_falObj = fobj;

    Engine::GetInstance()->objmgr->Add(obj);

    Engine::GetInstance()->OnObjectCreated(obj);
}

Falcon::CoreObject* fal_ObjectCarrier::factory( const Falcon::CoreClass *cls, void *user_data, bool )
{
    const Falcon::String& classname = cls->symbol()->name();

    // TODO: automatic class selection does NOT work for chain inheritance (no idea why):
    // e.g. Olaf from PlayerEx from Player

    Falcon::ClassDef *clsdef = cls->symbol()->getClassDef();
    BaseObject *obj = NULL;

    if(clsdef->inheritsFrom("Player") || classname == "Player")
    {
        obj = new Player;
    }
    else if(clsdef->inheritsFrom("Unit") || classname == "Unit")
    {
        obj = new Unit;
    }
    else if(clsdef->inheritsFrom("Object") || classname == "Object")
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

void fal_ObjectCarrier::gcMark(Falcon::uint32 g)
{
    FalconObject::gcMark(g);
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
        ((ActiveRect*)_obj)->HasMoved();
        return true;
    }

    // faster check for x2, y2, x2f, y2f (FIXME: this is not 100% correct)
    if(prop.length() > 1 && prop.length() <= 3 && prop.getCharAt(1) == '2' && (prop.getCharAt(0) == 'x' || prop.getCharAt('y')))
    {
        throw new Falcon::AccessError( Falcon::ErrorParam( Falcon::e_prop_ro ).
            extra( prop ) );
    }

    // convenience accessors, bypassing function overloads
    if(prop == "update") { ((Object*)_obj)->SetUpdate(value.isTrue()); return true; }
    if(prop == "blocking")  { ((Object*)_obj)->SetBlocking(value.isTrue()); return true; }
    if(prop == "collision")
    {
        if(value.isBoolean())
            ((Object*)_obj)->SetCollisionMask(value.isTrue() ? -1 : 0);
        else
            ((Object*)_obj)->SetCollisionMask((uint32)value.forceInteger());
        return true;
    }

    if(_obj->GetType() >= OBJTYPE_OBJECT)
    {
        // convenience accessors, bypassing function overloads
        if(prop == "physics") { ((Object*)_obj)->SetAffectedByPhysics(value.isTrue()); return true; }
        if(prop == "layerId") { ((Object*)_obj)->SetLayer(uint32(value.forceInteger())); return true; }
        if(prop == "visible") { ((Object*)_obj)->SetVisible(value.isTrue()); return true; }
        if(prop == "mass") { ((Object*)_obj)->phys.mass = (float)value.forceNumeric(); return true; }
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
    if(prop == "y2") { ret = Falcon::int32(((ActiveRect*)_obj)->y2()); return true; }

    if(prop == "pos")
    {
        fal_Vector2d *v = new fal_Vector2d(VECTOR_CLASS_SYMBOL2(_obj->_falObj->vm));
        v->vec() = ((ActiveRect*)_obj)->pos;
        ret = v;
        return true;
    }

    if(prop == "size")
    {
        fal_Vector2d *v = new fal_Vector2d(VECTOR_CLASS_SYMBOL2(_obj->_falObj->vm));
        v->vec() = ((ActiveRect*)_obj)->size;
        ret = v;
        return true;
    }

    if(prop == "update") { ret = ((Object*)_obj)->IsUpdate(); return true; }
    if(prop == "blocking")  { ret = ((Object*)_obj)->IsBlocking(); return true; }
    if(prop == "collision") { ret = ((Object*)_obj)->IsCollisionEnabled(); return true; }

    if(_obj->GetType() >= OBJTYPE_OBJECT)
    {
        if(prop == "physics") { ret = ((Object*)_obj)->IsAffectedByPhysics(); return true; }
        if(prop == "layerId") { ret = Falcon::uint32(((Object*)_obj)->GetLayer()); return true; }
        if(prop == "visible") { ret = ((Object*)_obj)->IsVisible(); return true; }
        if(prop == "mass") { ret = Falcon::numeric(((Object*)_obj)->phys.mass); return true; }

        // TODO: quick accessors for speed, accel, friction?
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
    _falObj->CallMethod("OnUpdate", Falcon::int32(ms));
}

void Object::OnTouchWall()
{
    DEBUG_ASSERT_RETURN_VOID(_falObj);
    _falObj->CallMethod("OnTouchWall");
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

FALCON_FUNC fal_BaseObject_compare(Falcon::VMachine *vm)
{
    fal_ObjectCarrier *self = Falcon::dyncast<fal_ObjectCarrier*>( vm->self().asObject() );
    BaseObject *obj = self->GetObj();
    if(obj) // it can happen that the object is already deleted here, and the ptr is NULL
    {
        Falcon::Item *cmp = vm->param(0);
        if(cmp->isOfClass("BaseObject"))
        {
            fal_ObjectCarrier *other = Falcon::dyncast<fal_ObjectCarrier*>(cmp->asObject());
            if(BaseObject *otherObj = other->GetObj())
            {
                vm->retval(Falcon::int32(obj->GetId() - otherObj->GetId())); // if obj < otherObj, this will return < 0.. like falcon expects
                return;
            }
        }
    }
    vm->retnil();
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
}

FALCON_FUNC fal_ActiveRect_SetPos(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS_EXTRA(2, "Int x, Int y");
    int32 x = vm->param(0)->forceInteger();
    int32 y = vm->param(1)->forceInteger();
    fal_ObjectCarrier *self = Falcon::dyncast<fal_ObjectCarrier*>( vm->self().asObject() );
    ActiveRect *obj = (ActiveRect*)self->GetObj();
    obj->SetPos(x,y);
}

FALCON_FUNC fal_ActiveRect_SetCollisionEnabled(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS(1);
    fal_ObjectCarrier *self = Falcon::dyncast<fal_ObjectCarrier*>( vm->self().asObject() );
    ActiveRect *obj = (ActiveRect*)self->GetObj();
    Falcon::Item *p = vm->param(0);
    if(p->isBoolean())
        obj->SetCollisionMask(p->isTrue() ? -1 : 0);
    else
       obj->SetCollisionMask((uint32)p->forceInteger());
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

FALCON_FUNC fal_ActiveRect_CastRay(Falcon::VMachine *vm)
{
    fal_ObjectCarrier *self = Falcon::dyncast<fal_ObjectCarrier*>( vm->self().asObject() );
    Falcon::Item *i_to    = vm->param(0); // target pos or direction
    Falcon::Item *i_which = vm->param(1); // return last (default) or collision vector
    Falcon::Item *i_lcf   = vm->param(2); // LCF
    if(!(i_to && i_to->isOfClass("Vector")))
        throw new Falcon::ParamError(Falcon::ErrorParam( Falcon::e_inv_params, __LINE__ ));

    LayerMgr *lm = Engine::GetInstance()->_GetLayerMgr();
    const bool collv = i_which && i_which->isTrue();
    const LayerCollisionFlag lcf = i_lcf ? LayerCollisionFlag(i_lcf->forceIntegerEx()) : LCF_ALL; // TODO: use object's own LCF
    const Vector2df dst = Falcon::dyncast<fal_Vector2d*>(i_to->asObjectSafe())->vec();
    Vector2df lastpos, collpos;

    if(lm->CastRaysFromRect(*Falcon::dyncast<ActiveRect*>(self->GetObj()), dst, lastpos, collpos, lcf))
    {
        fal_Vector2d *v = new fal_Vector2d(VECTOR_CLASS_SYMBOL);
        v->vec() = collv ? collpos : lastpos;
        vm->retval(v);
    }
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

FALCON_FUNC fal_Object_GetSpeed(Falcon::VMachine *vm)
{
    fal_ObjectCarrier *self = Falcon::dyncast<fal_ObjectCarrier*>( vm->self().asObject() );
    Object *obj = (Object*)self->GetObj();
    fal_Vector2d *v = new fal_Vector2d(VECTOR_CLASS_SYMBOL);
    Falcon::Item *p0 = vm->param(0);
    if(!p0)
        v->vec() = obj->GetTotalSpeed();
    else
    {
        size_t idx = p0->forceIntegerEx();
        if(idx < obj->phys.size())
            v->vec() = obj->phys[idx].speed;
    }
    vm->retval(v);
}

FALCON_FUNC fal_Object_SetSpeed(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS(2);
    fal_ObjectCarrier *self = Falcon::dyncast<fal_ObjectCarrier*>( vm->self().asObject() );
    Object *obj = (Object*)self->GetObj();
    size_t idx = vm->param(0)->forceIntegerEx();
    Falcon::Item *p1 = vm->param(1);
    if(p1->isOfClass("Vector"))
        obj->phys.get(idx).speed = Falcon::dyncast<fal_Vector2d*>(p1->asObject())->vec(); // auto-extend
    else
    {
        Falcon::Item *p2 = vm->param(2);
        Vector2df& spd = obj->phys.get(idx).speed; // auto-extend
        if(!p1->isNil())
            spd.x = (float)p1->forceNumeric();
        if(!p2->isNil())
            spd.y = (float)p2->forceNumeric();
    }
}

FALCON_FUNC fal_Object_AddSpeed(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS(2);
    fal_ObjectCarrier *self = Falcon::dyncast<fal_ObjectCarrier*>( vm->self().asObject() );
    Object *obj = (Object*)self->GetObj();
    size_t idx = vm->param(0)->forceIntegerEx();
    Falcon::Item *p1 = vm->param(1);
    if(p1->isOfClass("Vector"))
        obj->phys.get(idx).speed += Falcon::dyncast<fal_Vector2d*>(p1->asObject())->vec(); // auto-extend
    else
    {
        Falcon::Item *p2 = vm->param(2);
        Vector2df& spd = obj->phys.get(idx).speed; // auto-extend
        if(!p1->isNil())
            spd.x += (float)p1->forceNumeric();
        if(!p2->isNil())
            spd.y += (float)p2->forceNumeric();
    }
}

FALCON_FUNC fal_Object_GetAccel(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS(1);
    fal_ObjectCarrier *self = Falcon::dyncast<fal_ObjectCarrier*>( vm->self().asObject() );
    Object *obj = (Object*)self->GetObj();
    fal_Vector2d *v = new fal_Vector2d(VECTOR_CLASS_SYMBOL);
    size_t idx = vm->param(0)->forceIntegerEx();
    if(idx < obj->phys.size())
        v->vec() = obj->phys[idx].accel;
    vm->retval(v);
}

FALCON_FUNC fal_Object_SetAccel(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS(2);
    fal_ObjectCarrier *self = Falcon::dyncast<fal_ObjectCarrier*>( vm->self().asObject() );
    Object *obj = (Object*)self->GetObj();
    size_t idx = vm->param(0)->forceIntegerEx();
    Falcon::Item *p1 = vm->param(1);
    if(p1->isOfClass("Vector"))
        obj->phys.get(idx).accel = Falcon::dyncast<fal_Vector2d*>(p1->asObject())->vec(); // auto-extend
    else
    {
        Falcon::Item *p2 = vm->param(2);
        Vector2df& acl = obj->phys.get(idx).accel; // auto-extend
        if(!p1->isNil())
            acl.x = (float)p1->forceNumeric();
        if(!p2->isNil())
            acl.y = (float)p2->forceNumeric();
    }
}

FALCON_FUNC fal_Object_GetFrict(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS(1);
    fal_ObjectCarrier *self = Falcon::dyncast<fal_ObjectCarrier*>( vm->self().asObject() );
    Object *obj = (Object*)self->GetObj();
    fal_Vector2d *v = new fal_Vector2d(VECTOR_CLASS_SYMBOL);
    size_t idx = vm->param(0)->forceIntegerEx();
    if(idx < obj->phys.size())
        v->vec() = obj->phys[idx].friction;
    vm->retval(v);
}

FALCON_FUNC fal_Object_SetFrict(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS(2);
    fal_ObjectCarrier *self = Falcon::dyncast<fal_ObjectCarrier*>( vm->self().asObject() );
    Object *obj = (Object*)self->GetObj();
    size_t idx = vm->param(0)->forceIntegerEx();
    Falcon::Item *p1 = vm->param(1);
    if(p1->isOfClass("Vector"))
        obj->phys.get(idx).friction = Falcon::dyncast<fal_Vector2d*>(p1->asObject())->vec(); // auto-extend
    else
    {
        Falcon::Item *p2 = vm->param(2);
        Vector2df& fr = obj->phys.get(idx).friction; // auto-extend
        if(!p1->isNil())
            fr.x = (float)p1->forceNumeric();
        if(!p2->isNil())
            fr.y = (float)p2->forceNumeric();
    }
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

FALCON_FUNC fal_Object_IsBlocking(Falcon::VMachine *vm)
{
    fal_ObjectCarrier *self = Falcon::dyncast<fal_ObjectCarrier*>( vm->self().asObject() );
    vm->retval(((Object*)self->GetObj())->IsBlocking());
}

FALCON_FUNC fal_Object_SetBlocking(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS_EXTRA(1,"B");
    bool block = vm->param(0)->isTrue();
    fal_ObjectCarrier *self = Falcon::dyncast<fal_ObjectCarrier*>( vm->self().asObject() );
    ((Object*)self->GetObj())->SetBlocking(block);
}


FALCON_FUNC fal_TileLayer_SetVisible(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS(1);
    fal_TileLayer *self = Falcon::dyncast<fal_TileLayer*>( vm->self().asObject() );
    self->GetLayer()->visible = (bool)vm->param(0)->isTrue();
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

// TODO: deprecate?
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

FALCON_FUNC fal_TileLayer_SetParallaxMulti(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS_EXTRA(1, "N");
    fal_TileLayer *self = Falcon::dyncast<fal_TileLayer*>( vm->self().asObject() );
    self->GetLayer()->parallaxMulti = vm->param(0)->forceNumeric();
}

FALCON_FUNC fal_TileLayer_GetParallaxMulti(Falcon::VMachine *vm)
{
    fal_TileLayer *self = Falcon::dyncast<fal_TileLayer*>( vm->self().asObject() );
    vm->retval(Falcon::numeric(self->GetLayer()->parallaxMulti));
}

FALCON_FUNC fal_Objects_GetLastId(Falcon::VMachine *vm)
{
    vm->retval((Falcon::int64)Engine::GetInstance()->objmgr->GetLastId());
}

FALCON_FUNC fal_Objects_GetCount(Falcon::VMachine *vm)
{
    vm->retval((Falcon::int64)Engine::GetInstance()->objmgr->GetCount());
}

FALCON_FUNC fal_Objects_Get(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS_EXTRA(1, "N");
    BaseObject *obj = Engine::GetInstance()->objmgr->Get(vm->param(0)->forceIntegerEx());
    if(!obj)
    {
        vm->retnil();
        return;
    }
    DEBUG(ASSERT(obj->_falObj && obj->_falObj->coreCls));
    fal_ObjectCarrier *co = obj->_falObj->self();
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
        rect = *arect; // clone
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
    Engine::GetInstance()->objmgr->GetAllObjectsIn(rect, li);
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
        fal_ObjectCarrier *co = obj->_falObj->self();
        arr->append(co);
    }
    vm->retval(arr);
}

FALCON_FUNC fal_Objects_GetAll(Falcon::VMachine *vm)
{
    const ObjectMap& m = Engine::GetInstance()->objmgr->GetAllObjects();
    Falcon::CoreArray *arr = new Falcon::CoreArray(m.size());
    for(ObjectMap::const_iterator it = m.begin(); it != m.end(); ++it)
    {
        BaseObject *obj = it->second;
        DEBUG(ASSERT(obj->_falObj && obj->_falObj->coreCls));
        fal_ObjectCarrier *co = obj->_falObj->self();
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
    m->addClassMethod(clsObjects, "GetAll", fal_Objects_GetAll);
    m->addClassMethod(clsObjects, "Get", fal_Objects_Get);
    m->addClassMethod(clsObjects, "GetLastId", fal_Objects_GetLastId);
    m->addClassMethod(clsObjects, "GetCount", fal_Objects_GetCount);

    Falcon::Symbol *clsTileLayer = m->addClass("TileLayer", &forbidden_init);
    clsTileLayer->setWKS(true);
    m->addClassMethod(clsTileLayer, "IsVisible", &fal_TileLayer_IsVisible);
    m->addClassMethod(clsTileLayer, "SetVisible", &fal_TileLayer_SetVisible);
    m->addClassMethod(clsTileLayer, "SetTile", &fal_TileLayer_SetTile);
    m->addClassMethod(clsTileLayer, "GetTile", &fal_TileLayer_GetTile);
    m->addClassMethod(clsTileLayer, "GetArraySize", &fal_TileLayer_GetArraySize); // TODO: deprecate?
    m->addClassMethod(clsTileLayer, "SetCollisionEnabled", &fal_TileLayer_SetCollisionEnabled);
    m->addClassMethod(clsTileLayer, "IstCollisionEnabled", &fal_TileLayer_IsCollisionEnabled);
    m->addClassMethod(clsTileLayer, "GetParallaxMulti", &fal_TileLayer_GetParallaxMulti);
    m->addClassMethod(clsTileLayer, "SetParallaxMulti", &fal_TileLayer_SetParallaxMulti);
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
    m->addClassMethod(clsBaseObject, "compare", fal_BaseObject_compare); // override falcon comparison operator
    m->addConstant("OBJTYPE_RECT", (Falcon::int64)OBJTYPE_RECT, true);
    m->addConstant("OBJTYPE_OBJECT", (Falcon::int64)OBJTYPE_OBJECT, true);
    m->addConstant("OBJTYPE_UNIT", (Falcon::int64)OBJTYPE_UNIT, true);
    m->addConstant("OBJTYPE_PLAYER", (Falcon::int64)OBJTYPE_PLAYER, true);

    Falcon::Symbol *clsActiveRect = m->addClass("ActiveRect", &fal_ObjectCarrier::init);
    clsActiveRect->getClassDef()->addInheritance(inhBaseObject);
    Falcon::InheritDef *inhRect = new Falcon::InheritDef(clsActiveRect); // there are other classes that inherit from ActiveRect
    clsActiveRect->setWKS(true);

    m->addClassMethod(clsActiveRect, "OnUpdate", fal_NullFunc);
    m->addClassMethod(clsActiveRect, "OnEnter", fal_NullFunc);
    m->addClassMethod(clsActiveRect, "OnLeave", fal_NullFunc);
    m->addClassMethod(clsActiveRect, "OnTouch", fal_NullFunc);
    m->addClassMethod(clsActiveRect, "OnEnteredBy", fal_NullFunc);
    m->addClassMethod(clsActiveRect, "OnLeftBy", fal_NullFunc); // TODO: NYI
    m->addClassMethod(clsActiveRect, "OnTouchedBy", fal_NullFunc);
    m->addClassMethod(clsActiveRect, "SetBBox", fal_ActiveRect_SetBBox);
    m->addClassMethod(clsActiveRect, "SetPos", fal_ActiveRect_SetPos);
    m->addClassMethod(clsActiveRect, "IsCollisionEnabled", fal_ActiveRect_IsCollisionEnabled); // also see .collision property
    m->addClassMethod(clsActiveRect, "SetCollisionEnabled", fal_ActiveRect_SetCollisionEnabled);
    m->addClassMethod(clsActiveRect, "GetDistance", fal_ActiveRect_GetDistance);
    m->addClassMethod(clsActiveRect, "GetDistanceX", fal_ActiveRect_GetDistanceX);
    m->addClassMethod(clsActiveRect, "GetDistanceY", fal_ActiveRect_GetDistanceY);
    m->addClassMethod(clsActiveRect, "SetBlocking", fal_NullFunc); // ActiveRect is never blocking // also see .blocking property
    m->addClassMethod(clsActiveRect, "IsBlocking", fal_FalseFunc);
    m->addClassMethod(clsActiveRect, "CastRay", fal_ActiveRect_CastRay);
    m->addClassProperty(clsActiveRect, "x");
    m->addClassProperty(clsActiveRect, "y");
    m->addClassProperty(clsActiveRect, "w");
    m->addClassProperty(clsActiveRect, "h");
    m->addClassProperty(clsActiveRect, "x2");
    m->addClassProperty(clsActiveRect, "y2");
    m->addClassProperty(clsActiveRect, "x2f");
    m->addClassProperty(clsActiveRect, "y2f");
    m->addClassProperty(clsActiveRect, "pos");
    m->addClassProperty(clsActiveRect, "size");
    // TODO: pos/size properties - return vector

    Falcon::Symbol *clsObject = m->addClass("Object", &fal_ObjectCarrier::init);
    clsObject->getClassDef()->addInheritance(inhRect);
    Falcon::InheritDef *inhObject = new Falcon::InheritDef(clsObject); // there are other classes that inherit from Object
    clsObject->setWKS(true);
    m->addClassMethod(clsObject, "OnTouchWall", fal_NullFunc);
    m->addClassMethod(clsObject, "SetSprite", fal_Object_SetSprite); // also see .sprite property
    m->addClassMethod(clsObject, "GetSprite", fal_Object_GetSprite);
    m->addClassMethod(clsObject, "SetLayerId", fal_Object_SetLayerId); // also see .layerId property
    m->addClassMethod(clsObject, "GetLayerId", fal_Object_GetLayerId);
    m->addClassMethod(clsObject, "SetAffectedByPhysics", &fal_Object_SetAffectedByPhysics); // also see .physics property
    m->addClassMethod(clsObject, "IsAffectedByPhysics", &fal_Object_IsAffectedByPhysics);
    m->addClassMethod(clsActiveRect, "SetBlocking", fal_Object_SetBlocking); // also see .blocking property
    m->addClassMethod(clsActiveRect, "IsBlocking", fal_Object_IsBlocking);
    m->addClassMethod(clsActiveRect, "SetSpeed", fal_Object_SetSpeed);
    m->addClassMethod(clsActiveRect, "GetSpeed", fal_Object_GetSpeed);
    m->addClassMethod(clsActiveRect, "AddSpeed", fal_Object_AddSpeed);
    m->addClassMethod(clsActiveRect, "SetAccel", fal_Object_SetAccel);
    m->addClassMethod(clsActiveRect, "GetAccel", fal_Object_GetAccel);
    m->addClassMethod(clsActiveRect, "SetFrict", fal_Object_SetFrict);
    m->addClassMethod(clsActiveRect, "GetFrict", fal_Object_GetFrict);
    m->addClassProperty(clsObject, "mass");
    m->addClassProperty(clsObject, "gfxOffsX"); // TODO: deprecate
    m->addClassProperty(clsObject, "gfxOffsY");


    Falcon::Symbol *clsUnit = m->addClass("Unit", &fal_ObjectCarrier::init);
    Falcon::InheritDef *inhUnit = new Falcon::InheritDef(clsUnit);
    clsUnit->getClassDef()->addInheritance(inhObject);
    clsUnit->setWKS(true);

    Falcon::Symbol *clsPlayer = m->addClass("Player", &fal_ObjectCarrier::init);
    clsPlayer->getClassDef()->addInheritance(inhUnit);
    clsPlayer->setWKS(true);

    /*Falcon::Symbol *clsVectorAdapter = m->addClass("VectorAdapter");
    clsVectorAdapter->setWKS(true);
    m->addClassMethod(clsVectorAdapter, OVERRIDE_OP_GETINDEX, &fal_VectorAdapter_GetIndex);
    m->addClassMethod(clsVectorAdapter, OVERRIDE_OP_SETINDEX, &fal_VectorAdapter_SetIndex);
    m->addClassMethod(clsVectorAdapter, "len", &fal_VectorAdapter_len);*/


    return m;
}
