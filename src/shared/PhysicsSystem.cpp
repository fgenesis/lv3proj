#include "common.h"
#include "SharedDefines.h"
#include "Objects.h"
#include "LayerMgr.h"
#include "PhysicsSystem.h"


void PhysicsMgr::UpdatePhysics(Object *obj, uint32 ms)
{
    // affected by physics already checked in ObjectMgr::Update

    DEBUG(ASSERT(obj->GetType() >= OBJTYPE_OBJECT));

    PhysProps& phys = obj->phys;
    float tf =  ms * 0.001f; // time factor
    bool canfall = obj->CanFallDown();
    bool neg;

    // set y acceleration to gravity if we can fall down and acceleration is lower then gravity
    if(canfall)
    {
        if(phys.weight && phys.yaccel < envPhys.gravity)
            phys.yaccel = envPhys.gravity;
    }
    else
    {
        if(phys.yaccel > 0.0f)
            phys.yaccel = 0.0f;
        if(phys.yspeed > 0.0f)
            phys.yspeed = 0.0f;
    }

    // apply acceleration to speed
    phys.xspeed += (phys.xaccel * tf);
    phys.yspeed += (phys.yaccel * tf);

    // apply friction, x speed
    if(phys.xfriction)
    {
        neg = fastsgncheck(phys.xspeed);
        phys.xspeed = fastabs(phys.xspeed) - (phys.xfriction * tf);
        if(phys.xspeed < 0.0f)
            phys.xspeed = 0.0f;
        else if(neg)
            phys.xspeed = fastneg(phys.xspeed);
    }

    // apply friction, y speed
    if(phys.yfriction)
    {
        neg = fastsgncheck(phys.yspeed);
        phys.yspeed = fastabs(phys.yspeed) - (phys.yfriction * tf);
        if(phys.yspeed < 0.0f)
            phys.yspeed = 0.0f;
        else if(neg)
            phys.yspeed = fastneg(phys.yspeed);
    }

    // limit speed if required
    if(phys.xmaxspeed >= 0.0f && phys.xspeed > phys.xmaxspeed)
        phys.xspeed = phys.xmaxspeed;
    if(phys.ymaxspeed >= 0.0f && phys.yspeed > phys.ymaxspeed)
        phys.yspeed = phys.ymaxspeed;

    if(phys.xspeed || phys.yspeed)
    {
        float oldx = obj->x, oldy = obj->y;
        // apply speed to current position
        obj->x += (phys.xspeed * tf);
        obj->y += (phys.yspeed * tf);

        // oops, newly selected position is somewhere inside a wall, find nearest valid spot
        if(_layerMgr->CollisionWith(obj))
        {
            uint8 direction = DIRECTION_NONE;

            if(obj->x < oldx)
                direction |= DIRECTION_LEFT;
            else if(obj->x > oldx)
                direction |= DIRECTION_RIGHT;

            if(obj->y < oldy)
                direction |= DIRECTION_UP;
            else if(obj->y > oldy)
                direction |= DIRECTION_DOWN;

            obj->x = oldx;
            obj->y = oldy;

            Point np = _layerMgr->GetClosestNonCollidingPoint(obj, direction);

            obj->x = float(np.x);
            obj->y = float(np.y);

            // unable to change position, set speed to 0
            if(direction & (DIRECTION_LEFT |DIRECTION_RIGHT) && np.x == oldx)
                phys.xspeed = 0.0f;
            if(direction & (DIRECTION_UP |DIRECTION_DOWN) && np.y == oldy)
                phys.yspeed = 0.0f;
        }

        obj->UpdateAnchor();
        obj->HasMoved();
    }
}
