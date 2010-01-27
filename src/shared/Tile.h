#ifndef TILE_H
#define TILE_H

#include "Anim.h"


// there is no need for a "normal" tile, we just use a SDL_Surface instead

struct AnimatedTile
{
    AnimatedTile() : surface(NULL) {}
    AnimatedTile(Anim *a, uint32 idx = 0, const char *startwith = NULL);
    Anim *ani;
    AnimFrame *curFrame;
    AnimFrameStore *curFrameStore;
    SDL_Surface *surface; // currently active surface
    uint32 nextupdate; // the time when this tile will change its texture (Engine::GetCurrentFrameTime() + X)

    void SetupDefaults(uint32 idx = 0, const char *startwith = NULL);
    void Init(uint32 t); // current system clock
    static void SplitFilenameToProps(const char *in, std::string *fn, uint32 *idx, std::string *str);
};

#endif
