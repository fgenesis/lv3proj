#include <SDL/SDL.h>
#include "SDL_func.h"

#include "common.h"
#include "Engine.h"
#include "ResourceMgr.h"
#include "AsciiLevelParser.h"
#include "LayerMgr.h"
#include "Objects.h"
#include "ObjectMgr.h"
#include "SharedDefines.h"


LayerMgr::LayerMgr(Engine* e)
: engine(e), _maxdim(0), _collisionMap(NULL)
{
    for(uint32 i = 0; i < LAYER_MAX; ++i)
        _layers[i] = NULL;

}

TileLayer *LayerMgr::CreateLayer(bool collision, uint32 xoffs /* = 0 */, uint32 yoffs /* = 0 */)
{
    ASSERT(_maxdim); // sanity check

    TileLayer *layer = new TileLayer;
    layer->mgr = this;
    layer->tilearray.resize(_maxdim, NULL);
    layer->used = 0;
    layer->collision = collision;
    layer->target = engine->GetSurface();
    layer->visible_area = engine->GetVisibleBlockRect();
    layer->visible = true;
    layer->xoffs = xoffs;
    layer->yoffs = yoffs;

    return layer;
}

LayerMgr::~LayerMgr()
{
    Clear();
}

void LayerMgr::Clear(void)
{
    for(uint32 i = 0; i < LAYER_MAX; ++i)
    {
        if(_layers[i])
        {
            delete _layers[i];
        }

        _layers[i] = NULL;
    }
    if(_collisionMap)
        delete _collisionMap;
}

void LayerMgr::Render(void)
{
    for(uint32 i = 0; i < LAYER_MAX; ++i)
    {
        // render map tiles
        if(_layers[i])
            _layers[i]->Render();

        // render objects/sprites
        engine->objmgr->RenderLayer(i);
    }
}

void LayerMgr::Update(uint32 curtime)
{
    for(uint32 i = 0; i < LAYER_MAX; ++i)
        if(_layers[i])
            _layers[i]->Update(curtime);
}

void LayerMgr::CreateCollisionMap(void)
{
    if(_collisionMap)
        delete _collisionMap;
    _collisionMap = new BitSet2d(_maxdim * 16, _maxdim * 16, true); // _maxdim is tile size, * 16 is pixel size
}

// intended for initial collision map generation, NOT for regular updates! (its just too slow)
void LayerMgr::UpdateCollisionMap(void)
{
    for(uint32 y = 0; y < _maxdim; ++y)
        for(uint32 x = 0; x < _maxdim; ++x)
            UpdateCollisionMap(x,y);
}

// TODO: this can maybe be a lot more optimized...
void LayerMgr::UpdateCollisionMap(uint32 x, uint32 y) // this x and y are tile positions!
{
    DEBUG_LOG("LayerMgr::UpdateCollisionMap(%u, %u)", x, y);
    DEBUG(ASSERT(_collisionMap));
    uint32 x16 = x << 4, y16 = y << 4; // x*16, y*16
    // pre-select layers to be used, and check if a tile exists at that position
    bool uselayer[LAYER_MAX];
    bool counter = 0;
    for(uint32 i = 0; i < LAYER_MAX; ++i)
        if(uselayer[i] = _layers[i] && _layers[i]->used && _layers[i]->collision && _layers[i]->tilearray(x,y))
            ++counter;
    if(!counter) // no layers to be used, means there is no tile here on any layer -> tile is fully passable. update all 16x16 pixels.
    {
        for(uint32 py = 0; py < 16; ++py)
            for(uint32 px = 0; px < 16; ++px)
                _collisionMap->set(x16 + px, y16 + py, false);
        return;
    }

    // lock the SDL_Surfaces on all layers for the tile at the specified position, if required
    for(uint32 i = 0; i < LAYER_MAX; ++i)
    {
        if(uselayer[i])
        {
            SDL_Surface *surface = _layers[i]->tilearray(x,y)->surface;
            if(SDL_MUSTLOCK(surface))
                SDL_LockSurface(surface);
        }
    }
    uint32 pix;
    uint8 r, g, b, a;
    bool solid;

    for(uint32 py = 0; py < 16; ++py)
        for(uint32 px = 0; px < 16; ++px)
        {
            solid = false;
            for(uint32 i = 0; i < LAYER_MAX; ++i)
            {
                if(uselayer[i])
                {
                    BasicTile *tile = _layers[i]->tilearray(x,y);
                    pix = SDLfunc_getpixel(tile->surface, px, py);
                    SDL_GetRGBA(pix, tile->surface->format, &r, &g, &b, &a);
                    // if not fully transparent, this pixel is solid and cannot be passed
                    if(a) // TODO: maybe support that an alpha value below some threshold does NOT count as solid...?
                    {
                        solid = true;
                        break;
                    }
                }
            }
            _collisionMap->set(x16 + px, y16 + py, solid);
        }

    // unlock the SDL_Surfaces on all layers for the tile at the specified position, if required
    for(uint32 i = 0; i < LAYER_MAX; ++i)
    {
        if(uselayer[i])
        {
            SDL_Surface *surface = _layers[i]->tilearray(x,y)->surface;
            if(SDL_MUSTLOCK(surface))
                SDL_UnlockSurface(surface);
        }
    }
}

