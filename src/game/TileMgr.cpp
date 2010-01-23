#include "common.h"
#include "Engine.h"
#include "ResourceMgr.h"
#include "TileMgr.h"

AnimatedTile::AnimatedTile(char *afile)
: surface(NULL)
{
    ani = resMgr.LoadAnim(afile, false);
    ASSERT(ani);
    SetupDefaults();
}

TileMgr::TileMgr(Engine* e)
: engine(e), _staticSurfaceNeedsUpdate(false), _staticSurface(NULL)
{
    staticTiles.resize(128); // TODO: TEMP VALUE
    staticTiles.fill(NULL);
    animTiles.resize(128); // TODO: TEMP VALUE
    animTiles.fill(NULL);
}

void TileMgr::InitStaticSurface(void)
{
    if(_staticSurface)
        return; // prevent init twice or more when changing resolution

    // create the surface where static tiles will be drawn to,
    // use the same resolution as the screen
    _staticSurface = SDL_CreateRGBSurface(SDL_HWSURFACE, engine->GetResX(), engine->GetResY(), engine->GetBPP(), 0, 0, 0, 0);
    ASSERT(_staticSurface);
}

TileMgr::~TileMgr()
{
}

SDL_Rect TileMgr::_GetVisibleBlockRect(void)
{
    SDL_Rect rect;
    rect.x = engine->GetCameraPos().x >> 4; // (x / 16) (16 = block size in pixels)
    rect.y = engine->GetCameraPos().y >> 4;
    rect.w = (engine->GetResX() >> 4) + 1;
    rect.h = (engine->GetResY() >> 4) + 1;
    return rect;
}

void TileMgr::RenderStaticTiles(void)
{
    if(_staticSurfaceNeedsUpdate)
    {
        _staticSurfaceNeedsUpdate = false;
        SDL_Rect rect;

        SDL_Rect blockrect = _GetVisibleBlockRect();
        blockrect.w = std::min(uint32(blockrect.w + blockrect.x), staticTiles.size1d()); // use absolute values, this is right x point now
        blockrect.h = std::min(uint32(blockrect.h + blockrect.y), staticTiles.size1d()); // now bottom y point

        for(uint32 y = blockrect.y; y < blockrect.h; ++y)
        {
            for(uint32 x = blockrect.x; x < blockrect.w; ++x)
            {
                SDL_Surface *tile = staticTiles(x,y);
                if(!tile)
                    continue;

                rect.x = x << 4; // x * 16
                rect.y = y << 4; // y * 16

                SDL_BlitSurface(tile, NULL, _staticSurface, &rect);
            }
        }
    }
    
    SDL_BlitSurface(_staticSurface, NULL, engine->GetSurface(), NULL);
    
}

void TileMgr::RenderAnimatedTiles(void)
{
    SDL_Rect rect;
    SDL_Surface *screen = engine->GetSurface();

    SDL_Rect blockrect = _GetVisibleBlockRect();
    blockrect.w = std::max(uint32(blockrect.w + blockrect.x), staticTiles.size1d()); // use absolute values, this is right x point now
    blockrect.h = std::max(uint32(blockrect.h + blockrect.y), staticTiles.size1d()); // now bottom y point

    for(uint32 y = blockrect.y; y < blockrect.h; ++y)
    {
        for(uint32 x = blockrect.x; x < blockrect.w; ++x)
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
                    tile->curFrameStore = &(tile->ani->anims[tile->curFrame->nextanim]); // <-- TODO: this call should be precalculated (eats CPU)
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