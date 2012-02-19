#include "common.h"
#include "SharedDefines.h"
#include "Objects.h"
#include "LayerMgr.h"
#include "ObjectMgr.h"
#include "PhysicsSystem.h"
#include "Engine.h"

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

void PhysicsMgr::UpdatePhysics(Object *obj, float dt)
{
    // affected by physics already checked in ObjectMgr::Update

    DEBUG(ASSERT(obj->GetType() >= OBJTYPE_OBJECT));
    DEBUG(ASSERT(dt > 0.0f && dt <= 1.0f));

    Vector2df oldpos = obj->pos;

    
    _ApplyAccel(obj, dt);
    _ApplyFriction(obj, dt);
    _ApplySpeedAndCollision(obj, dt);
    

    /*if(oldpos != obj->pos)
    {
        obj->SetMoved(true);
    }*/
}

void PhysicsMgr::_ApplyAccel(Object *obj, float dt)
{
    obj->phys.speed += (obj->phys.accel * dt);

    const float effectiveMass = clamp(obj->phys.mass, -1.0f, 1.0f);
    obj->phys.speed += (envPhys.gravity * dt * effectiveMass);
}

void PhysicsMgr::_ApplyFriction(Object *obj, float dt)
{
    // FIXME: no idea what this is supposed to do
    const Vector2df u(1.0f, 1.0f);
    Vector2df factor = u - (obj->phys.friction * dt);
    clamp(factor.x, 0.0f, 1.0f);
    clamp(factor.y, 0.0f, 1.0f);
    obj->phys.speed *= factor;
}

inline static void _UpdatePositionBySpeed(Object *obj, const Vector2df& spd)
{
    if(!spd.isZero())
    {
        obj->pos += spd;
        obj->SetMoved(true);
    }
    // TODO: anything else?
}

// scales the speed in such a way that adding it to an object
// will align it perfectly into the last possible spot without
// overlapping with the collision mask.
// The code is probably inefficient, but is used to adjust the very last pixels only.
// TODO: Speed this up a bit, anyway
static Vector2df _AdjustSpeed(Object *obj, const Vector2df& spd)
{
    if(spd.isZero())
        return spd;

    float factor = 0;
    Vector2df norm = spd;
    norm.normalize();

    // one of both components should be exactly 1 (positive or negative)
    norm.x = abs(norm.x);
    norm.y = abs(norm.y);

    if(norm.x < norm.y)
        factor = 1.0f / norm.y;
    else
        factor = 1.0f / norm.x;

    norm = spd;
    norm.normalize();
    norm *= factor;

    // now at least one component is exactly 1, and the other <= 1.
    DEBUG(ASSERT(norm.x <= 1.0f));
    DEBUG(ASSERT(norm.y <= 1.0f));

    // start moving the object until it hits the wall.
    Vector2df ret;
    BaseRect rect(*obj);

    while(true) // FIXME
    {
        rect.pos += norm;
        if(Engine::GetInstance()->_GetLayerMgr()->CollisionWith(&rect, 1, LCF_ALL))
            return ret;

        // HACK: wtf?
        if(ret.len() > spd.len()) // we exceed the len, something is wrong
            return ret;
        ret += norm;
    }

    NOT_REACHED_LINE;
    return Vector2df();
}



void PhysicsMgr::_ApplySpeedAndCollision(Object *obj, float dt)
{
    // -- obj vs. wall collision --

    bool xblocked = false;
    bool yblocked = false;
    bool xhit = false;
    bool yhit = false;
    bool hadSpeed = false;

    LayerCollisionFlag lcf = obj->GetBlockedByLCF();

    // effective total speed
    Vector2df spdf = obj->GetSpeed() * dt;
    //float spdlen = spdf.len();

cast_ray:
    
    hadSpeed = hadSpeed || !spdf.isZero();

    if(xblocked)
    {
        spdf.x = 0;
        xblocked = false;
        xhit = false;
    }
    if(yblocked)
    {
        spdf.y = 0;
        yblocked = false;
        yhit = false;
    }

    // no speed? nothing else to do here.
    if(spdf.isZero())
        return;

    // round spdi up or down, because spdf's mantissa is lost.
    // integer conversion required because bresenham linecasting is int-only.
    //const Vector2di speedOffset(sgn(spdf.x), sgn(spdf.y));
    //Vector2di spdi (spdf.x, spdf.y);
    //spdi += speedOffset;

    const Vector2di spdi(fceili(spdf.x), fceili(spdf.y));

    //Vector2di spdi (spdf.x + 1, spdf.y + 1);

    // if the object will never collide with wall, we are done after updating its speed
    /*if(obj->GetBlockedByLCF() == LCF_NONE)
    {
        obj->pos += spd;
        return;
    }*/


    Vector2di lastposWall, collposWall; // will store relative pixel amount of last accessible point
                                        // and first point that collides with wall.

    bool hitwall = obj->CastRay(spdi, &lastposWall, &collposWall, lcf);
    //DEBUG(ASSERT(!hitwall || spd.lensq() >= lastposWall.lensq()));



    // TODO: -- obj vs obj collision here --
    //BaseRect combined(*obj);
    //combined = combined.unionRect(BaseRect(obj->pos + spd, obj->size)); // TODO: use this for bbox enlargement at high speeds


    // combine everything and update position
    if(hitwall)
    {
        xblocked = spdf.x && obj->CastRay(Vector2di(sgn(spdf.x), 0), NULL, NULL, lcf);
        yblocked = spdf.y && obj->CastRay(Vector2di(0, sgn(spdf.y)), NULL, NULL, lcf);


        if(!xblocked)
        {
            /*if(collposWall.x == lastposWall.x) // stuck? can't move then.
            {
                xblocked = true;
                spdf.x = 0;
                //goto cast_ray;
            }
            else*/ if(exceeds(spdi.x, lastposWall.x)) // limit speed to max. movement dir
            {
                xhit = true;
                //spdi.x = lastposWall.x;
                //spdf.x = lastposWall.x;
            }
        }
        if(!yblocked)
        {
            /*if(collposWall.y == lastposWall.y)
            {
                yblocked = true;
                spdf.y = 0;
                //goto cast_ray;
            }
            else*/ if(exceeds(spdi.y, lastposWall.y))
            {
                yhit = true;
                //spdi.y = lastposWall.y;
                //spdf.y = lastposWall.y;
            }
        }

        Vector2df useSpeed = spdf;
        if(xhit || yhit || xblocked || yblocked)
            useSpeed = _AdjustSpeed(obj, spdf);

        _UpdatePositionBySpeed(obj, useSpeed);

        bool keepSpeed = false;
        if(hadSpeed)
            keepSpeed = obj->OnTouchWall();

        if(!keepSpeed)
        {
            // clear speed managed by gravity
            // -- done AFTER the collision callback, so that the object still knows its impact speed
            if(xblocked)
                obj->phys.speed.x = 0;
            if(yblocked)
                obj->phys.speed.y = 0;
        }

        // if there is still momentum left, handle the rest of the speed
        //if(!obj->phys.speed.isZero())
        {
            float len = spdf.len();
            len -= useSpeed.len();
            if(len <= 0)
                return;
            spdf.setLen(len);

            //printf("[%p] momentum: x: %f, y: %f\n", obj, spdf.x, spdf.y);
            goto cast_ray;
        }

    }
    else
    {
        _UpdatePositionBySpeed(obj, spdf);
    }
    

}
