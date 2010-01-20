#include "common.h"
#include "Engine.h"
#include "ResourceMgr.h"
#include "TileMgr.h"

AnimatedTile::AnimatedTile(char *afile)
: surface(NULL)
{
    ani = (Anim*)resMgr.GetResource(afile);
    ASSERT(ani);
    SetupDefaults();
}

TileMgr::TileMgr(Engine* e)
: engine(e)
{
    staticTiles.resize(128); // TODO: TEMP VALUE
    staticTiles.fill(NULL);
    animTiles.resize(128); // TODO: TEMP VALUE
    animTiles.fill(NULL);
}

TileMgr::~TileMgr()
{
}

void TileMgr::RenderStaticTiles(void)
{
    SDL_Rect rect;
    SDL_Surface *screen = engine->GetSurface();
    // TODO: render only visible area!
    for(uint32 y = 0; y < staticTiles.size1d(); ++y)
    {
        for(uint32 x = 0; x < staticTiles.size1d(); ++x)
        {
            SDL_Surface *tile = staticTiles(x,y);
            if(!tile)
                continue;

            rect.x = x << 4; // x * 16
            rect.y = y << 4; // y * 16

            SDL_BlitSurface(tile, NULL, screen, &rect);
        }
    }
}

void TileMgr::RenderAnimatedTiles(void)
{
    SDL_Rect rect;
    SDL_Surface *screen = engine->GetSurface();
    // TODO: render only visible area!
    for(uint32 y = 0; y < animTiles.size1d(); ++y)
    {
        for(uint32 x = 0; x < animTiles.size1d(); ++x)
        {
            AnimatedTile *tile = animTiles(x,y);
            if(!tile || !tile->surface)
                continue;

            rect.x = x << 4; // x * 16
            rect.y = y << 4; // y * 16

            SDL_BlitSurface(tile->surface, NULL, screen, &rect);
        }
    }
}

void TileMgr::HandleAnimation(uint32 ms)
{
    uint32 to = animTiles.size2d();
    AnimatedTile *tile;
    uint32 diff;
    for(uint32 i = 0; i < to; ++i)
    {
        if(tile = animTiles[i])
        {
            if(tile->timeleft < ms)
            {
                diff = ms - tile->timeleft;
                tile->timeleft = diff;
                if(tile->curFrame->nextframe)
                {
                    tile->curFrame = &((*tile->curFrameStore)[tile->curFrame->nextframe - 1]);
                    tile->surface = tile->curFrame->surface;
                }
                else if(tile->curFrame->nextanim.length())
                {
                    tile->curFrameStore = &(tile->ani->anims[tile->curFrame->nextanim]);
                    tile->curFrame = &((*tile->curFrameStore)[0]);
                    tile->surface = tile->curFrame->surface;
                }
                tile->timeleft = tile->curFrame->frametime;
            }
            else
            {
                tile->timeleft -= ms;
            }
        }
    }
}