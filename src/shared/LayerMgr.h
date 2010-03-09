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

enum LayerDepth
{
    LAYER_REARMOST_BACKGROUND = 0,
    LAYER_DEFAULT_ENV = 6, // where most walls and basic stuff should be put. the editor will start with this layer.
    LAYER_DEFAULT_SPRITES = 15, // the default sprite layer. should not contain
                        // anything else than sprites, because these have to be treated specially
    LAYER_FOREMOST_OVERLAY = 31,

    LAYER_MAX = 32
};


class LayerMgr
{
public:

    LayerMgr(Engine*);
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

    void CreateCollisionMap(void); // create new collision map (and delete old if exists)
    void UpdateCollisionMap(uint32 x, uint32 y); // recalculates the collision map at a specific tile
    void UpdateCollisionMap(void); // recalculates the *whole* collision map - use rarely!
    bool CollisionWith(ActiveRect *rect, int32 skip = 4); // check if an ActiveRect overlaps with at least one solid pixel in our collision map.
    // when calling this function, we assume there is NO collision yet (check new position with CollisionWith() before!)
    Point GetClosestNonCollidingPoint(ActiveRect *rect, uint8 direction, int32 skip = 1);
    bool CanFallDown(Point anchor, uint32 arealen);
    bool LoadAsciiLevel(AsciiLevel *level);


private:
    Engine *engine;
    TileLayer *_layers[LAYER_MAX];
    BitSet2d *_collisionMap;
    uint32 _maxdim; // max dimension for all created layers

};

#endif
