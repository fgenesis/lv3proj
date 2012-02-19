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
    obj->phys.speed += (envPhys.gravity * (dt * obj->phys.gravityMult));
}

void PhysicsMgr::_ApplyFriction(Object *obj, float dt)
{
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
    const float spdLen = spd.len();
    const float normLen = norm.len();
    const LayerCollisionFlag lcf = obj->GetBlockedByLCF();
    Vector2df ret;
    BaseRect rect(*obj);
    float retLen = 0;

    while(true)
    {
        rect.pos += norm;
        if(Engine::GetInstance()->_GetLayerMgr()->CollisionWith(&rect, 1, LCF_ALL))
            return ret;

        // we exceed the len, something is wrong. Weren't supposed to move further anyways, so just return.
        if(retLen > spdLen)
            return ret;

        ret += norm;
        retLen += normLen;
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

    const Vector2di spdi(fceili(spdf.x), fceili(spdf.y));


    Vector2di lastposWall, collposWall; // will store relative pixel amount of last accessible point
                                        // and first point that collides with wall.

    bool hitwall = obj->CastRay(spdi, &lastposWall, &collposWall, lcf);



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
            if(exceeds(spdi.x, lastposWall.x)) // limit speed to max. movement dir
            {
                xhit = true;
            }
        }
        if(!yblocked)
        {
            if(exceeds(spdi.y, lastposWall.y))
            {
                yhit = true;
            }
        }

        Vector2df useSpeed = spdf;
        if(xhit || yhit || xblocked || yblocked)
            useSpeed = _AdjustSpeed(obj, spdf);

        _UpdatePositionBySpeed(obj, useSpeed);

        bool keepSpeed = false;
        if(hadSpeed)
            keepSpeed = obj->OnTouchWall(); // returns true to indicate it handled the collision in its own way.

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
        float len = spdf.len();
        len -= useSpeed.len();
        if(len <= 0)
            return;
        spdf.setLen(len);

        goto cast_ray;

    }
    else
    {
        _UpdatePositionBySpeed(obj, spdf);
    }
}
