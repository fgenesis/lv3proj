#include "common.h"
#include "Engine.h"
#include "Objects.h"
#include "ObjectMgr.h"
#include "LayerMgr.h"
#include "Tile.h"

ObjectMgr::ObjectMgr(Engine *e)
: _curId(0)
{
    _engine = e;
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
    for(uint32 i = 0; i < LAYER_MAX; ++i)
        _renderLayers[i].clear();

    _store.clear();
    _curId = 0;
}

uint32 ObjectMgr::Add(BaseObject *obj)
{
    obj->_id = ++_curId;
    _store[_curId] = obj;
    if(obj->GetType() >= OBJTYPE_OBJECT)
    {
        _renderLayers[((Object*)obj)->GetLayer()].insert((Object*)obj);
    }
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
    BaseObject *obj = Get(id);
    _store.erase(id);
    if(obj->GetType() >= OBJTYPE_OBJECT)
    {
        _renderLayers[((Object*)obj)->GetLayer()].erase((Object*)obj);
    }
}

void ObjectMgr::Update(uint32 ms)
{
    for(ObjectMap::iterator it = _store.begin(); it != _store.end(); it++)
    {
        ActiveRect *base = (ActiveRect*)it->second;

        if(base->GetType() >= OBJTYPE_OBJECT)
        {
            Object *obj = (Object*)base;
            // physics
            if(obj->IsAffectedByPhysics())
                _physMgr->UpdatePhysics(obj); // the collision with walls is handled in here. also sets HasMoved() to true if required.
            // update layer sets if changed
            if(obj->_NeedsLayerUpdate())
            {
                _renderLayers[obj->GetOldLayer()].erase(obj);
                _renderLayers[obj->GetLayer()].insert(obj);
                obj->_SetLayerUpdated();
            }
            // update gfx if required
            if(obj->GetSprite() && obj->GetSprite()->type == TILETYPE_ANIMATED)
                ((AnimatedTile*)(obj->GetSprite()))->Update(Engine::GetCurFrameTime());

            obj->OnUpdate(ms);
        }
        // collision detection (object vs object)
        // i guess this will be very slow for MANY objects in the game... have to see how this works out
        for(ObjectMap::iterator jt = _store.begin(); jt != _store.end(); jt++)
        {
            ActiveRect *other = (ActiveRect*)jt->second;
            // never calculate collision with self; and only calc if at least one of both objects has moved 
            if(base == other || !(base->HasMoved() || other->HasMoved()))
                continue;

            if(uint8 side = base->CollisionWith(other))
            {
                // if OnTouch() returns true, movement should be stopped and remain "touching" (= no overlap should occur)
                if(base->OnTouch(side, other))
                {
                    // we should only move objects around, since rects are considered static unless manually moved
                    if(base->GetType() >= OBJTYPE_OBJECT)
                    {
                        int32 xold = base->x, yold = base->y; // TODO: need width and height too?
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
        base->SetMoved(false); // collision detection and everything done. next movement may be done in next cycle.
    }
}

void ObjectMgr::RenderLayer(uint32 id)
{
    for(ObjectSet::iterator it = _renderLayers[id].begin(); it != _renderLayers[id].end(); it++)
    {
        Object *obj = *it;
        if(obj->GetSprite())
        {
            SDL_Rect dst;
            dst.x = obj->x;
            dst.y = obj->y;
            dst.w = obj->w;
            dst.h = obj->h;
            SDL_BlitSurface(obj->GetSprite()->surface, NULL, _engine->GetSurface(), &dst);
        }
    }
}

