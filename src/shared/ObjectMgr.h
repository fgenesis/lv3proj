#ifndef OBJECTMGR_H
#define OBJECTMGR_H

#include <map>
#include <set>

#include "LayerMgr.h"

class BaseObject;
class PhysicsMgr;
class AppFalconGame;

typedef std::map<uint32, BaseObject*> ObjectMap;
typedef std::set<Object*> ObjectSet;


class ObjectMgr
{
public:
    ObjectMgr(Engine *e);
    ~ObjectMgr();
    uint32 Add(BaseObject*);
    BaseObject *Get(uint32 id);
    void Update(uint32 ms);
    void RenderLayer(uint32 id);
    void FlagForRemove(BaseObject *obj);
    ObjectMap::iterator GetIterator(uint32 id);
    ObjectMap::iterator Remove(uint32 id);
    void RemoveAll(void);

    inline void SetPhysicsMgr(PhysicsMgr *pm) { _physMgr = pm; }
    inline void SetLayerMgr(LayerMgr *layers) {_layerMgr = layers; }

protected:
    uint32 _curId;
    ObjectMap _store;
    PhysicsMgr *_physMgr;
    LayerMgr *_layerMgr;
    Engine *_engine;
    ObjectSet _renderLayers[LAYER_MAX];

};

#endif
