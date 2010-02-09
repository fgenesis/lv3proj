#ifndef TILELAYER_H
#define TILELAYER_H

#include <set>
#include "array2d.h"

struct SDL_Surface;
struct SDL_Rect;
struct AnimatedTile;
struct BasicTile;

typedef std::set<AnimatedTile*> AnimTileSet;

enum LayerType
{
    LAYERTYPE_BASE,
    LAYERTYPE_TILED,
    LAYERTYPE_ANIMATED
};


// TileLayerBase is best suited for surfaces that have to be rendered once and stay the same
// all the time after beeing rendered. Means they can just be blitted on the screen.
class TileLayerBase
{
    friend class LayerMgr;

public:
    ~TileLayerBase();
    LayerType ty;
    SDL_Surface *surface; // for direct access - should be created using SDL_CreateSurfceRGB()
    SDL_Surface *target; // where to render to - should be set to Engine::GetSurface()
    SDL_Rect *visible_area; // what to render - Engine::GetVisibleBlockRect()
    virtual void Update(uint32 curtime) {}
    virtual void Render(void);
    bool collision; // do collision checking against this layer for non-transparent areas
    bool visible;
    uint32 xoffs;
    uint32 yoffs;
};

// TileLayerArray2d should be used for static tiles that do not need animation.
// Only the parts that are visible on the screen will be rendered and updated,
// and only if needed (if something is changed). otherwise, everything is rendered once into <surface>
// and blitted to the screen like TileLayerBase does.
class TileLayerArray2d : public TileLayerBase
{
    friend class LayerMgr;

public:
    virtual void Update(uint32 curtime) {}
    virtual void Render(void);
    virtual void SetTile(uint32 x, uint32 y, BasicTile *tile);
    virtual BasicTile *GetTile(uint32 x, uint32 y);
    virtual uint32 GetArraySize(void) { return tilearray.size1d(); }
    virtual uint32 GetPixelSize(void) { return GetArraySize() * 16; }

protected:
    array2d<BasicTile*> tilearray;
    bool _update;
};

// The generic TileLayer should be used for animated tiles that have to be touched every cycle.
// Using an array2d for this would cause performance loss, so we use a populated std::set.
// <surface> is not used, everything is rendered directly to the target (usually the screen)
class TileLayer : public TileLayerArray2d
{
    friend class LayerMgr;

public:
    virtual void Update(uint32 curtime);
    virtual void Render(void);
    virtual void SetTile(uint32 x, uint32 y, BasicTile *ani);

protected:
    AnimTileSet tileset;
};


#endif
