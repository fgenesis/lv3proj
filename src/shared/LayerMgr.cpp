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
#include "UndefUselessCrap.h"


LayerMgr::LayerMgr(Engine *e)
: engine(e), _maxdim(0), _collisionMap(LCF_WALL)
{
    for(uint32 i = 0; i < LAYER_MAX; ++i)
        _layers[i] = NULL;

}

TileLayer *LayerMgr::CreateLayer(bool collision /* = false */, uint32 xoffs /* = 0 */, uint32 yoffs /* = 0 */)
{
    ASSERT(_maxdim); // sanity check

    TileLayer *layer = new TileLayer();
    layer->Resize(_maxdim);
    layer->collision = collision;
    layer->target = engine->GetSurface();
    layer->visible_area = engine->GetVisibleBlockRect();
    layer->visible = true;
    layer->xoffs = xoffs;
    layer->yoffs = yoffs;
    layer->camera = engine->GetCameraPosPtr();
    layer->mgr = this;

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
    _collisionMap.free();
}

void LayerMgr::SetMaxDim(uint32 dim)
{
    _maxdim = dim;
    for(uint32 i = 0; i < LAYER_MAX; i++)
        if(TileLayer *layer = GetLayer(i))
            layer->Resize(dim);
    _collisionMap.resize(dim, false);
}

void LayerMgr::Render(void)
{
    ObjectMgr *omgr = engine->objmgr;
    for(uint32 i = 0; i < LAYER_MAX; ++i)
    {
        // render map tiles
        if(_layers[i] && !engine->HasDebugFlag(EDBG_HIDE_LAYERS))
            _layers[i]->Render();

        // render objects/sprites
        if(!engine->HasDebugFlag(EDBG_HIDE_SPRITES))
            omgr->RenderLayer(i);
    }

    // DEBUG: render collision map
    if(engine->HasDebugFlag(EDBG_COLLISION_MAP_OVERLAY))
    {
        uint32 xmax = std::min(_collisionMap.size1d(), engine->GetResX());
        uint32 ymax = std::min(_collisionMap.size1d(), engine->GetResY());
        for(uint32 y = 0; y < ymax; y++)
        {
            for(uint32 x = 0; x < xmax; x++)
            {
                uint8 f = _collisionMap(x,y);
                uint32 c = 0;
                if(f & 1)
                    c |= 0xFF0000FF;
                if(f & 2)
                    c |= 0xFFFF0000;

                if(c)
                    SDLfunc_putpixel(engine->GetSurface(), x, y, c);
            }
        }
    }
    
    if(engine->HasDebugFlag(EDBG_SHOW_BBOXES))
        omgr->RenderBBoxes();
}

void LayerMgr::Update(uint32 curtime)
{
    for(uint32 i = 0; i < LAYER_MAX; ++i)
        if(_layers[i])
            _layers[i]->Update(curtime);
}

void LayerMgr::CreateCollisionMap(void)
{
    _collisionMap.free();
    _collisionMap.resize(_maxdim * 16, LCF_NONE);
}

void LayerMgr::CreateInfoLayer(void)
{
    DEBUG(ASSERT(_maxdim));
    _infoLayer.resize(_maxdim, TILEFLAG_DEFAULT);
}

// intended for initial collision map generation, NOT for regular updates! (its just too slow)
void LayerMgr::UpdateCollisionMap(void)
{
    DEBUG(ASSERT(_maxdim));
    if(!HasCollisionMap())
        return;
    for(uint32 y = 0; y < _maxdim; ++y)
        for(uint32 x = 0; x < _maxdim; ++x)
            UpdateCollisionMap(x,y);
}

