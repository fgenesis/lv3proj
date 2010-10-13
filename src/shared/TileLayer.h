#ifndef TILELAYER_H
#define TILELAYER_H

#include <map>
#include "array2d.h"

struct SDL_Surface;
struct SDL_Rect;
class AnimatedTile;
class BasicTile;
class LayerMgr;
struct Point;

typedef std::map<AnimatedTile*, uint32> AnimTileMap;


class TileLayer
{
    friend class LayerMgr;

public:
    TileLayer();
    ~TileLayer();
    void Update(uint32 curtime);
    void Render(void);
    void SetTile(uint32 x, uint32 y, BasicTile *tile, bool updateCollision = true);
    inline BasicTile *GetTile(uint32 x, uint32 y) { return tilearray(x,y); }
    inline uint32 GetArraySize(void) { return tilearray.size1d(); }
    inline uint32 GetPixelSize(void) { return GetArraySize() * 16; }
    inline bool IsUsed(void) { return used; }
    inline uint32 UsedTiles(void) { return used; }
    void Resize(uint32 dim); // do not use this for layers stored in the LayerMgr!

    bool visible;
    bool collision; // do collision checking against this layer for non-transparent areas
    SDL_Rect *visible_area; // what to render - Engine::GetVisibleBlockRect()
    Point *camera;
    SDL_Surface *target; // where to render to - should be set to Engine::GetSurface(). set visible=false if this is NULL!
    uint32 xoffs;
    uint32 yoffs;
    std::string name;

protected:
    array2d<BasicTile*> tilearray;
    AnimTileMap tilemap;
    uint32 used; // amount of used tiles - if 0 Update() and Render() are skipped. Counted in SetTile()
    LayerMgr *mgr; // ptr to layer mgr - this is needed for collision map (re-)calculation
};


#endif
