#include "common.h"
#include "Objects.h"
#include "ObjectMgr.h"
#include "LayerMgr.h"

ObjectMgr::ObjectMgr()
: _curId(0)
{
}

ObjectMgr::~ObjectMgr()
{
    RemoveAll(true, NULL);
}

void ObjectMgr::RemoveAll(bool del, cleanfunc f)
{
    for(ObjectMap::iterator it = _store.begin(); it != _store.end(); it++)
    {
        if(f)
            (it->second->*f)();
        if(del)
            delete it->second;
    }
    _store.clear();
    _curId = 0;
}

uint32 ObjectMgr::Add(BaseObject *obj)
{
    obj->_id = ++_curId;
    _store[_curId] = obj;
    return _curId;
}

BaseObject *ObjectMgr::Get(uint32 id)
{
    ObjectMap::iterator it = _store.find(id);
    if(it != _store.end())
        return it->second;

    return NULL;
}

void ObjectMgr::Remove(uint32 id)
{
    _store.erase(id);
}

void ObjectMgr::Update(void)
{
    for(ObjectMap::iterator it = _store.begin(); it != _store.end(); it++)
    {
        ActiveRect *base = (ActiveRect*)it->second;

        // physics
        if(base->GetType() >= OBJTYPE_OBJECT && ((Object*)base)->IsAffectedByPhysics())
        {
            _physMgr->UpdatePhysics((Object*)base); // the collision with walls is handled in here
        }
        // collision detection (object vs object)
        // i guess this will be very slow for MANY objects in the game... have to see how this works out
        for(ObjectMap::iterator jt = _store.begin(); jt != _store.end(); jt++)
        {
            ActiveRect *other = (ActiveRect*)jt->second;
            // never calculate collision with self
            if(base == other)
                continue;

            if(uint8 side = base->CollisionWith(other))
            {
                // if OnTouch() returns true, movement should be stopped and remain "touching" (= no overlap should occur)
                if(base->OnTouch(side, other))
                {
                    // we should only move objects around, since rects are considered static unless manually moved
                    if(base->GetType() >= OBJTYPE_OBJECT)
                    {
                        int32 xold = base->x, yold = base->y;
                        base->AlignToSideOf(other, side);
                        if(_layerMgr->CollisionWith(base))
                        {
                            // ouch, new position collided with wall... reset position to old
                            // and now we HAVE TO call OnEnter()
                            // TODO: try pushing to left or right a little (at an elevator, for example. it never crushes anything)
                            //       (but this can be done in falcon too.. i think)
                            base->x = xold;
                            base->y = yold;
                            base->OnEnter(side, other);
                        }
                    }
                }
                else
                {
                    base->OnEnter(side, other);
                }
            }
        }
    }
}




