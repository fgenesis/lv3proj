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
        : mass(0)
    {
        resize(1); // something to get started
    }

    inline PhysEntry& operator[] (size_t i)
    {
        return _p[i];
    }

    inline const PhysEntry& operator[] (size_t i) const
    {
        return _p[i];
    }

    // safe variant
    /*inline PhysEntry getSafe(size_t i) const
    {
        return (i < size() ? _p[i] : PhysEntry());
    }*/ // probably unnecessary and confusing

    inline PhysEntry& get(size_t i)
    {
        enlarge(i + 1);
        return _p[i];
    }

    // auto-increase size
    inline void set(size_t i, const PhysEntry &p)
    {
        enlarge(i + 1);
        _p[i] = p;
    }

    inline void enlarge(size_t s)
    {
        if(size() < s)
            resize(s);
    }

    inline void resize(size_t s)
    {
        _p.resize(s);
    }

    inline size_t size() const
    {
        return _p.size();
    }

    float mass;

private:
    std::vector<PhysEntry> _p;

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
    void _ApplySpeedAndCollision(Object *obj, float tf);

    LayerMgr *_layerMgr;
    ObjectMgr *_objMgr;
};



#endif
