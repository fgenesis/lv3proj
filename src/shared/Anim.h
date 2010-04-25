#ifndef ANIM_H
#define ANIM_H

#include <map>
#include <vector>

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

struct AnimFrame
{
    AnimFrame() : surface(NULL) {}
    AnimFrame(std::string& fn, uint16 t) : surface(NULL), filename(fn), frametime(t) {}
    std::string filename;
    uint16 frametime;
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
