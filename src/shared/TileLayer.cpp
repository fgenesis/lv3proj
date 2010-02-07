#include <SDL/SDL.h>

#include "common.h"
#include "Tile.h"
#include "TileLayer.h"

void TileLayerBase::Render(void)
{
    if(!visible)
        return;
    SDL_Rect dst;
    dst.x = xoffs;
    dst.y = yoffs;
    dst.w = dst.x + surface->w;
    dst.h = dst.w + surface->h;
    SDL_BlitSurface(surface, NULL, target, &dst); // TODO: optimize this!!
}

TileLayerBase::~TileLayerBase()
{
    if(surface)
        SDL_FreeSurface(surface);
}

TileLayerArray2d::~TileLayerArray2d()
{
    // need to store the pointers in a set because one pointer is probably used multiple times in the tilearray
    // every pointer has to be deleted only ONCE, obviously.
    // TODO: be sure a pointer is not used in multiple layers...
    std::set<BasicTile*> tmp;
    for (uint32 i = 0; i < tilearray.size2d(); i++)
        tmp.insert(tilearray[i]);
    for(std::set<BasicTile*>::iterator it = tmp.begin(); it != tmp.end(); it++)
        delete *it;
}

// Puts a tile to location (x,y). Set tile to NULL to remove current tile.
void TileLayerArray2d::SetTile(uint32 x, uint32 y, BasicTile *s)
{
    BasicTile *& tileref = tilearray(x,y);
    if(tileref != s)
        _update = true;
    tileref = s;
}

BasicTile *TileLayerArray2d::GetTile(uint32 x, uint32 y)
{
    return tilearray(x,y);
}

void TileLayerArray2d::Render(void)
{
    if(!visible)
        return;

    if(_update)
    {
        _update = false;
        SDL_Rect rect;

        SDL_Rect blockrect = *visible_area;
        blockrect.w = std::min(uint32(blockrect.w + blockrect.x), tilearray.size1d()); // use absolute values, this is right x point now
        blockrect.h = std::min(uint32(blockrect.h + blockrect.y), tilearray.size1d()); // now bottom y point

        for(uint32 y = blockrect.y; y < blockrect.h; ++y)
        {
            for(uint32 x = blockrect.x; x < blockrect.w; ++x)
            {
                BasicTile *tile = tilearray(x,y);
                if(!tile)
                    continue;

                rect.x = (x << 4) + xoffs; // x * 16
                rect.y = (y << 4) + yoffs; // y * 16

                SDL_BlitSurface(tile->surface, NULL, surface, &rect);
            }
        }
    }

    TileLayerBase::Render();
}

// Puts a tile to location (x,y). Set tile to NULL to remove current tile.
void TileLayer::SetTile(uint32 x, uint32 y, BasicTile *ani) // we expect ani to be an animated tile, but it does not have to
{
    BasicTile *& tileref = tilearray(x,y);
    if(tileref == ani)
        return;

    // insert only animated tiles into the animated tile store
    if(ani && ani->type == TILETYPE_ANIMATED)
    {
        AnimTileSet::iterator it = tileset.find((AnimatedTile*)tileref);
        if(it != tileset.end())
            tileset.erase(it);

        tileset.insert((AnimatedTile*)ani);
    }

    tileref = ani;
}

void TileLayer::Update(uint32 curtime)
{
    for(AnimTileSet::iterator it = tileset.begin(); it != tileset.end(); it++)
    {
        AnimatedTile *& tile = *it;
        if(tile->nextupdate < curtime)
        {
            if(tile->curFrame->nextframe)
            {
                tile->curFrame = &((*tile->curFrameStore)[tile->curFrame->nextframe - 1]);
                tile->surface = tile->curFrame->surface;
            }
            else if(tile->curFrame->nextanim.length())
            {
                tile->curFrameStore = &(tile->ani->anims[tile->curFrame->nextanim]); // <-- TODO: this call could be precalculated, maybe (eats CPU)
                tile->curFrame = &((*tile->curFrameStore)[0]);
                tile->surface = tile->curFrame->surface;
            }
            tile->nextupdate = curtime + tile->curFrame->frametime;
        }
    }
}

void TileLayer::Render(void)
{
    if(!visible)
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