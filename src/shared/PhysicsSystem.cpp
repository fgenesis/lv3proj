#include "common.h"
#include "SharedDefines.h"
#include "Objects.h"
#include "LayerMgr.h"
#include "ObjectMgr.h"
#include "PhysicsSystem.h"

#include "UndefUselessCrap.h"


void PhysicsMgr::UpdatePhysics(Object *obj, uint32 ms)
{
    // affected by physics already checked in ObjectMgr::Update

    DEBUG(ASSERT(obj->GetType() >= OBJTYPE_OBJECT));

    PhysProps& phys = obj->phys;
    float tf =  ms * 0.001f; // time factor
    uint8 speedsSet = 0; // can be 0, 1 or 2
    bool neg;
    int32 begin_ix = int32(obj->x);
    int32 begin_iy = int32(obj->y);
    float begin_x = obj->x;
    float begin_y = obj->y;
    ObjectWithSideSet solidCollidedObjs;
    bool selectNearby = obj->IsCollisionEnabled() && (obj->GetType() >= OBJTYPE_PLAYER || obj->IsBlocking());


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
    BaseRect newRect = obj->cloneRect();
    newRect.x = obj->x + (phys.xspeed * tf);
    newRect.y = obj->y + (phys.yspeed * tf);

    uint8 dirx = DIRECTION_NONE, diry = DIRECTION_NONE;


    // check if obj can move in x direction
    if(phys.xspeed)
    {
        if(newRect.x < obj->x)
        {
            dirx |= DIRECTION_LEFT;
            if(!_layerMgr->CanMoveToDirection(obj, DIRECTION_LEFT))
            {
                // we can not move in that direction, check if there is an object we would have pushed otherwise
                if(selectNearby)
                {
                    BaseRect area;
                    area.x = newRect.x;
                    area.y = obj->y;
                    area.w = 1;
                    area.h = obj->h - 1;
                    _objMgr->GetAllObjectsIn(area, solidCollidedObjs, SIDE_LEFT);
                }
            }
        }
        else if(newRect.x > obj->x)
        {
            dirx |= DIRECTION_RIGHT;
            if(!_layerMgr->CanMoveToDirection(obj, DIRECTION_RIGHT))
            {
                // we can not move in that direction, check if there is an object we would have pushed otherwise
                if(selectNearby)
                {
                    BaseRect area;
                    area.x = newRect.x + obj->w;
                    area.y = obj->y;
                    area.w = 1;
                    area.h = obj->h - 1;
                    _objMgr->GetAllObjectsIn(area, solidCollidedObjs, SIDE_RIGHT);
                }
            }
        }
    }

    // check if obj can move in y direction
    if(phys.yspeed)
    {
        if(newRect.y < obj->y)
        {
            diry |= DIRECTION_UP;
            if(!_layerMgr->CanMoveToDirection(obj, DIRECTION_UP))
            {
                // we can not move in that direction, check if there is an object we would have pushed otherwise
                if(selectNearby)
                {
                    BaseRect area;
                    area.x = obj->x;
                    area.y = newRect.y;
                    area.w = obj->w;
                    area.h = 1;
                    _objMgr->GetAllObjectsIn(area, solidCollidedObjs, SIDE_TOP);
                }
            }
        }
        else if(newRect.y > obj->y)
        {
            diry |= DIRECTION_DOWN;
            if(!_layerMgr->CanMoveToDirection(obj, DIRECTION_DOWN))
            {
                // we can not move in that direction, check if there is an object we would have pushed otherwise
                if(selectNearby)
                {
                    BaseRect area;
                    area.x = obj->x;
                    area.y = newRect.y + obj->h;
                    area.w = obj->w - 1;
                    area.h = 1;
                    _objMgr->GetAllObjectsIn(area, solidCollidedObjs, SIDE_BOTTOM);
                }
            }
        }
    }

    if(solidCollidedObjs.size())
    {
        for(ObjectWithSideSet::iterator it = solidCollidedObjs.begin(); it != solidCollidedObjs.end(); it++)
        {
            if(obj->GetType() >= OBJTYPE_OBJECT)
            {
                Object *target = (Object*)it->first;
                if(target == obj || !obj->IsCollisionEnabled())
                    continue;
                if(target->IsBlocking() && target->IsCollisionEnabled())
                {
                    obj->OnTouch(it->second | SIDE_FLAG_SOLID, target);
                    target->OnTouchedBy(InvertSide(it->second) | SIDE_FLAG_SOLID, obj); // TODO: return value?
                }
            }
        }
    }

    if(uint8 direction = dirx | diry) // combined directions
    {
        if(!_layerMgr->CollisionWith(&newRect))
        {
            obj->x = newRect.x;
            obj->y = newRect.y;
        }
        else // oops, newly selected position is somewhere inside a wall, find nearest valid spot
        {

            // -- start of messy part --
            // TODO: this part is a draft and not optimized. clean this up and OPTIMIZE!!
            Point pud, plr, pdia, pcur((uint32)obj->x, (uint32)obj->y);
            
            float distx = abs(newRect.x - obj->x);
            float disty = abs(newRect.y - obj->y);
            uint8 actualDir = DIRECTION_NONE;
            bool definiteWallCollision = false;

            if(dirx && diry)
                pdia = _layerMgr->GetNonCollidingPoint(obj, direction, 1 + (uint32)std::max(distx, disty));
            else
                pdia.invalidate();

            if(dirx && _layerMgr->CanMoveToDirection(obj, dirx))
                plr = _layerMgr->GetNonCollidingPoint(obj, dirx, 1 + (uint32) distx);
            else
                plr.invalidate();

            if(diry && _layerMgr->CanMoveToDirection(obj, diry))
                pud = _layerMgr->GetNonCollidingPoint(obj, diry, 1 + (uint32) disty);
            else
                pud.invalidate();

            if(pdia.valid() && pdia != pcur)
            {
                obj->x = float(pdia.x);
                obj->y = float(pdia.y);
                actualDir = dirx | diry;
            }
            else
            {
                if(pud.valid())
                {
                    float oldyy = obj->y;
                    obj->y = newRect.y;
                    // this is a hack and seems necessary, because otherwise the gravity makes things
                    // slide through walls and whatnot. but we have to force this new position,
                    // because if we just ignore this, objects will float 1 pixel above the wall.
                    if(_layerMgr->CollisionWith(obj))
                    {
                        obj->y = pud.y; 
                        definiteWallCollision = true;
                    }
                    actualDir = diry;
                }
                else if(plr.valid())
                {
                    obj->x = newRect.x;
                    actualDir = dirx;
                }
            }

            if(!actualDir)
            {
                if(!phys._wallTouched)
                {
                    if(obj->x < phys._lastx)
                        actualDir |= DIRECTION_LEFT;
                    else if(obj->x > phys._lastx)
                        actualDir |= DIRECTION_RIGHT;
                    if(obj->y < phys._lasty)
                        actualDir |= DIRECTION_UP;
                    else if(obj->y > phys._lasty)
                        actualDir |= DIRECTION_DOWN;
                }
                phys._wallTouched = false;
            }

            if(definiteWallCollision || (actualDir /*&& Point((uint32)obj->x, (uint32)obj->y) != pcur*/ && !_layerMgr->CanMoveToDirection(obj, actualDir)))
            {
                obj->OnTouchWall(actualDir, phys.xspeed, phys.yspeed); // if we are going right, the wall hits us right...
                phys._wallTouched = true;
            }

            // -- end of messy part --

            // now check where we can move from this position
            if(dirx && !_layerMgr->CanMoveToDirection(obj, dirx))
            {
                phys.xspeed *= -(dirx & DIRECTION_LEFT ? phys.lbounce : phys.rbounce);
            }
            if(diry && !_layerMgr->CanMoveToDirection(obj, diry))
            {
                phys.yspeed *= -(diry & DIRECTION_UP ? phys.ubounce : phys.dbounce);
            }
        }
    }


    if(begin_ix != int32(obj->x) || begin_iy != int32(obj->y))
    {
        obj->UpdateAnchor();
        obj->SetMoved(true);
    }

    phys._lastx = begin_x;
    phys._lasty = begin_y;
}
