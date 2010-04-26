#include <SDL/SDL.h>

#include "common.h"
#include "Tile.h"
#include "TileLayer.h"
#include "LayerMgr.h"

// upon deletion, update refcount of all tiles, this ensures proper resource cleanup.
TileLayer::~TileLayer()
{
    for(uint32 y = 0; y < tilearray.size1d(); ++y)
        for(uint32 x = 0; x < tilearray.size1d(); ++x)
            if(BasicTile *tile = tilearray(x,y))
                tile->ref--;
}

// Puts a tile to location (x,y). Set tile to NULL to remove current tile.
void TileLayer::SetTile(uint32 x, uint32 y, BasicTile *tile, bool updateCollision /* = true */)
{
    if(x >= tilearray.size1d() || y >= tilearray.size1d())
        return;

    BasicTile *& tileref = tilearray(x,y);
    if(tileref == tile)
        return;

    // insert only animated tiles into the animated tile store
    if(tile && tile->GetType() == TILETYPE_ANIMATED)
    {
        AnimTileMap::iterator it = tilemap.find((AnimatedTile*)tile);
        if(it != tilemap.end())
            it->second++; // tile is used multiple times, increase count
        else
            tilemap[(AnimatedTile*)tile] = 1; // first time added, count must be 1
    }

    // update amount of used tiles and drop the old one out of the tilemap, if required
    if(tileref)
    {
        if(tileref->GetType() == TILETYPE_ANIMATED)
        {
            AnimTileMap::iterator it = tilemap.find((AnimatedTile*)tileref);
            DEBUG(ASSERT(it != tilemap.end())); // it MUST have been added before
            if(it != tilemap.end())
            {
                --(it->second);
                if(!it->second)
                    tilemap.erase(it);
            }
        }

        tileref->ref--; // will be overwritten, decr ref
        if(!tile)
            --used; // we have a tile currently, but setting NULL -> one tile less used
    }
    else if(tile) // tileref is NULL, but we are setting a tile -> one more tile used
    {
        ++used;
    }

    if(tile) // we will use this, incr ref
        tile->ref++;

    tileref = tile;

    // if this tile is relevant for collision detection, update collision map at this pos
    if(updateCollision && collision && mgr && mgr->HasCollisionMap())
        mgr->UpdateCollisionMap(x,y);
}

void TileLayer::Update(uint32 curtime)
{
    if(!used) // no tiles there, nothing to do
        return;

    for(AnimTileMap::iterator it = tilemap.begin(); it != tilemap.end(); it++)
        it->first->Update(curtime);
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

            SDL_BlitSurface(tile->GetSurface(), NULL, target, &rect);
        }
    }
}