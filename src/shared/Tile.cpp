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

// splits string like "filename.anim:4:default". leaves unused args unchanged!
void AnimatedTile::SplitFilenameToProps(const char *in, std::string *fn, uint32 *idx, std::string *str)
{
    std::vector<std::string> fields;
    StrSplit(in, ":", fields, true);
    if(fields.size() >= 1 && fn)
        *fn = fields[0];
    if(fields.size() >= 2 && idx)
        *idx = atoi(fields[1].c_str());
    if(fields.size() >= 3 && str)
        *str = fields[2];


    /*
    char *nextpos;
    nextpos = strchr(in, ':');
    if(!nextpos)
    {
        *fn = in;
        return;
    }
    else
    {
        fn->append(in, nextpos - in);
    }
    if(nextpos)
    {
        if(idx)
        {
            *idx = atoi(nextpos + 1);
        }
        if(str)
        {
            nextpos = strchr(nextpos + 1, ':');
            if(nextpos)
                *str = nextpos + 1;
        }
    }
    */
}


