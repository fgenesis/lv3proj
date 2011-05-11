#ifndef PHYSICSSYSTEM_H
#define PHYSICSSYSTEM_H

#include "Vector2d.h"

class LayerMgr;
class Object;
class ObjectMgr;


// an object can have certain physical properties.
// we put them into an extra struct for the sake of clearer code
// and to make integration with falcon easier.
// TODO: create constructor to initialize it with *useful* values?
struct PhysProps
{
    float mass;

    std::vector<Vector2df> speed;
    std::vector<Vector2df> accel;
    std::vector<Vector2df> friction;

    PhysProps()
    {
        resize(5); // something to get started
    }


    inline void resize(size_t s)
    {
        speed.resize(s);
        accel.resize(s);
        friction.resize(s);
    }

    inline size_t size() const
    {
        return speed.size();
    }

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
    void UpdatePhysics(Object *obj, float frac);

    EnvPhysProps envPhys;
    
    inline void SetLayerMgr(LayerMgr *layers) { _layerMgr = layers; }
    inline void SetObjMgr(ObjectMgr *mgr) { _objMgr = mgr; }
private:

    void _ApplyAccel(Object *obj, float tf);
    void _ApplyFriction(Object *obj, float tf);
    void _ApplySpeed(Object *obj, float tf);
    void _DoCollision(Object *obj, const Vector2df& oldpos);

    LayerMgr *_layerMgr;
    ObjectMgr *_objMgr;
};



#endif
