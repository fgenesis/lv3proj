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
    uint8 speedsSet = 0; // can be 0, 1 or 2
    bool neg;

    float yaccelTotal = envPhys.gravity + phys.yaccel;

    // apply acceleration to speed
    phys.xspeed += (phys.xaccel * tf);
    phys.yspeed += (yaccelTotal * tf);

    
    if(phys.xspeed)
    {
        ++speedsSet;
        neg = fastsgncheck(phys.xspeed);

        // limit y speed if required
        if(phys.xmaxspeed >= 0.0f && abs(phys.xspeed) > phys.xmaxspeed)
            phys.xspeed = neg ? -phys.xmaxspeed : phys.xmaxspeed;

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
    }


    if(phys.yspeed)
    {
        ++speedsSet;
        neg = fastsgncheck(phys.yspeed);

        // limit y speed if required
        if(phys.ymaxspeed >= 0.0f && abs(phys.yspeed) > phys.ymaxspeed)
            phys.yspeed = neg ? -phys.ymaxspeed : phys.ymaxspeed;

        // apply friction, y speed
        if(phys.yfriction)
        {
            phys.yspeed = fastabs(phys.yspeed) - (phys.yfriction * tf);
            if(phys.yspeed < 0.0f)
                phys.yspeed = 0.0f;
            else if(neg)
                phys.yspeed = fastneg(phys.yspeed);
        }
    }

    // we are not moving, nothing else to do here.
    if(!speedsSet)
    {
        return;
    }

    // get new positions for current speed and time diff
    float newx = obj->x + (phys.xspeed * tf);
    float newy = obj->y + (phys.yspeed * tf);

    uint8 dirx = DIRECTION_NONE, diry = DIRECTION_NONE;


    // check if obj can move in x direction
    if(phys.xspeed)
    {
        if(newx < obj->x)
        {
            dirx |= DIRECTION_LEFT;
            if(!_layerMgr->CanMoveToDirection(obj, DIRECTION_LEFT))
            {
                phys.xspeed = 0.0f;
            }
        }
        else// if(newx > obj->x)
        {
            dirx |= DIRECTION_RIGHT;
            if(!_layerMgr->CanMoveToDirection(obj, DIRECTION_RIGHT))
            {
                phys.xspeed = 0.0f;
            }
        }
    }

    // check if obj can move in y direction
    if(phys.yspeed)
    {
        if(newy < obj->y)
        {
            diry |= DIRECTION_UP;
            if(!_layerMgr->CanMoveToDirection(obj, DIRECTION_UP))
            {
                phys.yspeed = 0.0f;
            }
        }
        else// if(newy > obj->y)
        {
            diry |= DIRECTION_DOWN;
            if(!_layerMgr->CanMoveToDirection(obj, DIRECTION_DOWN))
            {
                phys.yspeed = 0.0f;
            }
        }
    }

    uint8 direction = dirx | diry; // combined directions
    float oldx = obj->x;
    float oldy = obj->y;
    if(phys.xspeed && phys.yspeed) // cam move diagonally? have to recheck if thats the case
    {
        if(_layerMgr->CanMoveToDirection(obj, direction))
        {
            // obj can move, do it.
            obj->x = newx;
            obj->y = newy;
        }
        else
        {
            // moving into both directions failed, prefer moving up or down only.
            // the directions stay as they are, otherwise GetClosestNonCollidingPoint() would do mess.
            obj->y = newy;
        }
    }
    else // just moving in one direction (the check for this was already done above)
    {
        if(phys.xspeed)
            obj->x = newx;
        if(phys.yspeed)
            obj->y = newy;
    }

    // oops, newly selected position is somewhere inside a wall, find nearest valid spot
    if(direction && _layerMgr->CollisionWith(obj))
    {
        obj->x = oldx;
        obj->y = oldy;

        Point np = _layerMgr->GetClosestNonCollidingPoint(obj, direction);

        obj->x = float(np.x);
        obj->y = float(np.y);

        // now check where we can move from this position
        if(dirx && !_layerMgr->CanMoveToDirection(obj, dirx))
        {
            phys.xspeed = 0;
        }
        if(diry && !_layerMgr->CanMoveToDirection(obj, diry))
        {
            phys.yspeed = 0;
        }
    }

    obj->UpdateAnchor();
    obj->HasMoved();
}
