#ifndef LAYERMGR_H
#define LAYERMGR_H

#include "array2d.h"
#include "BitSet2d.h"
#include "Tile.h"
#include "TileLayer.h"
#include "SharedStructs.h"
#include "Vector2d.h"

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
    void SetLayer(TileLayer *layer, uint32 depth);

    void Update(uint32 curtime);
    void Render(void);
    void Clear(void);

    void SetMaxDim(uint32 dim); // set x and y size of all layers & collision map + resize if necessary
    void SetRenderOffset(int32 x, int32 y);
    inline uint32 GetMaxDim(void) const { return _maxdim; }
    inline uint32 GetMaxPixelDim(void)  const { return _maxdim * 16; } // TODO: FIXME for tile sizes != 16

    // TODO: is the info layer really needed?
    void CreateInfoLayer(void);
    inline void SetInfoLayer(uint16 *ti) { _infoLayer.setPtr(ti); }
    inline uint16 *GetInfoLayer(void) { return _infoLayer.getPtr(); }
    inline uint16 GetTileInfo(uint32 x, uint32 y) { return _infoLayer(x,y); }
    inline void SetTileInfo(uint32 x, uint32 y, uint16 info) { _infoLayer(x,y) = info; }

    inline bool HasCollisionMap(void) const { return _collisionMap.size1d(); }
    inline const CollisionMap& GetCollisionMap(void) const { return _collisionMap; }
    void CreateCollisionMap(void); // create new collision map (and delete old if exists)
    void UpdateCollisionMap(uint32 x, uint32 y); // recalculates the collision map at a specific tile
    void UpdateCollisionMap(void); // recalculates the *whole* collision map - use rarely!
    void UpdateCollisionMap(Object *obj); // uses LCF_BLOCKING_OBJECT to mark the collision map
    void RemoveFromCollisionMap(Object *obj);
    bool CollisionWith(const BaseRect *rect, int32 skip = 1, uint8 flags = LCF_ALL) const; // check if a rectangle overlaps with at least one solid pixel in our collision map.
    
    bool CastRayAbs(const Vector2di src, const Vector2di& targ, Vector2di *lastpos, Vector2di *collpos, LayerCollisionFlag lcf = LCF_ALL) const;
    bool CastRayDir(const Vector2di src, const Vector2di& dir, Vector2di *lastpos, Vector2di *collpos, LayerCollisionFlag lcf = LCF_ALL) const;
    bool CastRaysFromRect(const BaseRect& src, const Vector2di& dir, Vector2di *lastpos, Vector2di *collpos,
        LayerCollisionFlag lcf = LCF_ALL) const;

    
    
    // when calling this function, we assume there is NO collision yet (check new position with CollisionWith() before!)
    void LoadAsciiLevel(AsciiLevel *level); // TODO: obsolete, remove as soon as native map files can be loaded

    std::map<std::string, std::string> stringdata; // stores arbitrary content, to be used in scripts or so. // TODO: add documentation

private:
    Engine *_engine;
    TileLayer *_layers[LAYER_MAX];
    TileInfoLayer _infoLayer; // TODO: deprecate, or make something useful with this
    CollisionMap _collisionMap;
    uint32 _maxdim; // max dimension for all created layers

};

#endif