bool LayerMgr::CollisionWith(ActiveRect *rect, int32 skip /* = 4 */)
{
    int32 x, y;
    int32 x2 = rect->x2();
    int32 y2 = rect->y2();
    for(y = int32(rect->y); y < y2; y += skip)
        for(x = int32(rect->x); x < x2; x += skip)
            if(_collisionMap->at(x,y))
                return true;

    // always check bottom edge of the rect, if missed due to skipping
    if(y2 % skip)
        for(x = int32(rect->x); x < x2; x += skip)
            if(_collisionMap->at(x, y2 - 1))
                return true;

    // always check right edge of the rect, if missed due to skipping
    if(x2 % skip)
        for(y = int32(rect->y); y < y2; y += skip)
            if(_collisionMap->at(x2 - 1, y))
                return true;

    // always check bottom right pixel of the rect (if missed due to skipping, but we skip the check for that)
    if(_collisionMap->at(x2 - 1, y2 - 1))
        return true;

    return false;
}

Point LayerMgr::GetClosestNonCollidingPoint(ActiveRect *rect, uint8 direction)
{
    int32 xstart, ystart, xend, yend, xstep = 0, ystep = 0, xoffs = 0, yoffs = 0;
    uint32 directionCtr = 0; // the amount of directions set must be in [1..2] after counting
    if(direction & DIRECTION_LEFT)
    {
        xstart = int32r(rect->x); // leftmost position
        if(xstart < 0) xstart = 0; else if(xstart > int32(GetMaxPixelDim())) xstart = int32(GetMaxPixelDim());
        xend = 0;
        xstep = -1;
        ++directionCtr;
    }
    else if(direction & DIRECTION_RIGHT)
    {
        xstart = rect->x2(); // rightmost position
        if(xstart < 0) xstart = 0; else if(xstart > int32(GetMaxPixelDim())) xstart = int32(GetMaxPixelDim());
        xend = int32(GetMaxPixelDim()); // ... to the right of the screen
        xstep = 1;
        xoffs = -(int32(rect->w) - 1);
        ++directionCtr;
    }
    else // not moving horizontally
    {
        xstart = int32r(rect->x); // left object position
        if(xstart < 0) xstart = 0; else if(xstart > int32(GetMaxPixelDim())) xstart = int32(GetMaxPixelDim());
        xend = rect->x2(); // right object position
        if(xend < 0) xend = 0; else if(xend > int32(GetMaxPixelDim())) xend = int32(GetMaxPixelDim());
        xstep = 1;
    }
    if(direction & DIRECTION_UP)
    {
        ystart = int32r(rect->y); // topmost position
        if(ystart < 0) ystart = 0; else if(ystart > int32(GetMaxPixelDim())) ystart = int32(GetMaxPixelDim());
        yend = 0; // ... up to the top of the screen
        ystep = -1;
        ++directionCtr;
    }
    else if(direction & DIRECTION_DOWN)
    {
        ystart = rect->y2(); // bottom-most position
        if(ystart < 0) ystart = 0; else if(ystart > int32(GetMaxPixelDim())) ystart = int32(GetMaxPixelDim());
        yend = int32(GetMaxPixelDim()); // ... down to the bottom of the screen
        ystep = 1;
        yoffs = -(int32(rect->h) - 1);
        ++directionCtr;
    }
    else // not moving vertically
    {
        ystart = int32r(rect->y); // top object position
        if(ystart < 0) ystart = 0; else if(ystart > int32(GetMaxPixelDim())) ystart = int32(GetMaxPixelDim());
        yend = rect->y2(); // bottom object position
        if(yend < 0) yend = 0; else if(yend > int32(GetMaxPixelDim())) yend = int32(GetMaxPixelDim());
        ystep = 1;
    }
    // collision with no movement direction is as unlikely as movement in 3 directions
    ASSERT(directionCtr >= 1 && directionCtr <= 2);

    bool c = false;
    int32 x = xstart, y = ystart;
    int32 goodx = rect->x, goody = rect->y;

    // moving by 1 axis
    if(directionCtr == 1)
    {
        if(direction & (DIRECTION_UP | DIRECTION_DOWN))
        {
            for(y = ystart ; (ystep > 0 ? y < yend : y > yend); y += ystep)
            {
                for(x = xstart ; (xstep > 0 ? x < xend : x > xend); x += xstep)
                {
                    if(_collisionMap->at(x,y))
                        return Point(goodx + xoffs, goody + yoffs);
                }
                goody = y;
            }
        }
        else if(direction & (DIRECTION_LEFT | DIRECTION_RIGHT))
        {
            for(x = xstart ; (xstep > 0 ? x < xend : x > xend); x += xstep)
            {
                for(y = ystart ; (ystep > 0 ? y < yend : y > yend); y += ystep)
                {
                    if(_collisionMap->at(x,y))
                        return Point(goodx + xoffs, goody + yoffs);
                }
                goodx = x;
            }
        }
    }
    else // special case: diagonal movement (2 axes)
    {
        do 
        {
            for(x = xstart; abs(x - xstart) < int32(rect->w); x -= xstep)
            {
                if(_collisionMap->at(x,ystart))
                    return Point(goodx, goody);
            }         
            for(y = ystart; abs(y - ystart) < int32(rect->h); y -= ystep)
            {
                if(_collisionMap->at(xstart,y))
                    return Point(goodx, goody);
            }
            xstart += xstep;
            ystart += ystep;
            goodx += xstep;
            goody += ystep;

            /*
            // limit checking to max. width and height of the rect
            int32 xstop = (xstep > 0 ? min(xstart + int32(rect->w), xend) : max(xstart - int32(rect->w), xend));
            int32 ystop = (ystep > 0 ? min(ystart + int32(rect->h), yend) : max(ystart - int32(rect->h), yend));
            goodx = rect->x;            
            for(y = ystart ; (ystep > 0 ? y < ystop : y > ystop); y += ystep)
            {
                if(_collisionMap->at(xstart,y))
                    return Point(goodx + xoffs, goody + yoffs);
                goody = y; // no collision so far, this point was ok
            }
            goody = rect->y;
            for(x = xstart ; (xstep > 0 ? x < xstop : x > xstop); x += xstep)
            {
                if(_collisionMap->at(x,ystart))
                    return Point(goodx + xoffs, goody + yoffs);
                goodx = x; // no collision so far, this point was ok
            }
            xstart += xstep;
            ystart += ystep;
            */
        }
        while( (ystep > 0 ? ystart < yend : ystart > yend) || (xstep > 0 ? xstart < xend : xstart > xend) );
    }

    return Point(int32(rect->x),int32(rect->y)); // nothing found
}

