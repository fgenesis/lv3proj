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
    nextupdate = 0; // TODO: THIS IS A DIRTY HACK!!! (forces immediate update, but skips a frame
}

void AnimatedTile::SetName(char *name)
{
    AnimMap::iterator am = ani->anims.find(name);
    if(am != ani->anims.end())
    {
        curFrameStore = &(am->second);
        SetFrame(0);
    }
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

void AnimatedTile::Init(uint32 curtime)
{
     nextupdate = curtime + curFrame->frametime;
     surface = curFrame->surface;
}

// TODO: smoothen animation, compensate lost frames
void AnimatedTile::Update(uint32 curtime)
{
    if(nextupdate < curtime)
    {
        if(curFrame->nextframe)
        {
            curFrame = &(curFrameStore->store[curFrame->nextframe - 1]);
            surface = curFrame->surface;
        }
        else if(curFrame->nextanim.length())
        {
            curFrameStore = &(ani->anims[curFrame->nextanim]); // <-- TODO: this call could be precalculated, maybe (eats CPU)
            curFrame = &(curFrameStore->store[0]);
            surface = curFrame->surface;
        }
        nextupdate = curtime + curFrame->frametime;
    }
}
