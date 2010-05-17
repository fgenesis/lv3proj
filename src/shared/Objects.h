#ifndef OBJECTS_H
#define OBJECTS_H

#include <falcon/engine.h>
#include "SharedStructs.h"
#include "PhysicsSystem.h"

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
    OBJTYPE_ITEM    = 2,
    OBJTYPE_UNIT    = 3,
    OBJTYPE_PLAYER  = 4
};

// basic object class, defines shared properties but can't be instantiated
class BaseObject
{
    friend class ObjectMgr;

public:
    BaseObject();
    virtual ~BaseObject();
    virtual void Init(void) = 0;
    FalconProxyObject *_falObj;
    void unbind(void); // clears bindings from falcon, should be called before deletion
    inline uint32 GetId(void) { return _id; }
    inline uint8 GetType(void) { return type; }

    inline void SetLayerMgr(LayerMgr *mgr) { _layermgr = mgr; }
    inline void MustDie(bool b) { _mustdie = b; }
    inline bool MustDie(void) { return _mustdie; }

protected:
    uint32 _id;
    uint8 type;
    bool _mustdie; // if this is true, do not touch this object anymore



    LayerMgr *_layermgr; // required for collision checks
};


// A rectangle with object properties and falcon bindings, the base of everything.
// can be used to detect collision with other objects, but not to block their movement.
class ActiveRect : public BaseObject, public BaseRect
{
public:
    virtual void Init(void);

    virtual void SetBBox(float x, float y, uint32 w, uint32 h);
    virtual void SetPos(float x, float y);
    virtual void MoveRelative(float xr, float yr);

    // see SharedDefines.h for the sides enum
    virtual void OnEnter(uint8 side, ActiveRect *who);
    virtual void OnLeave(uint8 side, ActiveRect *who); // TODO: NYI
    virtual bool OnTouch(uint8 side, ActiveRect *who);

    virtual void OnEnteredBy(uint8 side, ActiveRect *who);
    virtual void OnLeftBy(uint8 side, ActiveRect *who); // TODO: NYI
    virtual bool OnTouchedBy(uint8 side, ActiveRect *who);
    

    void AlignToSideOf(ActiveRect *other, uint8 side);

    inline bool HasMoved(void) { return _moved; }
    inline void SetMoved(bool moved = true) { _moved = moved; }
    inline bool IsCollisionEnabled(void) { return _collisionEnabled; }
    inline void SetCollisionEnabled(bool b) { _collisionEnabled = b; }

    virtual float GetDistanceX(ActiveRect *other);
    virtual float GetDistanceY(ActiveRect *other);
    virtual float GetDistance(ActiveRect *other);

    uint32 CanMoveToDirection(uint8 d, uint32 pixels = 1);

protected:

    bool _collisionEnabled; // do collision detection at all?
    bool _moved; // do collision detection if one of the involved objects moved
};


// a normal "Sprite" object. can block other objects.
class Object : public ActiveRect
{
public:
    virtual ~Object();
    virtual void Init(void);

    virtual void OnUpdate(uint32 ms);
    virtual void OnTouchWall(uint8 side, float xspeed, float yspeed);
    virtual void SetBBox(float x, float y, uint32 w, uint32 h);
    virtual void SetPos(float x, float y);

    inline void SetAffectedByPhysics(bool b) { _physicsAffected = b; }
    inline bool IsAffectedByPhysics(void) { return _physicsAffected; }
    inline bool _NeedsLayerUpdate(void) { return _layerId != _oldLayerId; }
    inline void _SetLayerUpdated(void) { _oldLayerId = _layerId; }
    inline void SetLayer(uint32 newLayer) { _layerId = newLayer; } // will be updated in next cycle, before rendering
    inline uint32 GetLayer(void) { return _layerId; }
    inline uint32 GetOldLayer(void) { return _oldLayerId; }
    inline void SetBlocking(bool b) { _blocking = true; }
    inline bool IsBlocking(void) { return _blocking; }
    void SetSprite(BasicTile *tile);

    inline BasicTile *GetSprite(void) { return _gfx; }

    bool CanFallDown(void); // TODO: obsolete



    inline void UpdateAnchor(void)
    {
        anchor.x = int32(x) + (w / 2);
        anchor.y = int32(y) + h;
    }

    PhysProps phys;
    struct
    {
        int32 x,y;
        uint32 w,h;
    } _oldLayerRect; // this is used to keep track of the previous positions of the object. necessary to update the collision map of the LayerMgr.
    int32 gfxoffsx, gfxoffsy; // especially NPC objects can have a larger sprite then their bounding box. these are the relative offsets for the sprite.

protected:
    void _GenericInit(void);
    bool _physicsAffected;
    bool _blocking; // true if this object affects the LayerMgr's CollisionMap
    uint32 _layerId; // layer ID where this sprite is drawn on
    uint32 _oldLayerId; // prev. layer id, if theres a difference between both, ObjectMgr::Update() has to correct the layer set assignment
    BasicTile *_gfx;
    Point anchor; // where this object stands on the ground (center of object) - used for CanFallDown()

};

// an item a player carries around in the inventory
class Item : public Object
{
public:
    virtual void Init(void);

    virtual bool OnUse(Object *who);
};

// unit, most likely some NPC, enemy, or player
class Unit : public Object
{
public:
    virtual void Init(void);
};

// the Player class, specialized for playable characters like these vikings this is all about
class Player : public Unit
{
public:
    virtual void Init(void);

};

#endif
