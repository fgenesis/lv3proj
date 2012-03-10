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
            //_layerMgr->RemoveFromCollisionMap((Object*)obj);
            _renderLayers[((Object*)obj)->GetLayer()].erase((Object*)obj);
        }
        obj->unbind();
        delete obj;
    }
    return it;
}

void ObjectMgr::Update(float dt, uint32 frametime)
{
    // if no time passed, do nothing
    // TODO: this is *maybe* wrong, check how it works out
    if(!dt)
        return;

    // first, update all objects, handle physics, movement, etc.
    for(ObjectMap::iterator it = _store.begin(); it != _store.end(); it++)
    {
        ActiveRect *base = (ActiveRect*)it->second;

        if(base->GetType() >= OBJTYPE_OBJECT)
        {
            Object *obj = (Object*)base;

            _layerMgr->RemoveFromCollisionMap(obj);

            // do not touch objects flagged for deletion
            if(base->CanBeDeleted())
                continue;

            // physics
            if(obj->IsAffectedByPhysics())
                _physMgr->UpdatePhysics(obj, dt); // the collision with walls is handled in here. also sets HasMoved() to true if required.
            // update layer sets if changed
            if(obj->_NeedsLayerUpdate())
            {
                _renderLayers[obj->GetOldLayer()].erase(obj);
                _renderLayers[obj->GetLayer()].insert(obj);
                obj->_SetLayerUpdated();
            }
            // update gfx if required
            if(obj->GetSprite() && obj->GetSprite()->GetType() == TILETYPE_ANIMATED)
                ((AnimatedTile*)(obj->GetSprite()))->Update(frametime); // TODO: remove the frametime here

            if(obj->IsUpdate())
                obj->OnUpdate(dt);

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
            if(base == other || other->CanBeDeleted() || !other->CollisionMaskMatch(base))
                continue;

            // <base> is the object that has moved, trigger collision from it.
            if(base->CollisionWith(other))
            {
                base->OnCollide(other);
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

// this renders the objects.
// it is called from LayerMgr::Render(), so that objects on higher layers are drawn over objects on lower layers
void ObjectMgr::RenderLayer(uint32 id)
{
    SDL_Surface *esf = _engine->GetSurface();
    Camera cam = _engine->GetCamera();
    TileLayer *layer = _engine->_GetLayerMgr()->GetLayer(id);
    float parallaxMulti = layer ? layer->parallaxMulti : 1.0f; // layer may be NULL and still have objects
    Vector2df draw;
    SDL_Rect dst;
    for(ObjectSet::iterator it = _renderLayers[id].begin(); it != _renderLayers[id].end(); ++it)
    {
        Object *obj = *it;
        if(obj->IsVisible())
        {
            if(BasicTile *sprite = obj->GetSprite())
            {
                draw.x = obj->pos.x + obj->gfxoffs.x;
                draw.y = obj->pos.y + obj->gfxoffs.y;
                cam.TranslateVector(draw);
                draw.x *= parallaxMulti;
                draw.y *= parallaxMulti;
                dst.x = draw.x;
                dst.y = draw.y;
                dst.w = obj->w;
                dst.h = obj->h;
                SDL_BlitSurface(sprite->GetSurface(), NULL, esf, &dst);
            }
        }
    }
}

void ObjectMgr::GetAllObjectsIn(BaseRect& rect, BaseObjectSet& result) const
{
    for(ObjectMap::const_iterator it = _store.begin(); it != _store.end(); it++)
        if(((ActiveRect*)it->second)->CollisionWith(&rect))
            result.insert(it->second);
}

void ObjectMgr::RenderBBoxes(void)
{
    SDL_Rect r;
    Camera cam = _engine->GetCamera();
    static const uint32 vCol[4] = // FIXME: fix this for big endian
    {
        0xFF9FFF9F,
        0xFFFF0000,
        0xFF00FF00,
        0xFFFFFFFF,
    };
    for(ObjectMap::iterator it = _store.begin(); it != _store.end(); it++)
    {
        BaseRect br = ((ActiveRect*)it->second)->cloneRect();
        r.x = int32(br.x) - cam.x; // FIXME: this is weird
        r.y = int32(br.y) - cam.y;
        r.h = br.h;
        r.w = br.w;
        SDLfunc_drawRectangle(_engine->GetSurface(), r, 0xDF, 0xDF, 0xDF, 0);

        if(it->second->GetType() >= OBJTYPE_OBJECT)
        {
            Object *obj = (Object*)it->second;
            Vector2df origin = obj->pos + (Vector2df(obj->w, obj->h) / 2);
            Vector2df dest;
            origin.x -= cam.x; // FIXME: this is weird
            origin.y -= cam.y;
            int32 ox = int32(origin.x);
            int32 oy = int32(origin.y);

            // render vectors
            dest = origin + obj->phys.speed;
            SDLfunc_drawLine(_engine->GetSurface(), ox, oy, dest.x, dest.y, vCol[3]);
            dest = origin + obj->phys.accel;
            SDLfunc_drawLine(_engine->GetSurface(), ox, oy, dest.x, dest.y, vCol[1]);
            dest = origin + obj->phys.friction;
            SDLfunc_drawLine(_engine->GetSurface(), ox, oy, dest.x, dest.y, vCol[2]);
        }
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