// TODO: this can maybe be a lot more optimized...
// before calling this, make sure you check HasCollisionMap() !!
void LayerMgr::UpdateCollisionMap(uint32 x, uint32 y) // this x and y are tile positions!
{
    //DEBUG_LOG("LayerMgr::UpdateCollisionMap(%u, %u)", x, y);

    uint32 x16 = x << 4, y16 = y << 4; // x*16, y*16

    // first, check if there is a tile explicitly marked as solid
    if(GetInfoLayer())
        if(_infoLayer(x,y) & TILEFLAG_SOLID)
        {
            for(uint32 py = 0; py < 16; ++py)
                for(uint32 px = 0; px < 16; ++px)
                    _collisionMap(x16 + px, y16 + py) |= LCF_WALL;
            return;
        }

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
                _collisionMap(x16 + px, y16 + py) &= ~LCF_WALL;
        return;
    }

    // lock the SDL_Surfaces on all layers for the tile at the specified position, if required
    for(uint32 i = 0; i < LAYER_MAX; ++i)
    {
        if(uselayer[i])
        {
            SDL_Surface *surface = _layers[i]->tilearray(x,y)->GetSurface();
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
                    pix = SDLfunc_getpixel(tile->GetSurface(), px, py);
                    SDL_GetRGBA(pix, tile->GetSurface()->format, &r, &g, &b, &a);
                    // if not fully transparent, this pixel is solid and cannot be passed
                    if(a) // TODO: maybe support that an alpha value below some threshold does NOT count as solid...?
                    {
                        solid = true;
                        break;
                    }
                }
            }
            if(solid)
                _collisionMap(x16 + px, y16 + py) |= LCF_WALL;
            else
                _collisionMap(x16 + px, y16 + py) &= ~LCF_WALL;

        }

    // unlock the SDL_Surfaces on all layers for the tile at the specified position, if required
    for(uint32 i = 0; i < LAYER_MAX; ++i)
    {
        if(uselayer[i])
        {
            SDL_Surface *surface = _layers[i]->tilearray(x,y)->GetSurface();
            if(SDL_MUSTLOCK(surface))
                SDL_UnlockSurface(surface);
        }
    }
}

// TODO: this will ASSERT fail if an object moves out of the screen, fix this
void LayerMgr::RemoveFromCollisionMap(Object *obj)
{
    if(!obj->IsBlocking() || obj->_oldLayerRect.w == 0 || obj->_oldLayerRect.h == 0)
        return;
    int32 xoffs = obj->_oldLayerRect.x;
    int32 yoffs = obj->_oldLayerRect.y;
    int32 x,y;

    // remove LCF_BLOCKING_OBJECT from the prev. rect of this object
    DEBUG(ASSERT(xoffs >= 0 && yoffs >= 0));
    for(y = 0; y < int32(obj->_oldLayerRect.h); ++y)
    {
        for(x = 0; x < int32(obj->_oldLayerRect.w); ++x)
        {
            _collisionMap(x + xoffs, y + yoffs) &= ~LCF_BLOCKING_OBJECT;
        }
    }
}

// TODO: this will ASSERT fail if an object moves out of the screen, fix this
void LayerMgr::UpdateCollisionMap(Object *obj)
{
    // set LCF_BLOCKING_OBJECT in the current rect of the object
    if(obj->IsBlocking())
    {
        int32 ix = int32(obj->x);
        int32 iy = int32(obj->y);
        int32 x,y;

        DEBUG(ASSERT(ix >= 0 && iy >= 0));
        for(y = 0; y < int32(obj->h); ++y)
        {
            for(x = 0; x < int32(obj->w); ++x)
            {
                _collisionMap(x + ix, y + iy) |= LCF_BLOCKING_OBJECT;
            }
        }

        obj->_oldLayerRect.x = ix;
        obj->_oldLayerRect.y = iy;
        obj->_oldLayerRect.w = obj->w;
        obj->_oldLayerRect.h = obj->h;
    }
    else
    {
        obj->_oldLayerRect.x = 0;
        obj->_oldLayerRect.y = 0;
        obj->_oldLayerRect.w = 0;
        obj->_oldLayerRect.h = 0;

    }
}





