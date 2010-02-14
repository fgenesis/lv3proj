#include "common.h"
#include "Tile.h"

AnimatedTile::AnimatedTile()
{
     surface = NULL;
     type = TILETYPE_ANIMATED;
}

AnimatedTile::AnimatedTile(Anim *a, uint32 idx /* = 0*/, const char *startwith /* = NULL*/)
: nextupdate(0)
{
    surface = NULL;
    type = TILETYPE_ANIMATED;
    ani = a;
    ASSERT(ani);
    SetupDefaults(idx, startwith);
}

void AnimatedTile::SetFrame(uint32 frame)
{
    curFrame = &(curFrameStore->store[frame % curFrameStore->store.size()]);
}

void AnimatedTile::SetName(char *name)
{
    AnimMap::iterator am = ani->anims.find(name);
    if(am != ani->anims.end())
        curFrameStore = &(am->second);
}

void AnimatedTile::SetupDefaults(uint32 idx /* = 0*/, const char *startwith /* = NULL*/)
{
    AnimMap::iterator am = ani->anims.find(startwith ? startwith : "default");
    if(am == ani->anims.end())
    {
        am = ani->anims.find("stand");
        if(am == ani->anims.end())
            am = ani->anims.begin(); // FALLBACK // TODO: is this necessary?
    }
    curFrameStore = &(am->second);
    curFrame = &(curFrameStore->store[idx]);
}

void AnimatedTile::Init(uint32 t)
{
     nextupdate = t + curFrame->frametime;
     surface = curFrame->surface;
}



