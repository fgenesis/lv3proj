#ifndef PHYSICSSYSTEM_H
#define PHYSICSSYSTEM_H

class LayerMgr;
class Object;
class ObjectMgr;


// an object can have certain physical properties.
// we put them into an extra struct for the sake of clearer code
// and to make integration with falcon easier.
// TODO: create constructor to initialize it with *useful* values?
struct PhysProps
{
    float weight; // in kg
    float xspeed; // current speed, x axis. negative means left, positive means right
    float yspeed; // current speed, y axis. negative means up, positive means down
    float xmaxspeed; // max speed. must NOT be negative, ever!
    float ymaxspeed;
    float xaccel; // current x-axis acceleration. negative means left, positive means right
    float yaccel; // current y-axis acceleration. negative means up, positive means down
    float xfriction; // friction gets subtracted from the abs current speed, until speed is 0
    float yfriction;
    float ubounce; // bounciness, this multiplier is applied to current speed when an object hits a wall, and the direction inverted
    float dbounce; // ... must not be negative.
    float lbounce; // ... directions are separate for up, down, left, right
    float rbounce;

    // internal

    // these are only interesting for the physics system (wall collision), so we store them here
    float _lastx;
    float _lasty;
    bool _wallTouched;
};

// and the environment can have certain physical properties too
struct EnvPhysProps
{
    //float airFriction; // not used (yet)
    float gravity;
};

class PhysicsMgr
{
public:
    void UpdatePhysics(Object *obj, uint32 ms);

    EnvPhysProps envPhys;
    
    inline void SetLayerMgr(LayerMgr *layers) { _layerMgr = layers; }
    inline void SetObjMgr(ObjectMgr *mgr) { _objMgr = mgr; }
private:

    LayerMgr *_layerMgr;
    ObjectMgr *_objMgr;
};



#endif
