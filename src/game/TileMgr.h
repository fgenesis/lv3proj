#ifndef TILEMGR_H
#define TILEMGR_H

#include "array2d.h"
#include "Tile.h"

class Engine;
struct SDL_Surface;
struct AsciiLevel;

class TileMgr
{
public:

    TileMgr(Engine*);
    ~TileMgr();

    inline void SetStaticTileSurface(uint32 x, uint32 y, SDL_Surface *s)
    {
        SDL_Surface *& tileref = staticTiles(x,y);
        if(tileref != s)
            _staticSurfaceNeedsUpdate = true;
        tileref = s;
    }
    inline void SetAnimatedTileSurface(uint32 x, uint32 y, AnimatedTile *t)
    {
        animTiles(x,y) = t;
    }

    inline SDL_Surface *GetStaticTile(uint32 x, uint32 y) { return staticTiles(x,y); }
    inline SDL_Surface *GetTile(uint32 x, uint32 y) { return staticTiles(x,y); }

    void InitStaticSurface(void);
    void RenderStaticTiles(void);
    void RenderAnimatedTiles(void);
    void HandleAnimation(void);

    bool LoadAsciiLevel(AsciiLevel *level);


private:
    SDL_Rect _GetVisibleBlockRect(void);
    Engine *engine;
    SDL_Surface *_staticSurface;
    array2d<SDL_Surface*> staticTiles;
    array2d<AnimatedTile*> animTiles;
    bool _staticSurfaceNeedsUpdate;
};

#endif
