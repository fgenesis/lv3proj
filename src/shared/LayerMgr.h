#ifndef LAYERMGR_H
#define LAYERMGR_H

#include "array2d.h"
#include "BitSet2d.h"
#include "Tile.h"
#include "TileLayer.h"
#include "SharedStructs.h"

class Engine;
struct SDL_Surface;
struct AsciiLevel;
class ActiveRect;
class Object;

enum LayerDepth
{
    LAYER_REARMOST_BACKGROUND = 0,
    LAYER_FOREMOST_OVERLAY = 31,

    LAYER_MAX = 32 // do not use
};

enum LayerCollisionFlag
{
    LCF_NONE = 0x00,
    LCF_WALL = 0x01,
    LCF_BLOCKING_OBJECT = 0x02,

    LCF_ALL = 0xFF
};

typedef array2d<uint16> TileInfoLayer;
typedef array2d<uint8, true> CollisionMap;


class LayerMgr
{
public:

    LayerMgr(Engine *e);
    ~LayerMgr();


    inline TileLayer *GetLayer(uint32 depth) { return _layers[depth]; }
    TileLayer *CreateLayer(bool collision = false, uint32 xoffs = 0, uint32 yoffs = 0);
    inline void SetLayer(TileLayer *layer, uint32 depth)
    {
        _layers[depth] = layer;
    }

    void Update(uint32 curtime);
    void Render(void);
    void Clear(void);

    inline void SetMaxDim(uint32 dim) { _maxdim = dim; }
    inline uint32 GetMaxDim(void) { return _maxdim; }
    inline uint32 GetMaxPixelDim(void) { return _maxdim * 16; }
    void CreateInfoLayer(void);
    inline void SetInfoLayer(uint16 *ti) { _infoLayer.setPtr(ti); }
    inline uint16 *GetInfoLayer(void) { return _infoLayer.getPtr(); }
    inline uint16 GetTileInfo(uint32 x, uint32 y) { return _infoLayer(x,y); }
    inline void SetTileInfo(uint32 x, uint32 y, uint16 info) { _infoLayer(x,y) = info; }

    inline bool HasCollisionMap(void) { return _collisionMap.size1d(); }
    void CreateCollisionMap(void); // create new collision map (and delete old if exists)
    void UpdateCollisionMap(uint32 x, uint32 y); // recalculates the collision map at a specific tile
    void UpdateCollisionMap(void); // recalculates the *whole* collision map - use rarely!
    void UpdateCollisionMap(Object *obj); // uses LCF_BLOCKING_OBJECT to mark the collision map
    void RemoveFromCollisionMap(Object *obj);
    bool CollisionWith(BaseRect *rect, int32 skip = 4, uint8 flags = LCF_ALL); // check if a rectangle overlaps with at least one solid pixel in our collision map.
    // when calling this function, we assume there is NO collision yet (check new position with CollisionWith() before!)
    Point GetNonCollidingPoint(BaseRect *rect, uint8 direction, uint32 maxdist = -1);
    uint32 CanMoveToDirection(BaseRect *rect, uint8 direction, uint32 pixels = 1); // returns the amount of pixels until the object hits the wall, up to [pixels]
    uint32 CanMoveToDirection(BaseRect *rect, MovementDirectionInfo& mdi, uint32 pixels = 1);
    bool CanFallDown(Point anchor, uint32 arealen);
    void LoadAsciiLevel(AsciiLevel *level);


private:
    Engine *engine;
    TileLayer *_layers[LAYER_MAX];
    TileInfoLayer _infoLayer;
    CollisionMap _collisionMap;
    uint32 _maxdim; // max dimension for all created layers

};

#endif
