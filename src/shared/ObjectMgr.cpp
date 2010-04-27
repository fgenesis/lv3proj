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
    RemoveAll();
}

void ObjectMgr::RemoveAll(void)
{
    for(ObjectMap::iterator it = _store.begin(); it != _store.end(); it++)
    {
        BaseObject *obj = it->second;
        obj->unbind();
        delete obj;
    }
    for(uint32 i = 0; i < LAYER_MAX; ++i)
        _renderLayers[i].clear();

    _store.clear();
    _curId = 0;
}

uint32 ObjectMgr::Add(BaseObject *obj)
{
    obj->_id = ++_curId;
    _store[obj->_id] = obj;
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

ObjectMap::iterator ObjectMgr::GetIterator(uint32 id)
{
    ObjectMap::iterator it = _store.find(id);
    if(it != _store.end())
        return it;

    return _store.end();
}

ObjectMap::iterator ObjectMgr::Remove(uint32 id)
{
    ObjectMap::iterator it = GetIterator(id);
    BaseObject *obj = it->second;
    if(obj)
    {
        DEBUG(logdebug("ObjectMgr::Remove(%u) -> "PTRFMT, id, obj));
        it = _store.erase(it);
        if(obj->GetType() >= OBJTYPE_OBJECT)
        {
            _renderLayers[((Object*)obj)->GetLayer()].erase((Object*)obj);
        }
        obj->unbind();
        delete obj;
    }
    return it;
}

void ObjectMgr::Update(uint32 ms)
{
    // if no time passed, do nothing
    // TODO: this is *maybe* wrong, check how it works out
    if(!ms)
        return;

    // first, update all objects, handle physics, movement, etc.
    for(ObjectMap::iterator it = _store.begin(); it != _store.end(); it++)
    {
        ActiveRect *base = (ActiveRect*)it->second;

        // do not touch objects flagged for deletion
        if(base->MustDie())
            continue;

        if(base->GetType() >= OBJTYPE_OBJECT)
        {
            Object *obj = (Object*)base;
            // physics
            if(obj->IsAffectedByPhysics())
                _physMgr->UpdatePhysics(obj, ms); // the collision with walls is handled in here. also sets HasMoved() to true if required.
            // update layer sets if changed
            if(obj->_NeedsLayerUpdate())
            {
                _renderLayers[obj->GetOldLayer()].erase(obj);
                _renderLayers[obj->GetLayer()].insert(obj);
                obj->_SetLayerUpdated();
            }
            // update gfx if required
            if(obj->GetSprite() && obj->GetSprite()->GetType() == TILETYPE_ANIMATED)
                ((AnimatedTile*)(obj->GetSprite()))->Update(Engine::GetCurFrameTime());

            obj->OnUpdate(ms);
        }
    }

    // now that every object that should have moved has done so, we can check what collided with what
    for(ObjectMap::iterator it = _store.begin(); it != _store.end(); it++)
    {
        ActiveRect *base = (ActiveRect*)it->second;

        // collision detection (object vs object)
        if(base->MustDie() || !base->IsCollisionEnabled())
            continue;

        // i guess this will be very slow for MANY objects in the game... have to see how this works out
        for(ObjectMap::iterator jt = _store.begin(); jt != _store.end(); jt++)
        {
            ActiveRect *other = (ActiveRect*)jt->second;
            // never calculate collision with self; and only calc if at least one of both objects has moved.
            if(base == other || other->MustDie() || !other->IsCollisionEnabled() || !(base->HasMoved() || other->HasMoved()))
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
    }

    for(ObjectMap::iterator it = _store.begin(); it != _store.end(); )
    {
        BaseObject *obj = it->second;
        if(obj->MustDie())
        {
            // remove expired objects
            it = Remove(obj->GetId());
        }
        else
        {
            // reset moved state for all objects.
            // collision detection and everything done. next movement may be done in next cycle.
            ((ActiveRect*)it->second)->SetMoved(false);
            it++;
        }
    }
}

// this renders the objects.
// it is called from LayerMgr::Render(), so that objects on higher layers are drawn over objects on lower layers
void ObjectMgr::RenderLayer(uint32 id)
{
    SDL_Surface *esf = _engine->GetSurface();
    for(ObjectSet::iterator it = _renderLayers[id].begin(); it != _renderLayers[id].end(); it++)
    {
        Object *obj = *it;
        if(BasicTile *sprite = obj->GetSprite())
        {
            SDL_Rect dst;
            dst.x = int(obj->x) + obj->gfxoffsx;
            dst.y = int(obj->y) + obj->gfxoffsy;
            dst.w = obj->w;
            dst.h = obj->h;
            SDL_BlitSurface(sprite->GetSurface(), NULL, esf, &dst);
        }
    }
}

void ObjectMgr::GetAllObjectsIn(BaseRect& rect, ObjectList& result)
{
    for(ObjectMap::iterator it = _store.begin(); it != _store.end(); it++)
        if(((ActiveRect*)it->second)->CollisionWith(&rect))
            result.push_back(it->second);
}

