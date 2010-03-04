#ifndef TILELAYER_H
#define TILELAYER_H

#include <set>
#include "array2d.h"

struct SDL_Surface;
struct SDL_Rect;
struct AnimatedTile;
struct BasicTile;

typedef std::set<AnimatedTile*> AnimTileSet;


class TileLayer
{
    friend class LayerMgr;

public:
    void Update(uint32 curtime);
    void Render(void);
    void SetTile(uint32 x, uint32 y, BasicTile *tile, bool updateCollision = true);
    inline BasicTile *GetTile(uint32 x, uint32 y) { return tilearray(x,y); }
    inline uint32 GetArraySize(void) { return tilearray.size1d(); }
    inline uint32 GetPixelSize(void) { return GetArraySize() * 16; }

    bool visible;
    bool collision; // do collision checking against this layer for non-transparent areas
    uint32 xoffs;
    uint32 yoffs;

protected:
    SDL_Surface *target; // where to render to - should be set to Engine::GetSurface()
    SDL_Rect *visible_area; // what to render - Engine::GetVisibleBlockRect()
    array2d<BasicTile*> tilearray;
    AnimTileSet tileset;
    uint32 used; // amount of used tiles - if 0 Update() and Render() are skipped. Counted in SetTile()
    LayerMgr *mgr; // ptr to layer mgr - this is needed for collision map (re-)calculation
};


#endif
