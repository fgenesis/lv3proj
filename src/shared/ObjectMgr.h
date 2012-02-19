#ifndef OBJECTMGR_H
#define OBJECTMGR_H

#include <map>
#include <set>
#include <list>

#include "LayerMgr.h"

class BaseObject;
class PhysicsMgr;
class AppFalconGame;

typedef std::map<uint32, BaseObject*> ObjectMap;
typedef std::set<BaseObject*> BaseObjectSet;
typedef std::set<Object*> ObjectSet;


class ObjectMgr
{
public:
    ObjectMgr(Engine *e);
    ~ObjectMgr();
    uint32 Add(BaseObject*);
    BaseObject *Get(uint32 id);
    inline uint32 GetLastId(void) const { return _curId; }
    inline uint32 GetCount(void) const { return _store.size(); }
    void Update(float dt, uint32 frametime);
    void RenderLayer(uint32 id);
    void RenderBBoxes(void); // debug function
    void FlagForRemove(BaseObject *obj);
    
    void RemoveAll(void);

    void GetAllObjectsIn(BaseRect& rect, BaseObjectSet& result) const;
    const ObjectMap& GetAllObjects(void) const { return _store; }

    inline void SetPhysicsMgr(PhysicsMgr *pm) { _physMgr = pm; }
    inline void SetLayerMgr(LayerMgr *layers) {_layerMgr = layers; }

    void dbg_setcoll(bool b);

protected:
    ObjectMap::iterator _Remove(uint32 id);

    uint32 _curId;
    ObjectMap _store;
    PhysicsMgr *_physMgr;
    LayerMgr *_layerMgr;
    Engine *_engine;
    ObjectSet _renderLayers[LAYER_MAX];

};

#endif
