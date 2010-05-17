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
typedef std::set<Object*> ObjectSet;
typedef std::set<std::pair<BaseObject*,uint8> > ObjectWithSideSet;


class ObjectMgr
{
public:
    ObjectMgr(Engine *e);
    ~ObjectMgr();
    uint32 Add(BaseObject*);
    BaseObject *Get(uint32 id);
    inline uint32 GetLastId(void) { return _curId; }
    void Update(uint32 ms);
    void RenderLayer(uint32 id);
    void RenderBBoxes(void); // debug function
    void FlagForRemove(BaseObject *obj);
    ObjectMap::iterator GetIterator(uint32 id);
    ObjectMap::iterator Remove(uint32 id);
    void RemoveAll(void);
    void HandleObjectCollision(ActiveRect *base, ActiveRect *other, uint8 side);

    void GetAllObjectsIn(BaseRect& rect, ObjectWithSideSet& result, uint8 force_side = SIDE_NONE);

    inline void SetPhysicsMgr(PhysicsMgr *pm) { _physMgr = pm; }
    inline void SetLayerMgr(LayerMgr *layers) {_layerMgr = layers; }

    void dbg_setcoll(bool b);

protected:
    uint32 _curId;
    ObjectMap _store;
    PhysicsMgr *_physMgr;
    LayerMgr *_layerMgr;
    Engine *_engine;
    ObjectSet _renderLayers[LAYER_MAX];

};

#endif