bool LayerMgr::CanFallDown(Point anchor, uint32 arealen)
{
    // this check goes like this: 123412341234... (usually faster than linear search)
    for(uint32 align = 0; align < 4; ++align)
    {
        uint32 xmax = anchor.x + arealen + align;
        for(uint32 x = anchor.x - arealen + align; x < xmax; x += 4)
        {
            if(_collisionMap->at(x, anchor.y))
            {
                return false;
            }
        }
    }
    return true;
}

bool LayerMgr::LoadAsciiLevel(AsciiLevel *level)
{
    // reserve space
    SetMaxDim(level->tiles.size1d());

    // for the sake of loading speed in debug mode, this is not enabled for now
    CreateCollisionMap();

    // create the layers
    TileLayer *baseLayer = CreateLayer(true);
    TileLayer *animLayer = CreateLayer();
    
    // load the tiles
    std::map<std::string, BasicTile*> tmap; // stores already allocated animated tiles
    std::map<std::string, BasicTile*>::iterator it;
    std::string realFileName, startAnim;
    uint32 startIdx = 0;
    std::string startIdxStr;
    for(uint32 y = 0; y < level->tiles.size1d(); ++y)
    {
        for(uint32 x = 0; x < level->tiles.size1d(); ++x)
        {
            std::vector<std::string>& filevect = level->tiledata[level->tiles(x,y)];
            for(uint32 i = 0; i < filevect.size(); ++i)
            {
                std::string& f = filevect[i];
                startIdxStr = "";
                startAnim = "";
                SplitFilenameToProps(f.c_str(), &realFileName, &startIdxStr, &startAnim);
                startIdx = atoi(startIdxStr.c_str());
                if(FileGetExtension(realFileName) == ".png")
                {
                    it = tmap.find(f);
                    BasicTile *staTile = NULL;
                    if(it == tmap.end())
                    {
                        staTile = new BasicTile;
                        staTile->surface = resMgr.LoadImg((char*)f.c_str());
                        staTile->filename = realFileName;
                        tmap[f] = staTile;
                    }
                    else
                        staTile = it->second;

                    baseLayer->SetTile(x,y,staTile,false); // this will just copy the surface
                }
                else if(FileGetExtension(realFileName) == ".anim")
                {
                    it = tmap.find(f);
                    AnimatedTile *atile = NULL;
                    if(it == tmap.end())
                    {
                        Anim *ani = resMgr.LoadAnim((char*)realFileName.c_str());
                        if(ani)
                        {
                            atile = new AnimatedTile(ani, startIdx, startAnim.c_str());
                            atile->Init(Engine::GetCurFrameTime());
                            atile->filename = realFileName;
                            tmap[f] = atile;
                        }
                        else
                            logerror("LayerMgr::LoadAsciiLevel: Error loading '%s'", realFileName.c_str());
                    }
                    else
                        atile = (AnimatedTile*)it->second;
                        
                    animLayer->SetTile(x,y,atile,false);
                }
            }
        }
    }

    SetLayer(baseLayer, LAYER_DEFAULT_ENV);
    SetLayer(animLayer, LAYER_DEFAULT_ENV + 1);
    SetLayer(CreateLayer(), LAYER_DEFAULT_ENV + 2); // for testing

    // for the sake of loading speed in debug mode, this is not enabled for now
    logdebug("Calculating collision map...");
    UpdateCollisionMap();
    

    logdetail("ASCII Level loaded.");

    return true;
}
            