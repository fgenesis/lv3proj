#include "common.h"
#include "Engine.h"
#include "Tile.h"


AnimatedTile::AnimatedTile()
: nextupdate(0), curFrameIdx(0)
{
     surface = NULL;
     type = TILETYPE_ANIMATED;
}

AnimatedTile::AnimatedTile(Anim *a, const char *startwith /* = NULL*/)
: nextupdate(0), curFrameIdx(0)
{
    surface = NULL;
    type = TILETYPE_ANIMATED;
    ani = a;
    ASSERT(ani);
    SetupDefaults(startwith);
}

void AnimatedTile::SetFrame(uint32 frame)
{
    curFrame = &(curFrameStore->store[frame]);
    nextupdate = Engine::GetCurFrameTime() + curFrame->frametime;
    curFrameIdx = frame;
    surface = curFrame->surface;
}

void AnimatedTile::SetName(const char *name)
{
    // only change frame if the name really changes
    if(curFrameStore->name == name)
        return;

    AnimMap::iterator am = ani->anims.find(name);
    if(am != ani->anims.end())
    {
        curFrameStore = &(am->second);
        SetFrame(0);
    }
    else
    {
        logerror("AnimatedTile::SetName(%s) - name not found in '%s'", name, ani->filename.c_str());
    }
}

void AnimatedTile::SetupDefaults(const char *startwith /* = NULL*/)
{
    DEBUG(ASSERT(curFrameIdx == 0)); // this could as well be set to 0 here, but this is to check if the code is correct
    DEBUG(ASSERT(ani->first)); // the parser must set this
    curFrameStore = ani->first;
    if(startwith)
    {
        AnimMap::iterator am = ani->anims.find(startwith);
        if(am != ani->anims.end())
            am = ani->anims.begin();
    }
    curFrame = &(curFrameStore->store[0]);
}

void AnimatedTile::Init(uint32 curtime)
{
    //nextupdate = curtime + curFrame->frametime;
    //surface = curFrame->surface;
    SetFrame(0);
}

// TODO: smoothen animation, compensate lost frames
void AnimatedTile::Update(uint32 curtime)
{
    if(nextupdate < curtime)
    {
        uint32 diff = curtime - nextupdate;
        ++curFrameIdx;
        if(curFrameIdx >= curFrameStore->store.size())
        {
            switch(curFrameStore->cmd.action)
            {
                case ANIMCMD_REPEAT:
                    SetFrame(0);
                    break;

                case ANIMCMD_JUMP:
                    SetName(curFrameStore->cmd.param.c_str());
                    break;

                default:
                    ASSERT(false);
            }
        }
        else
            SetFrame(curFrameIdx);
        //nextupdate = (curtime + curFrame->frametime) - diff;
    }
}
