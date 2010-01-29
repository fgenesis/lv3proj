#include <SDL/SDL.h>

#include "common.h"
#include "Tile.h"
#include "TileLayer.h"

void TileLayerBase::Render(void)
{
    SDL_BlitSurface(surface, NULL, target, NULL); // TODO: optimize this!!
}

// Puts a tile to location (x,y). Set tile to NULL to remove current tile.
void TileLayerArray2d::SetTile(uint32 x, uint32 y, SDL_Surface *s)
{
    SDL_Surface *& tileref = tilearray(x,y);
    if(tileref != s)
        _update = true;
    tileref = s;
}

void TileLayerArray2d::Render(void)
{
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
                SDL_Surface *tile = tilearray(x,y);
                if(!tile)
                    continue;

                rect.x = x << 4; // x * 16
                rect.y = y << 4; // y * 16

                SDL_BlitSurface(tile, NULL, surface, &rect);
            }
        }
    }

    TileLayerBase::Render();
}

// Puts a tile to location (x,y). Set tile to NULL to remove current tile.
void TileLayer::SetTile(uint32 x, uint32 y, AnimatedTile *ani)
{
    AnimatedTile *& tileref = tilearray(x,y);
    if(tileref == ani)
        return;

    AnimTileSet::iterator it = tileset.find(tileref);
    if(it != tileset.end())
        tileset.erase(it);

    if(ani)
    {
        tileset.insert(ani);
        tileref = ani;
    }
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
    AnimTileSet::iterator it = tileset.begin();
}

void TileLayer::Render(void)
{
    SDL_Rect rect;
    SDL_Rect blockrect = *visible_area;
    blockrect.w = std::min(uint32(blockrect.w + blockrect.x), tilearray.size1d()); // use absolute values, this is right x point now
    blockrect.h = std::min(uint32(blockrect.h + blockrect.y), tilearray.size1d()); // now bottom y point

    for(uint32 y = blockrect.y; y < blockrect.h; ++y)
    {
        for(uint32 x = blockrect.x; x < blockrect.w; ++x)
        {
            AnimatedTile *& tile = tilearray(x,y);
            if(!tile)
                continue;


            rect.x = x << 4; // x * 16
            rect.y = y << 4; // y * 16

            SDL_BlitSurface(tile->surface, NULL, target, &rect);
        }
    }
}