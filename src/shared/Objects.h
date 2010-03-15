#ifndef OBJECTS_H
#define OBJECTS_H

#include <falcon/engine.h>
#include "SharedStructs.h"
#include "PhysicsSystem.h"

/*
 * NOTE: The OnEnter(), OnLeave(), OnWhatever() functions are defined in FalconGameModule.cpp !!
 */

class Object;
class ObjectMgr;
class BaseObject;
class FalconProxyObject;
struct BasicTile;

enum ObjectType
{
    OBJTYPE_RECT    = 0,
    OBJTYPE_OBJECT  = 1,
    OBJTYPE_ITEM    = 2,
    OBJTYPE_UNIT    = 3,
    OBJTYPE_PLAYER  = 4
};

class BaseObject
{
    friend class ObjectMgr;

public:
    BaseObject();
    virtual void Init(void) = 0;
    FalconProxyObject *_falObj;
    void unbind(void); // clears bindings from falcon, should be called before deletion
    inline uint32 GetId(void) { return _id; }
    inline uint8 GetType(void) { return type; }

    inline void SetLayerMgr(LayerMgr *mgr) { _layermgr = mgr; }

protected:
    uint32 _id;
    uint8 type;

    LayerMgr *_layermgr; // required for collision checks
};

// the base of everything
class ActiveRect : public BaseObject
{
public:
    virtual void Init(void);

    virtual void SetBBox(float x, float y, uint32 w, uint32 h);
    virtual void SetPos(float x, float y);

    // see SharedDefines.h for the sides enum
    virtual void OnEnter(uint8 side, ActiveRect *who);
    virtual void OnLeave(uint8 side, ActiveRect *who);
    virtual bool OnTouch(uint8 side, ActiveRect *who);

    float x, y; // we have to use floats for correct movement, so that rounding does not make fuss with movements < 1 pixel per step.
    uint32 w, h;

    uint8 CollisionWith(ActiveRect *other); // returns side where the collision occurred
    void AlignToSideOf(ActiveRect *other, uint8 side);

    // Method to calculate the second X corner
    inline int x2(void) const { return int32(x) + w; }
    // Method to calculate the second Y corner
    inline int y2(void) const { return int32(y) + h; }

    // Method to calculate the second X corner (float)
    inline float x2f(void) const { return x + float(w); }
    // Method to calculate the second Y corner (float)
    inline float y2f(void) const { return y + float(h); }

    inline bool HasMoved(void) { return _moved; }
    inline void SetMoved(bool moved = true) { _moved = moved; }

protected:

    bool _moved; // do collision detection if one of the involved objects moved
};


// a normal "Sprite" object
class Object : public ActiveRect
{
public:
    virtual void Init(void);

    virtual void OnUpdate(uint32 ms);

    inline void SetAffectedByPhysics(bool b) { _physicsAffected = b; }
    inline bool IsAffectedByPhysics(void) { return _physicsAffected; }
    inline bool _NeedsLayerUpdate(void) { return _layerId != _oldLayerId; }
    inline void _SetLayerUpdated(void) { _oldLayerId = _layerId; }
    inline void SetLayer(uint32 newLayer) { _layerId = newLayer; } // will be updated in next cycle, before rendering
    inline uint32 GetLayer(void) { return _layerId; }
    inline uint32 GetOldLayer(void) { return _oldLayerId; }
    inline void SetSprite(BasicTile *tile)
    {
        // TODO: properly cleanup old gfx
        _gfx = tile;
    }
    inline BasicTile *GetSprite(void) { return _gfx; }

    PhysProps phys;

protected:
    void _GenericInit(void);
    bool _physicsAffected;
    uint32 _layerId; // layer ID where this sprite is drawn on
    uint32 _oldLayerId; // prev. layer id, if theres a difference between both, ObjectMgr::Update() has to correct the layer set assignment
    BasicTile *_gfx;


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
    virtual void SetBBox(float x, float y, uint32 w, uint32 h);
    virtual void SetPos(float x, float y);
    inline void UpdateAnchor(void)
    {
        anchor.x = (int32(x) + w) / 2; // (x + w) / 2
        anchor.y = int32(y) + h;
    }

protected:
    Point anchor; // where this unit stands on the ground (center of object)

};

// the Player class, specialized for playable characters like these vikings this is all about
class Player : public Unit
{
public:
    virtual void Init(void);

};

#endif
