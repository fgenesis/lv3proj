#ifndef TILE_H
#define TILE_H

#include "Anim.h"

enum TileType
{
    TILETYPE_STATIC,
    TILETYPE_ANIMATED
};


// the basic tile. static by default, but can be overloaded to support animation
struct BasicTile
{
public:
    BasicTile() : surface(NULL), type(TILETYPE_STATIC) {}
    SDL_Surface *surface; // surface to be drawn
    uint8 type; // read-only!!
};


struct AnimatedTile : BasicTile
{
public:
    AnimatedTile();
    AnimatedTile(Anim *a, uint32 idx = 0, const char *startwith = NULL);
    Anim *ani;
    AnimFrame *curFrame;
    AnimFrameStore *curFrameStore;
    uint32 nextupdate; // the time when this tile will change its texture (Engine::GetCurrentFrameTime() + X)

    void SetupDefaults(uint32 idx = 0, const char *startwith = NULL);
    void Init(uint32 t); // current system clock
    static void SplitFilenameToProps(const char *in, std::string *fn, uint32 *idx, std::string *str);
};

#endif
