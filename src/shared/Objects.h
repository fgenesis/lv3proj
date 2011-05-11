#ifndef OBJECTS_H
#define OBJECTS_H

#include <falcon/engine.h>
#include "SharedStructs.h"
#include "PhysicsSystem.h"
#include "DelayedDeletable.h"

/*
 * NOTE: The OnEnter(), OnLeave(), OnWhatever() functions are defined in FalconObjectModule.cpp !!
 */

class Object;
class ObjectMgr;
class BaseObject;
class FalconProxyObject;
class BasicTile;

enum ObjectType
{
    OBJTYPE_RECT    = 0,
    OBJTYPE_OBJECT  = 1,
    OBJTYPE_UNIT    = 2,
    OBJTYPE_PLAYER  = 3
};

// basic object class, defines shared properties but can't be instantiated
class BaseObject : public DelayedDeletable
{
    friend class ObjectMgr;

public:
    BaseObject();
    virtual ~BaseObject();
    virtual void Init(void) = 0;

    inline uint32 GetId(void) { return _id; }
    inline uint8 GetType(void) { return type; }
    inline void SetLayerMgr(LayerMgr *mgr) { _layermgr = mgr; }

    void unbind(void); // clears bindings from falcon, should be called before deletion


    // return all objects that are attached to 'this'
    void GetAttached(std::set<BaseObject*>& found) const;

    // attach this object to another
    inline void AttachTo(BaseObject *other){ _parents.insert(other); other->_children.insert(this); }

    // attach another object to this
    inline void AttachToThis(BaseObject *who) { _children.insert(who); who->_parents.insert(this); }

    // detach this object from others
    inline void DetachFrom(BaseObject *other) { _parents.erase(other); other->_children.erase(this); }
    inline void DetachFromAll(void)
    {
        for(std::set<BaseObject*>::iterator it = _parents.begin(); it != _parents.end(); ++it)
            (*it)->_children.erase(this);
        _parents.clear();
    }

    // detach other objects from this
    inline void DetachFromThis(BaseObject *who) { _children.erase(who); who->_parents.erase(this); }
    inline void DetachAllFromThis(void)
    {
        for(std::set<BaseObject*>::iterator it = _children.begin(); it != _children.end(); ++it)
            (*it)->_parents.erase(this);
        _children.clear();
    }
    

    FalconProxyObject *_falObj;

protected:
    std::set<BaseObject*> _children; // objects that are attached to this one
    std::set<BaseObject*> _parents;  // objects this object is attached to
    LayerMgr *_layermgr; // required for collision checks
    uint32 _id;
    uint8 type;
};


// A rectangle with object properties and falcon bindings, the base of everything.
// can be used to detect collision with other objects, but not to block their movement.
// Does not have graphics.
class ActiveRect : public BaseObject, public BaseRect
{
public:
    virtual void Init(void);

    // see SharedDefines.h for the sides enum
    // TODO: use vector physics here
    virtual void OnEnter(uint8 side, ActiveRect *who);
    virtual void OnLeave(uint8 side, ActiveRect *who); // TODO: NYI
    virtual bool OnTouch(uint8 side, ActiveRect *who);

    virtual void OnEnteredBy(uint8 side, ActiveRect *who);
    virtual void OnLeftBy(uint8 side, ActiveRect *who); // TODO: NYI
    virtual bool OnTouchedBy(uint8 side, ActiveRect *who);

    // These take care of moving all objects that are attached to this as well.
    void SetBBox(float x, float y, uint32 w, uint32 h); // ... but only set our own bbox
    void SetPos(float x, float y);
    void SetX(float x);
    void SetY(float y);
    void Move(float xr, float yr);
    void MoveX(float xr);
    void MoveY(float yr);
    // width/height setter not required, but added for interface completeness
    inline void SetW(uint32 w_) { w = w_; }
    inline void SetH(uint32 h_) { h = h_; }

    inline bool HasMoved(void) const { return _moved; }
    inline void SetMoved(bool moved = true) { _moved = moved; }
    inline bool IsCollisionEnabled(void) const { return _collisionMask; }
    inline void SetCollisionMask(uint32 mask) { _collisionMask = mask; }
    inline uint32 GetCollisionMask(void) const { return _collisionMask; }
    inline void SetUpdate(bool b) { _update = b; }
    inline bool IsUpdate(void) const { return _update; }

    virtual float GetDistanceX(ActiveRect *other) const;
    virtual float GetDistanceY(ActiveRect *other) const;
    virtual float GetDistance(ActiveRect *other) const;


    Vector2df CanMoveToDirection(uint8 d, const Vector2df& dir); // TODO: deprecate

protected:

    uint32 _collisionMask; // two objects can collide if maskA & maskB != 0. Collision disabled if 0.
    bool _moved; // do collision detection if one of the involved objects moved
    bool _update; // if true, call OnUpdate() in every cycle
};


// a normal "Sprite" object. can block other objects.
class Object : public ActiveRect
{
public:
    virtual ~Object();
    virtual void Init(void);

    virtual void OnUpdate(uint32 ms);
    virtual void OnTouchWall(uint8 side, float xspeed, float yspeed);

    inline void SetAffectedByPhysics(bool b) { _physicsAffected = b; }
    inline bool IsAffectedByPhysics(void) const { return _physicsAffected; }
    inline bool _NeedsLayerUpdate(void) const { return _layerId != _oldLayerId; }
    inline void _SetLayerUpdated(void) { _oldLayerId = _layerId; }
    inline void SetLayer(uint32 newLayer) { _layerId = newLayer; } // will be updated in next cycle, before rendering
    inline uint32 GetLayer(void) const { return _layerId; }
    inline uint32 GetOldLayer(void) const { return _oldLayerId; }
    inline void SetBlocking(bool b) { _blocking = true; }
    inline bool IsBlocking(void) const { return _blocking; }
    inline void SetVisible(bool b) { _visible = b; }
    inline bool IsVisible(void) const { return _visible; }

    void SetSprite(BasicTile *tile);
    inline BasicTile *GetSprite(void) { return _gfx; }

    PhysProps phys;

    struct
    {
        int32 x,y;
        uint32 w,h;
    } _oldLayerRect; // this is used to keep track of the previous positions of the object. necessary to update the collision map of the LayerMgr.

protected:
    void _GenericInit(void);

    BasicTile *_gfx;
    uint32 _layerId; // layer ID where this sprite is drawn on
    uint32 _oldLayerId; // prev. layer id, if theres a difference between both, ObjectMgr::Update() has to correct the layer set assignment
    bool _physicsAffected;
    bool _blocking; // true if this object affects the LayerMgr's CollisionMap
    bool _visible;
};

// unit, most likely some NPC, enemy, or player
class Unit : public Object
{
public:
    virtual void Init(void);
};

// the Player class, specialized for playable characters
class Player : public Unit
{
public:
    virtual void Init(void);

};

#endif
