#ifndef TILE_H
#define TILE_H

#include "Anim.h"


// there is no need for a "normal" tile, we just use a SDL_Surface instead

struct AnimatedTile
{
    AnimatedTile() : surface(NULL) {}
    AnimatedTile(char *afile);
    Anim *ani;
    AnimFrame *curFrame;
    AnimFrameStore *curFrameStore;
    SDL_Surface *surface; // currently active surface
    uint32 timeleft; // the time until this tile will change its texture

    void SetupDefaults(void)
    {
        AnimMap::iterator am = ani->anims.find("default");
        if(am == ani->anims.end())
        {
            am = ani->anims.find("stand");
            if(am == ani->anims.end())
                am = ani->anims.begin(); // FALLBACK // TODO: is this necessary?
        }
        curFrameStore = &(am->second);
        curFrame = &((*curFrameStore)[0]);
        timeleft = curFrame->frametime;
    }
};

#endif
