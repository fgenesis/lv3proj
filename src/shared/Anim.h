#ifndef ANIM_H
#define ANIM_H

#include <map>
#include <vector>

struct SDL_Surface;

struct AnimFrame
{
    AnimFrame() : surface(NULL) {}
    uint16 index;
    std::string filename;
    uint16 frametime;
    uint16 nextframe;
    std::string nextanim; // this is used if nextframe == 0
    SDL_Surface *surface;
};

typedef std::vector<AnimFrame> AnimFrameStore;
typedef std::map<std::string, AnimFrameStore> AnimMap;

struct Anim
{
    AnimMap anims;

};

#endif
