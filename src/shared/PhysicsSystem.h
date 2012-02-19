#ifndef PHYSICSSYSTEM_H
#define PHYSICSSYSTEM_H

#include "Vector2d.h"

class LayerMgr;
class Object;
class ObjectMgr;


// an object can have certain physical properties.
// we put them into extra structs for the sake of clearer code
// and to make integration with falcon easier.
struct PhysEntry
{
    Vector2df speed;
    Vector2df accel;
    Vector2df friction;
};

class PhysProps
{
public:

    PhysProps()
        : mass(0), gravityMult(1)
    {
    }

    float mass;
    float gravityMult;
    Vector2df speed;
    Vector2df accel;
    Vector2df friction;
};

// and the environment can have certain physical properties too
struct EnvPhysProps
{
    Vector2df gravity;
};

class PhysicsMgr
{
public:
    PhysicsMgr();
    void SetDefaults(void);
    void UpdatePhysics(Object *obj, float dt);

    EnvPhysProps envPhys;
    
    inline void SetLayerMgr(LayerMgr *layers) { _layerMgr = layers; }
    inline void SetObjMgr(ObjectMgr *mgr) { _objMgr = mgr; }
private:

    void _ApplyAccel(Object *obj, float tf);
    void _ApplyFriction(Object *obj, float tf);
    void _ApplySpeedAndCollision(Object *obj, float tf);

    LayerMgr *_layerMgr;
    ObjectMgr *_objMgr;
};



#endif
