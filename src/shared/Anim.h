#ifndef ANIM_H
#define ANIM_H

#include <map>
#include <vector>
#include "ResourceMgr.h" // required for ResourceCallback<>

struct SDL_Surface;

enum AnimCmdEnum
{
    ANIMCMD_REPEAT, // set index back to 0 and repeat current sub-anim
    ANIMCMD_JUMP // jump to sub-anim with given name
};

struct AnimFrameCmd
{
    uint8 action;
    std::string param;
};

class AnimFrame
{
    friend class ResourceMgr;

public:
    inline const char *GetFilename(void) { return filename.c_str(); }
    inline SDL_Surface *GetSurface(void) { return surface; }
    uint16 frametime;
    AnimFrame() : surface(NULL) {}
    AnimFrame(std::string& fn, uint16 t) : surface(NULL), filename(fn), frametime(t) {}
protected:

    std::string filename;
    ResourceCallback<SDL_Surface> callback;

    SDL_Surface *surface;
};

typedef std::vector<AnimFrame> AnimFrameVector;

struct AnimFrameStore
{
    std::string name;
    AnimFrameVector store;
    AnimFrameCmd cmd; // what do do after the animation is played
};

typedef std::map<std::string, AnimFrameStore> AnimMap;

struct Anim
{
    AnimMap anims;
    AnimFrameStore *first;
    std::string filename;
};

#endif
