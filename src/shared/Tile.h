#ifndef TILE_H
#define TILE_H

#include "Anim.h"

enum TileType
{
    TILETYPE_STATIC,
    TILETYPE_ANIMATED
};


// TODO: this has to be extended & implemented. Add TileClass enum with custom values
enum TileFlags
{
    TILEFLAG_DEFAULT = 0x0000,
    // ...free to use...
    TILEFLAG_RES1    = 0x2000, // not yet used, reserved
    TILEFLAG_RES2    = 0x4000, // not yet used, reserved
    TILEFLAG_SOLID   = 0x8000, // the whole tile is solid, even if there are transparent pixels
}; // 16 bit enum


// the basic tile. static by default, but can be overloaded to support animation
struct BasicTile
{
public:
    BasicTile() : surface(NULL), type(TILETYPE_STATIC) {}
    std::string filename;
    SDL_Surface *surface; // surface to be drawn
    uint8 type; // read-only!!

};


struct AnimatedTile : BasicTile
{
public:
    AnimatedTile();
    AnimatedTile(Anim *a, const char *startwith = NULL);

    void SetupDefaults(const char *startwith = NULL);
    void SetName(const char *name);
    void SetFrame(uint32 frame);
    const char *GetName(void) { return curFrameStore->name.c_str(); }
    uint32 GetFrame(void) { return curFrameIdx; }
    void Init(uint32 curtime); // current system clock
    void Update(uint32 curtime);

private:
    Anim *ani;
    uint32 nextupdate; // the time when this tile will change its texture (Engine::GetCurrentFrameTime() + X)
    uint32 curFrameIdx;
    AnimFrame *curFrame;
    AnimFrameStore *curFrameStore;

};

#endif
