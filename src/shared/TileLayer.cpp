#include <SDL/SDL.h>

#include "common.h"
#include "Tile.h"
#include "TileLayer.h"
#include "LayerMgr.h"

TileLayer::TileLayer()
: used(false), collision(false), visible(false), xoffs(0), yoffs(0), camera(NULL), target(NULL),
visible_area(NULL), mgr(NULL)
{
}


// upon deletion, update refcount of all tiles, this ensures proper resource cleanup.
TileLayer::~TileLayer()
{
    for(uint32 y = 0; y < tilearray.size1d(); ++y)
        for(uint32 x = 0; x < tilearray.size1d(); ++x)
            if(BasicTile *tile = tilearray(x,y))
                tile->ref--;
}

// Puts a tile to location (x,y). Set tile to NULL to remove current tile. Does ref-counting.
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
    SDL_Rect blockrect;
    if(visible_area)
    {
        blockrect = *visible_area;
        blockrect.w = std::min(uint32(blockrect.w + blockrect.x), tilearray.size1d()); // use absolute values, this is right x point now
        blockrect.h = std::min(uint32(blockrect.h + blockrect.y), tilearray.size1d()); // now bottom y point
    }
    else
    {
        blockrect.x = blockrect.y = 0;
        blockrect.w = blockrect.h = tilearray.size1d();
    }

    if(camera)
    {
        for(uint32 y = blockrect.y; y < blockrect.h; ++y)
            for(uint32 x = blockrect.x; x < blockrect.w; ++x)
            {
                BasicTile *& tile = tilearray(x,y);
                if(!tile)
                    continue;

                rect.x = (x << 4) + xoffs - camera->x; // x * 16
                rect.y = (y << 4) + yoffs - camera->y; // y * 16

                SDL_BlitSurface(tile->GetSurface(), NULL, target, &rect);
            }
    }
    else
    {
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
}

// this should be called by LayerMgr only
void TileLayer::Resize(uint32 dim)
{
    // this would be bad...
    DEBUG(ASSERT(!mgr));

    // enlarging is easy as no tiles will disappear
    uint32 newsize = clp2(dim); // always n^2
    if(newsize >= tilearray.size1d())
        tilearray.resize(dim, NULL);

    // if shrinking the map, the tiles that are going to disappear have to be reference counted down properly
    // example: (not powers of 2 but who cares)
    /* XXXX.....
    *  XXXX.....
    *  XXXX.....
    *  .........
    *  .........*/
    uint32 y = 0, x;
    while(y < newsize)
    {
        for(x = tilearray.size1d(); x < newsize; x++)
            if(BasicTile *tile = GetTile(x,y))
                SetTile(x, y, NULL, false); // drop tile
        y++;
    }
    for(y = tilearray.size1d(); y < newsize; y++)
        for(x = 0; x < newsize; x++)
            if(BasicTile *tile = GetTile(x,y))
                SetTile(x, y, NULL, false); // drop tile
}

void TileLayer::CopyTo(uint32 startx, uint32 starty, TileLayer *dest, uint32 destx, uint32 desty, uint32 w, uint32 h)
{
    uint32 copyable_src_x = GetArraySize() - startx;
    uint32 copyable_src_y = GetArraySize() - starty;
    uint32 copyable_dest_x = dest->GetArraySize() - destx;
    uint32 copyable_dest_y = dest->GetArraySize() - desty;
    w = std::min(w, std::min(copyable_src_x, copyable_dest_x));
    h = std::min(h, std::min(copyable_src_y, copyable_dest_y));

    for(uint32 iy = 0; iy < h ; iy++)
    {
        for(uint32 ix = 0; ix < w; ix++)
        {
            uint32 fromx = startx + ix;
            uint32 fromy = starty + iy;
            uint32 tox = destx + ix;
            uint32 toy = desty + iy;

            BasicTile *tile = GetTile(fromx, fromy);
            dest->SetTile(tox, toy, tile);
        }
    }
}