bool LayerMgr::CollisionWith(BaseRect *rect, int32 skip /* = 4 */, uint8 flags /* = LCF_ALL */)
{
    if(!HasCollisionMap())
        return false;

    int32 x, y;
    int32 x2 = rect->x2();
    int32 y2 = rect->y2();
    for(y = int32(rect->y); y < y2; y += skip)
        for(x = int32(rect->x); x < x2; x += skip)
            if(_collisionMap(x,y) & flags)
                return true;

    // always check bottom edge of the rect, if missed due to skipping
    if(y2 % skip)
        for(x = int32(rect->x); x < x2; x += skip)
            if(_collisionMap(x, y2 - 1) & flags)
                return true;

    // always check right edge of the rect, if missed due to skipping
    if(x2 % skip)
        for(y = int32(rect->y); y < y2; y += skip)
            if(_collisionMap(x2 - 1, y) & flags)
                return true;

    // always check bottom right pixel of the rect (if missed due to skipping, but we skip the check for that)
    if(_collisionMap(x2 - 1, y2 - 1) & flags)
        return true;

    return false;
}

uint32 LayerMgr::CanMoveToDirection(BaseRect *rect, uint8 direction, uint32 pixels /* = 1 */ )
{
    MovementDirectionInfo mdi(*rect, direction);
    return CanMoveToDirection(rect, mdi, pixels);
}

uint32 LayerMgr::CanMoveToDirection(BaseRect *rect, MovementDirectionInfo& mdi, uint32 pixels /* = 1 */ )
{
    if(!HasCollisionMap())
        return pixels;

    uint32 moveable = 0;
    BaseRect r = rect->cloneRect();
    bool stop = false;
    int32 xi, yi;
    int32 xa, ya;
    while(pixels--)
    {
        xa = int32(r.x) + mdi.xstep;
        ya = int32(r.y) + mdi.ystep;
        if(mdi.ystep != 0)
        {
            for(xi = 0; xi < int32(r.w); ++xi)
            {
                if(_collisionMap(xa + xi, ya + mdi.yoffs))
                {
                    stop = true;
                    break;
                }
            }
        }
        if(mdi.xstep != 0)
        {
            for(yi = 0; yi < int32(r.h); ++yi)
            {
                if(_collisionMap(xa + mdi.xoffs, ya + yi))
                {
                    stop = true;
                    break;
                }
            }
        }
        if(stop)
            break;

        moveable++;
        r.MoveRelative(mdi.xstep, mdi.ystep);
    }
    return moveable;
}

Point LayerMgr::GetNonCollidingPoint(BaseRect *rect, uint8 direction, uint32 maxdist /* = -1 */)
{
    DEBUG(ASSERT(direction));
    MovementDirectionInfo mdi(*rect, direction);
    BaseRect r = rect->cloneRect();
    int32 moveable;
    if(moveable = (int32)CanMoveToDirection(&r, mdi, maxdist)) // try to move as far as possible
    {
        r.MoveRelative(mdi.xstep * moveable, mdi.ystep * moveable);
    }
    return Point(int32(r.x), int32(r.y));
}

// TODO: this method is obsolete, remove in future
bool LayerMgr::CanFallDown(Point anchor, uint32 arealen)
{
    if(!HasCollisionMap())
        return true;

    // this check goes like this: 123412341234... (usually faster than linear search)
    for(uint32 align = 0; align < 4; ++align)
    {
        uint32 xmax = anchor.x + arealen + align;
        for(uint32 x = anchor.x - arealen + align; x < xmax; x += 4)
        {
            if(_collisionMap(x, anchor.y))
            {
                return false;
            }
        }
    }
    return true;
}

void LayerMgr::LoadAsciiLevel(AsciiLevel *level)
{
    // reserve space
    SetMaxDim(level->tiles.size1d());

    // create the layers
    TileLayer *layers[LAYER_MAX];
    for(uint32 i = 0; i < LAYER_MAX; ++i)
    {
        layers[i] = CreateLayer();
        SetLayer(layers[i], i);
    }
    TileLayer *baseLayer = layers[6];
    TileLayer *animLayer = layers[7];
    
    // load the tiles
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
                startAnim = "";
                SplitFilenameToProps(f.c_str(), &realFileName, &startAnim);
                if(BasicTile *tile = AnimatedTile::New(realFileName.c_str()))
                {
                    (tile->GetType() == TILETYPE_ANIMATED ? animLayer : baseLayer)->SetTile(x,y,tile,false);
                    tile->ref--;
                }
            }
        }
    }

    logdetail("ASCII Level loaded.");
}
            