#include "common.h"
#include "Engine.h"
#include "Tile.h"

BasicTile::~BasicTile()
{
    // a basic tile has a surface it carries around all the time,
    // but in an animated tile, <surface> just points to a surface in its Anim ptr, so it does not need to be dropped!
    if(type == TILETYPE_STATIC)
        resMgr.Drop(surface);
}

AnimatedTile::~AnimatedTile()
{
    DEBUG(ASSERT(type == TILETYPE_ANIMATED));
    resMgr.Drop(ani);
}

BasicTile *BasicTile::_New(const char *filename)
{
    if(SDL_Surface *img = resMgr.LoadImg((char*)filename))
        return new BasicTile(img, filename);

    return NULL;
}

BasicTile *AnimatedTile::New(const char *filename)
{
    std::string ext(FileGetExtension(filename));
    if(ext == ".anim")
    {
        if(Anim *ani = resMgr.LoadAnim((char*)filename))
        {
            AnimatedTile *tile = new AnimatedTile(ani);
            tile->filename = filename;
            tile->Init(Engine::GetCurFrameTime());

            return tile;
        }

        return NULL;
    }

    return BasicTile::_New(filename);
}


AnimatedTile::AnimatedTile(Anim *a, const char *startwith /* = NULL*/)
: BasicTile(NULL, a->filename.c_str()), nextupdate(0), curFrameIdx(0)
{
    type = TILETYPE_ANIMATED;
    ani = a;
    SetupDefaults(startwith);
}

void AnimatedTile::SetFrame(uint32 frame)
{
    DEBUG(ASSERT(frame < curFrameStore->store.size()));
    curFrame = &(curFrameStore->store[frame]);
    nextupdate = Engine::GetCurFrameTime() + curFrame->frametime;
    curFrameIdx = frame;
    surface = curFrame->GetSurface();
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

        // list of frames is empty, but there may be a jump command to another frame
        if(curFrameStore->store.empty())
        {
            if(curFrameStore->cmd.action == ANIMCMD_JUMP)
                SetName(curFrameStore->cmd.param.c_str());
            else
                logerror("Tile '%s' framename '%s' does not contain any frames!", GetFilename(), name); 
            return;
        }

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
