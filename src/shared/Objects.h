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

protected:
    uint32 _id;
    uint8 type;
};

// the base of everything
class ActiveRect : public BaseObject
{
public:
    virtual void Init(void);

    // see SharedDefines.h for the sides enum
    virtual void OnEnter(uint8 side, ActiveRect *who);
    virtual void OnLeave(uint8 side, ActiveRect *who);
    virtual bool OnTouch(uint8 side, ActiveRect *who);

    int x, y, w, h;

    uint8 CollisionWith(ActiveRect *other); // returns side where the collision occurred
    void AlignToSideOf(ActiveRect *other, uint8 side);

    // Method to calculate the second X corner
    inline int x2() const { return x+w; }
    // Method to calculate the second Y corner
    inline int y2() const { return y+h; }
};


// a normal "Sprite" object
class Object : public ActiveRect
{
public:
    virtual void Init(void);
    virtual void SetBBox(uint32 x, uint32 y, uint32 w, uint32 h);

    virtual void OnUpdate(uint32 ms);

    inline void SetAffectedByPhysics(bool b) { _physicsAffected = b; }
    inline bool IsAffectedByPhysics(void) { return _physicsAffected; }

    PhysProps phys;

protected:
    bool _physicsAffected;
    bool _moved;


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
    virtual void SetBBox(uint32 x, uint32 y, uint32 w, uint32 h);

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
