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

inline static void _UpdatePositionBySpeed(Object *obj, const Vector2df& spd)
{
    obj->pos += spd;
}

void PhysicsMgr::_ApplySpeedAndCollision(Object *obj, float tf)
{
    // -- obj vs. wall collision --

    bool xblocked = false;
    bool yblocked = false;
    bool xhit = false;
    bool yhit = false;
    bool hadSpeed = false;

    // effective total speed
    Vector2df spdf = obj->GetTotalSpeed() * tf;

cast_ray:
    
    hadSpeed = hadSpeed || !spdf.isZero();

    if(xblocked)
    {
        spdf.x = 0;
    }
    if(yblocked)
    {
        spdf.y = 0;
    }

    const Vector2di speedOffset(sgn(spdf.x), sgn(spdf.y));
    Vector2di spdi (spdf.x, spdf.y);
    spdi += speedOffset;

    // if the object will never collide with wall, we are done after updating its speed
    /*if(obj->GetBlockedByLCF() == LCF_NONE)
    {
        obj->pos += spd;
        return;
    }*/


    Vector2di lastposWall, collposWall; // will store relative pixel amount of last accessible point
                                        // and first point that collides with wall.

    bool hitwall = obj->CastRay(spdi, lastposWall, collposWall, obj->GetBlockedByLCF());
    //DEBUG(ASSERT(!hitwall || spd.lensq() >= lastposWall.lensq()));



    // TODO: -- obj vs obj collision here --
    //BaseRect combined(*obj);
    //combined = combined.unionRect(BaseRect(obj->pos + spd, obj->size)); // TODO: use this for bbox enlargement at high speeds


    // combine everything and update position
    if(hitwall)
    {


        if(!xblocked)
        {
            if(collposWall.x == lastposWall.x) // stuck? can't move then.
            {
                xblocked = true;
                spdf.x = 0;
                goto cast_ray;
            }
            else if(exceeds(spdi.x, lastposWall.x)) // limit speed to max. movement dir
            {

                xhit = true;
                spdi.x = lastposWall.x;
                spdf.x = lastposWall.x;
            }
        }
        if(!yblocked)
        {
            if(collposWall.y == lastposWall.y)
            {
                yblocked = true;
                spdf.y = 0;
                goto cast_ray;
            }
            else if(exceeds(spdi.y, lastposWall.y))
            {
                yhit = true;
                spdi.y = lastposWall.y;
                spdf.y = lastposWall.y;
            }
        }

        //float l = spd.lensq();
        //spd.setLen(l);  //// <-------- FIXME: THIS IS WRONG !!!!!1 ##############

        _UpdatePositionBySpeed(obj, spdf);
        //obj->SetMoved(!spd.isZero());

        // FIXME: this must also be called if x and y speed are 0 (I don't think tht will always happen here!)
        if(hadSpeed)
            obj->OnTouchWall();

        // clear speed managed by gravity
        // -- done AFTER the collision callback, so that the object still knows its impact speed
        if(xhit || xblocked)
            obj->phys[0].speed.x = 0;
        if(yhit || yblocked)
            obj->phys[0].speed.y = 0;
    }
    else
    {
        obj->pos += spdf;
    }
    

}
