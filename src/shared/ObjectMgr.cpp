#include "common.h"
#include "Engine.h"
#include "Objects.h"
#include "ObjectMgr.h"
#include "LayerMgr.h"
#include "Tile.h"
#include "SDL_func.h"

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

ObjectMap::iterator ObjectMgr::_Remove(uint32 id)
{
    ObjectMap::iterator it = _store.find(id);
    if(it == _store.end())
        return it;

    BaseObject *obj = it->second;
    if(obj)
    {
        DEBUG(logdebug("ObjectMgr::Remove(%u) -> "PTRFMT, id, obj));
        _store.erase(it++);
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
        if(base->CanBeDeleted())
            continue;

        if(base->GetType() >= OBJTYPE_OBJECT)
        {
            Object *obj = (Object*)base;
            _layerMgr->RemoveFromCollisionMap(obj);
            // physics
            if(obj->IsAffectedByPhysics())        // the collision with walls is handled in here. also sets HasMoved() to true if required.
                _physMgr->UpdatePhysics(obj, ms); // also takes care of triggering OnTouch() for solid objects vs Players and other specific things
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

            if(obj->IsUpdate())
                obj->OnUpdate(ms);

            _layerMgr->UpdateCollisionMap(obj);
        }
    }

    // now that every object that should have moved has done so, we can check what collided with what
    for(ObjectMap::iterator it = _store.begin(); it != _store.end(); it++)
    {
        ActiveRect *base = (ActiveRect*)it->second;

        // collision detection (object vs object)
        if(base->CanBeDeleted() || !base->IsCollisionEnabled() || !base->HasMoved())
            continue;

        // i guess this will be very slow for MANY objects in the game... have to see how this works out
        for(ObjectMap::iterator jt = _store.begin(); jt != _store.end(); jt++)
        {
            ActiveRect *other = (ActiveRect*)jt->second;
            // never calculate collision with self, invalid, or non-colliding objects
            if(base == other || other->CanBeDeleted() || !other->IsCollisionEnabled())
                continue;
            // skip solid objects, as these are handled in the physics system.
            if(other->GetType() >= OBJTYPE_OBJECT && ((Object*)other)->IsBlocking())
                continue;

            // <base> is the object that has moved, trigger collision from it.
            if(uint8 side = base->CollisionWith(other))
            {
                HandleObjectCollision(base, other, side);
            }
        }
    }

    for(ObjectMap::iterator it = _store.begin(); it != _store.end(); )
    {
        BaseObject *obj = it->second;
        if(obj->CanBeDeleted())
        {
            // remove expired objects
            it = _Remove(obj->GetId());
        }
        else
        {
            // reset moved state for all objects.
            // collision detection and everything done. next movement may be done in next cycle.
            ((ActiveRect*)it->second)->SetMoved(false);
            ++it;
        }
    }
}

// <base> is the object that has moved, usually; <side> is <base's> side where <other> collided with it
void ObjectMgr::HandleObjectCollision(ActiveRect *base, ActiveRect *other, uint8 side)
{
    // if one of the 2 functions returns true, movement should be stopped and remain "touching" (= no overlap should occur)
    bool touchResult = base->OnTouch(side, other);
    bool touchedByResult = other->OnTouchedBy(InvertSide(side), base);
    if(touchResult || touchedByResult)
    {
        // we should only move objects around, since rects are considered static unless manually moved
        if(base->GetType() >= OBJTYPE_OBJECT)
        {
            float xold = base->x, yold = base->y; // TODO: need width and height too?
            uint8 oside = InvertSide(side);
            base->AlignToSideOf(other, oside);
            if(_layerMgr->CollisionWith(base, 4, ((Object*)base)->IsBlocking() ? ~LCF_BLOCKING_OBJECT : LCF_ALL)) // if object is blocking skip this flag
            {
                // ouch, new position collided with wall... reset position to old
                // and now we HAVE TO call OnEnter()
                // TODO: try pushing to left or right a little (at an elevator, for example. it never crushes anything)
                //       (but this can be done in falcon too.. i think)
                base->x = xold;
                base->y = yold;
                base->OnEnter(side, other);
                other->OnEnteredBy(InvertSide(side), base);
            }
        }
    }
    else
    {
        base->OnEnter(side, other);
        other->OnEnteredBy(InvertSide(side), base);
    }
}


// this renders the objects.
// it is called from LayerMgr::Render(), so that objects on higher layers are drawn over objects on lower layers
void ObjectMgr::RenderLayer(uint32 id)
{
    SDL_Surface *esf = _engine->GetSurface();
    Point cam = _engine->GetCamera();
    for(ObjectSet::iterator it = _renderLayers[id].begin(); it != _renderLayers[id].end(); it++)
    {
        Object *obj = *it;
        if(obj->IsVisible())
        {
            if(BasicTile *sprite = obj->GetSprite())
            {
                SDL_Rect dst;
                dst.x = int(obj->x) + obj->gfxoffsx - cam.x;
                dst.y = int(obj->y) + obj->gfxoffsy - cam.y;
                dst.w = obj->w;
                dst.h = obj->h;
                SDL_BlitSurface(sprite->GetSurface(), NULL, esf, &dst);
            }
        }
    }
}

void ObjectMgr::GetAllObjectsIn(BaseRect& rect, ObjectWithSideSet& result, uint8 force_side /* = SIDE_NONE */) const
{
    for(ObjectMap::const_iterator it = _store.begin(); it != _store.end(); it++)
        if(uint8 side = ((ActiveRect*)it->second)->CollisionWith(&rect))
            result.insert(std::pair<BaseObject*,uint8>(it->second, force_side ? force_side : side));
}

void ObjectMgr::RenderBBoxes(void)
{
    SDL_Rect r;
    Point cam = _engine->GetCamera();
    for(ObjectMap::iterator it = _store.begin(); it != _store.end(); it++)
    {
        BaseRect br = ((ActiveRect*)it->second)->cloneRect();
        r.x = int32(br.x) - cam.x;
        r.y = int32(br.y) - cam.y;
        r.h = br.h;
        r.w = br.w;
        SDLfunc_drawRectangle(_engine->GetSurface(), r, 0xDF, 0xDF, 0xDF, 0);
    }
}

void ObjectMgr::dbg_setcoll(bool b)
{
    for(ObjectMap::iterator it = _store.begin(); it != _store.end(); it++)
    {
        if(it->second->GetType() >= OBJTYPE_OBJECT)
        {
            if(b)
                _layerMgr->UpdateCollisionMap((Object*)it->second);
            else
                _layerMgr->RemoveFromCollisionMap((Object*)it->second);
        }
    }
}
