#ifndef OBJECTMGR_H
#define OBJECTMGR_H

#include <map>

class BaseObject;
class PhysicsMgr;
class AppFalconGame;

typedef std::map<uint32, BaseObject*> ObjectMap;
typedef void (BaseObject::*cleanfunc)(void);


class ObjectMgr
{
public:
    ObjectMgr();
    ~ObjectMgr();
    uint32 Add(BaseObject*);
    BaseObject *Get(uint32 id);
    void Update(void);
    void Remove(uint32 id);
    void RemoveAll(bool del, cleanfunc f);

    inline void SetPhysicsMgr(PhysicsMgr *pm) { _physMgr = pm; }
    inline void SetFalconBindings(AppFalconGame *f) { _falcon = f; }
    inline void SetLayerMgr(LayerMgr *layers) {_layerMgr = layers; }

protected:
    uint32 _curId;
    ObjectMap _store;
    PhysicsMgr *_physMgr;
    AppFalconGame *_falcon;
    LayerMgr *_layerMgr;
};

#endif
