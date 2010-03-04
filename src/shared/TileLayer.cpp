#include <SDL/SDL.h>

#include "common.h"
#include "Tile.h"
#include "TileLayer.h"
#include "LayerMgr.h"


// Puts a tile to location (x,y). Set tile to NULL to remove current tile.
void TileLayer::SetTile(uint32 x, uint32 y, BasicTile *tile, bool updateCollision /* = true */)
{
    if(x >= tilearray.size1d() || y >= tilearray.size1d())
        return;

    BasicTile *& tileref = tilearray(x,y);
    if(tileref == tile)
        return;

    // insert only animated tiles into the animated tile store
    if(tile && tile->type == TILETYPE_ANIMATED)
    {
        AnimTileSet::iterator it = tileset.find((AnimatedTile*)tileref);
        if(it != tileset.end())
            tileset.erase(it);

        tileset.insert((AnimatedTile*)tile);
    }

    // update amount of used tiles
    if(tileref && !tile)
        --used;
    else if(!tileref && tile)
        ++used;

    tileref = tile;

    // if this tile is relevant for collision detection, update collision map at this pos
    if(updateCollision && collision && mgr)
        mgr->UpdateCollisionMap(x,y);
}

void TileLayer::Update(uint32 curtime)
{
    if(!used) // no tiles there, nothing to do
        return;

    for(AnimTileSet::iterator it = tileset.begin(); it != tileset.end(); it++)
    {
        AnimatedTile *& tile = *it;
        if(tile->nextupdate < curtime)
        {
            if(tile->curFrame->nextframe)
            {
                tile->curFrame = &(tile->curFrameStore->store[tile->curFrame->nextframe - 1]);
                tile->surface = tile->curFrame->surface;
            }
            else if(tile->curFrame->nextanim.length())
            {
                tile->curFrameStore = &(tile->ani->anims[tile->curFrame->nextanim]); // <-- TODO: this call could be precalculated, maybe (eats CPU)
                tile->curFrame = &(tile->curFrameStore->store[0]);
                tile->surface = tile->curFrame->surface;
            }
            tile->nextupdate = curtime + tile->curFrame->frametime;
        }
    }
}

void TileLayer::Render(void)
{
    if( !(visible && used) )
        return;

    SDL_Rect rect;
    SDL_Rect blockrect = *visible_area;
    blockrect.w = std::min(uint32(blockrect.w + blockrect.x), tilearray.size1d()); // use absolute values, this is right x point now
    blockrect.h = std::min(uint32(blockrect.h + blockrect.y), tilearray.size1d()); // now bottom y point

    for(uint32 y = blockrect.y; y < blockrect.h; ++y)
    {
        for(uint32 x = blockrect.x; x < blockrect.w; ++x)
        {
            BasicTile *& tile = tilearray(x,y);
            if(!tile)
                continue;

            rect.x = (x << 4) + xoffs; // x * 16
            rect.y = (y << 4) + yoffs; // y * 16

            SDL_BlitSurface(tile->surface, NULL, target, &rect);
        }
    }
}