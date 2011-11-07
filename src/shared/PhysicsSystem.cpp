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
    envPhys.gravity = Vector2df(0, 0);
}

void PhysicsMgr::UpdatePhysics(Object *obj, float tf)
{
    // affected by physics already checked in ObjectMgr::Update

    DEBUG(ASSERT(obj->GetType() >= OBJTYPE_OBJECT));
    DEBUG(ASSERT(tf > 0.0f && tf <= 1.0f));

    Vector2df oldpos = obj->pos;

    
    _ApplyAccel(obj, tf);
    _ApplyFriction(obj, tf);
    _ApplySpeedAndCollision(obj, tf);
    

    if(oldpos != obj->pos)
    {
        obj->SetMoved(true);
    }
}

void PhysicsMgr::_ApplyAccel(Object *obj, float tf)
{
    for(uint32 i = 0; i < obj->phys.size(); ++i)
        obj->phys[i].speed += (obj->phys[i].accel * tf);

    const float effectiveMass = clamp(obj->phys.mass, -1.0f, 1.0f);
    obj->phys[0].speed += (envPhys.gravity * tf * effectiveMass);
}

void PhysicsMgr::_ApplyFriction(Object *obj, float tf)
{
    const Vector2df u(1.0f, 1.0f);
    for(uint32 i = 0; i < obj->phys.size(); ++i)
    {
        Vector2df factor = u - (obj->phys[i].friction * tf);
        clamp(factor.x, 0.0f, 1.0f);
        clamp(factor.y, 0.0f, 1.0f);
        obj->phys[i].speed *= factor;
    }
}

void PhysicsMgr::_ApplySpeedAndCollision(Object *obj, float tf)
{
    // -- obj vs. wall collision --

cast_ray:
    
    Vector2df spd = obj->GetTotalSpeed() * tf;

    // if the object will never collide with wall, we are done after updating its speed
    /*if(obj->GetBlockedByLCF() == LCF_NONE)
    {
        obj->pos += spd;
        return;
    }*/


    Vector2df lastposWall, collposWall; // will store relative pixel amount of last accessible point
                                        // and first point that collides with wall.

    bool recast = false;
    bool hitwall = obj->CastRay(spd, lastposWall, collposWall, obj->GetBlockedByLCF());
    //DEBUG(ASSERT(!hitwall || spd.lensq() >= lastposWall.lensq()));



    // TODO: -- obj vs obj collision here --
    //BaseRect combined(*obj);
    //combined = combined.unionRect(BaseRect(obj->pos + spd, obj->size)); // TODO: use this for bbox enlargement at high speeds


    // combine everything and update position
    if(hitwall)
    {


        if(collposWall.x == lastposWall.x) // stuck? can't move then.
        {
            obj->phys[0].speed.x = /*spd.x =*/ 0;
            recast = true;
        }
        else if(exceeds(spd.x, lastposWall.x)) // limit speed to max. movement dir
        {
            //spd.x = lastposWall.x;
            obj->phys[0].speed.x = 0; // clear speed affected by gravity
            recast = true;
        }

        if(collposWall.y == lastposWall.y)
        {
            obj->phys[0].speed.y = /*spd.y =*/ 0;
            recast = true;
        }
        else if(exceeds(spd.y, lastposWall.y))
        {
            //spd.y = lastposWall.y;
            obj->phys[0].speed.y = 0;
            recast = true;
        }

        if(recast)
            goto cast_ray;

        //float l = spd.lensq();
        //spd.setLen(l);  //// <-------- FIXME: THIS IS WRONG !!!!!1 ##############

        obj->pos += spd;
        //obj->SetMoved(!spd.isZero());

        if(!spd.isZero())
            obj->OnTouchWall();

        // clear speed managed by gravity
        // TODO: what about other speeds pointing into the same direction as spd?
        //obj->phys[0].speed = Vector2df(0, 0);
    }
    else
    {
        obj->pos += spd;
    }
    

}
