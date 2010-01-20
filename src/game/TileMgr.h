#ifndef TILEMGR_H
#define TILEMGR_H

#include "array2d.h"
#include "Tile.h"

class Engine;
struct SDL_Surface;

class TileMgr
{
public:

    TileMgr(Engine*);
    ~TileMgr();

    inline void SetStaticTileSurface(uint32 x, uint32 y, SDL_Surface *s)
    {
        staticTiles(x,y) = s;
    }
    inline void SetAnimatedTileSurface(uint32 x, uint32 y, AnimatedTile *t)
    {
        animTiles(x,y) = t;
    }

    void RenderStaticTiles(void);
    void RenderAnimatedTiles(void);
    void HandleAnimation(uint32 ms);


private:
    Engine *engine;
    array2d<SDL_Surface*> staticTiles;
    array2d<AnimatedTile*> animTiles;
};

#endif