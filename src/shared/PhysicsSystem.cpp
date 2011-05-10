#include "common.h"
#include "SharedDefines.h"
#include "Objects.h"
#include "LayerMgr.h"
#include "ObjectMgr.h"
#include "PhysicsSystem.h"

#include "UndefUselessCrap.h"

PhysicsMgr::PhysicsMgr()
: _layerMgr(NULL), _objMgr(NULL)
{
    SetDefaults();
}

void PhysicsMgr::SetDefaults(void)
{
    envPhys.gravity = 0.0f;
}

void PhysicsMgr::UpdatePhysics(Object *obj, float tf)
{
    // affected by physics already checked in ObjectMgr::Update

    DEBUG(ASSERT(obj->GetType() >= OBJTYPE_OBJECT));

    PhysProps& phys = obj->phys;
    Vector2df oldpos = obj->pos;

    _ApplySpeed(obj, tf);
    _ApplyAccel(obj, tf);
    _ApplyFriction(obj, tf);

    _DoWallCollision(obj);
    _DoObjectCollision(obj);
    

    if(oldpos != obj->pos)
    {
        obj->SetMoved(true);
    }
}

void PhysicsMgr::_ApplySpeed(Object *obj, float tf)
{
    for(uint32 i = 0; i < obj->phys.size(); ++i)
        obj->pos += (obj->phys.speed[i] * tf);
}

void PhysicsMgr::_ApplyAccel(Object *obj, float tf)
{
    for(uint32 i = 0; i < obj->phys.size(); ++i)
        obj->phys.speed += (obj->phys.accel[i] * tf);
}

void PhysicsMgr::_ApplyFriction(Object *obj, float tf)
{
    for(uint32 i = 0; i < obj->phys.size(); ++i)
        obj->phys.speed *= (obj->phys.friction[i] * tf);
}

void PhysicsMgr::_DoCollision(Object *obj, const Vector2df& oldpos)
{
    BaseRect combined
}
