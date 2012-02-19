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
#include "CollisionUtil.h"
#include "UndefUselessCrap.h"

// raycasting related debug stuff
//#define DEBUG_RAYCAST_VIS

LayerMgr::LayerMgr(Engine *e)
: _engine(e), _maxdim(0), _collisionMap(LCF_WALL), _tileDimX(16), _tileDimY(16)
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
    layer->target = _engine->GetSurface();
    layer->visible_area = _engine->GetVisibleBlockRect();
    layer->visible = true;
    layer->xoffs = xoffs;
    layer->yoffs = yoffs;
    layer->camera = _engine->GetCameraPtr();
    layer->mgr = this;
    layer->_tileDimY = _tileDimX;
    layer->_tileDimY = _tileDimY;

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

void LayerMgr::SetRenderOffset(int32 x, int32 y)
{
    for(uint32 i = 0; i < LAYER_MAX; i++)
        if(TileLayer *layer = GetLayer(i))
        {
            layer->xoffs = x;
            layer->yoffs = y;
        }
}

void LayerMgr::SetLayer(TileLayer *layer, uint32 depth)
{
    if(_layers[depth])
        _layers[depth]->mgr = NULL;
    if(layer)
        layer->mgr = this;
    _layers[depth] = layer;
}

void LayerMgr::Render(void)
{
    ObjectMgr *omgr = _engine->objmgr;
    for(uint32 i = 0; i < LAYER_MAX; ++i)
    {
        // render map tiles
        if(_layers[i] && !_engine->HasDebugFlag(EDBG_HIDE_LAYERS))
            _layers[i]->Render();

        // render objects/sprites
        if(!_engine->HasDebugFlag(EDBG_HIDE_SPRITES))
            omgr->RenderLayer(i);
    }

    // DEBUG: render collision map
    if(_engine->HasDebugFlag(EDBG_COLLISION_MAP_OVERLAY))
    {
        // WARNING SLOW
        const Camera& cam = _engine->GetCamera();
        uint32 cx = cam.x; // do float->uint32 conversion early and only once
        uint32 cy = cam.y;
        uint32 xmax = std::min(_collisionMap.size1d(), _engine->GetResX());
        uint32 ymax = std::min(_collisionMap.size1d(), _engine->GetResY());
        for(uint32 y = 0; y < ymax; y++)
        {
            for(uint32 x = 0; x < xmax; x++)
            {
                if(uint32 f = _collisionMap(x,y))
                {
                    uint32 c = 0;
                    if(f & 1)
                        c |= 0xFF0000FF;
                    if(f & 2)
                        c |= 0xFFFF0000;
                    SDLfunc_putpixel_safe(_engine->GetSurface(), x - cx, y - cy, c);
                }
            }
        }
    }
    
    if(_engine->HasDebugFlag(EDBG_SHOW_BBOXES))
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
    _collisionMap.resize(GetMaxPixelDim(), LCF_NONE);
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

    uint32 x16 = x * _tileDimY, y16 = y * _tileDimY;

    // first, check if there is a tile explicitly marked as solid
    if(GetInfoLayer())
        if(_infoLayer(x,y) & TILEFLAG_SOLID)
        {
            for(uint32 py = 0; py < _tileDimY; ++py)
                for(uint32 px = 0; px < _tileDimX; ++px)
                    _collisionMap(x16 + px, y16 + py) |= LCF_WALL;
            return;
        }

    // start-select layers to be used, and check if a tile exists at that position
    bool uselayer[LAYER_MAX];
    bool counter = 0;
    for(uint32 i = 0; i < LAYER_MAX; ++i)
        if(uselayer[i] = _layers[i] && _layers[i]->used && _layers[i]->collision && _layers[i]->tilearray(x,y))
            ++counter;
    if(!counter) // no layers to be used, means there is no tile here on any layer -> tile is fully passable. update all 16x16 pixels.
    {
        for(uint32 py = 0; py < _tileDimY; ++py)
            for(uint32 px = 0; px < _tileDimX; ++px)
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

    for(uint32 py = 0; py < _tileDimY; ++py)
        for(uint32 px = 0; px < _tileDimX; ++px)
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
// -- wait, what? wasn't this fixed already?
void LayerMgr::RemoveFromCollisionMap(Object *obj)
{
    if(!obj->IsBlocking() || obj->_oldLayerRect.w == 0 || obj->_oldLayerRect.h == 0)
        return;
    int32 xoffs = obj->_oldLayerRect.x;
    int32 yoffs = obj->_oldLayerRect.y;
    int32 xlim = obj->_oldLayerRect.w;
    int32 ylim = obj->_oldLayerRect.h;
    int32 x,y;

    if(xoffs < 0)
    {
        xlim += xoffs;
        if(xlim < 0)
            return;
        xoffs = 0;
    }

    if(yoffs < 0)
    {
        ylim += yoffs;
        if(ylim < 0)
            return;
        yoffs = 0;
    }

    // remove LCF_BLOCKING_OBJECT from the prev. rect of this object
    DEBUG(ASSERT(xoffs >= 0 && yoffs >= 0));
    const LayerCollisionFlag lcf = obj->GetOwnLCF();
    for(y = 0; y < ylim; ++y)
    {
        for(x = 0; x < xlim; ++x)
        {
            _collisionMap(x + xoffs, y + yoffs) &= ~lcf;
        }
    }
}

// TODO: this will ASSERT fail if an object moves out of the screen, fix this
// -- here too?
// TODO: add support for per-pixel collision map (also when objects are animated)
void LayerMgr::UpdateCollisionMap(Object *obj)
{
    // set LCF_BLOCKING_OBJECT in the current rect of the object
    if(obj->IsBlocking())
    {
        int32 xoffs = int32(obj->x);
        int32 yoffs = int32(obj->y);
        int32 xlim = int32(obj->w);
        int32 ylim = int32(obj->h);

        obj->_oldLayerRect.x = xoffs;
        obj->_oldLayerRect.y = yoffs;
        obj->_oldLayerRect.w = xlim;
        obj->_oldLayerRect.h = ylim;

        int32 x,y;

        if(xoffs < 0)
        {
            xlim += xoffs;
            if(xlim < 0)
                return;
            xoffs = 0;
        }

        if(yoffs < 0)
        {
            ylim += yoffs;
            if(ylim < 0)
                return;
            yoffs = 0;
        }

        DEBUG(ASSERT(xoffs >= 0 && yoffs >= 0));
        const LayerCollisionFlag lcf = obj->GetOwnLCF();
        for(y = 0; y < ylim; ++y)
        {
            for(x = 0; x < xlim; ++x)
            {
                _collisionMap(x + xoffs, y + yoffs) |= lcf;
            }
        }
    }
    else
    {
        obj->_oldLayerRect.x = 0;
        obj->_oldLayerRect.y = 0;
        obj->_oldLayerRect.w = 0;
        obj->_oldLayerRect.h = 0;

    }
}

bool LayerMgr::CollisionWith(const BaseRect *rect, int32 skip /* = 1 */, uint8 flags /* = LCF_ALL */) const
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

class RayCastCheckDo
{
public:
    RayCastCheckDo(const CollisionMap& cm, LayerCollisionFlag lcf ) : _cm(cm), _lcf(lcf) {}

    inline bool operator() (int32 x, int32 y) const
    {
    #ifdef DEBUG_RAYCAST_VIS
        bool b = _cm(x, y) & _lcf;
        if(!b)
        {
            Engine::GetInstance()->GetCamera().TranslatePoints(x, y);
            SDLfunc_putpixel_safe(Engine::GetInstance()->GetSurface(), x, y, 0x20FFFFFF);
        }
        return b;
    #else
        return _cm(x, y) & _lcf;
    #endif
    }

private:
    const CollisionMap& _cm;
    const LayerCollisionFlag _lcf;
};

bool LayerMgr::CastRayAbs(const Vector2di src, const Vector2di& targ, Vector2di *lastpos, Vector2di *collpos, LayerCollisionFlag lcf /* = LCF_ALL */) const
{
    RayCastCheckDo check(_collisionMap, lcf);
    return CastBresenhamLine(src.x, src.y, targ.x, targ.y, lastpos, collpos, check);
}

bool LayerMgr::CastRayDir(const Vector2di src, const Vector2di& dir, Vector2di *lastpos, Vector2di *collpos, LayerCollisionFlag lcf /* = LCF_ALL */) const
{
    if(CastRayAbs(src, src + dir, lastpos, collpos, lcf))
    {
        if(lastpos)
            *lastpos -= src;
        if(collpos)
            *collpos -= src;
        return true;
    }
    return false;
}

// increase granularity for faster speed (if you know what you're doing!)
// WARNING: the code is weird, but it works (somehow)
// FIXME: the corner pixel is currently MISSING! Will do this later.
bool LayerMgr::CastRaysFromRect(const BaseRect& srcRect, const Vector2di& dir, Vector2di *lastpos, Vector2di *collpos,
                                LayerCollisionFlag lcf /* = LCF_ALL */) const
{
    Vector2di srcpos((int32)srcRect.x, (int32)srcRect.y);
    int32 w = int32(srcRect.w);
    int32 h = int32(srcRect.h);
    bool collided = false;

    int32 maxnear = dir.lensq();
    Vector2di lastp, collp;

    Vector2di offs;
    Vector2di start;

    const int32 granularity = 1; // FIXME: make this function param again?

    // follow outer border on X axis
    if(dir.y)
    {
        Vector2di border(0, dir.y < 0 ? -1 : 0); // Y axis correction
        start = srcpos + border;
        offs.x = 0;
        offs.y = dir.y > 0 ? h: 0;
        for( ; offs.x < w; offs.x += granularity)
        {
            Vector2di cast = start + offs;

            if(CastRayDir(cast, dir, &lastp, &collp, lcf))
            {
                collided = true;
                int32 l = lastp.lensq();
                if(l < maxnear)
                {
                    maxnear = l;
                    if(lastpos)
                        *lastpos = lastp;
                    if(collpos)
                        *collpos = collp;
                }
                #ifdef DEBUG_RAYCAST_VIS
                 SDLfunc_putpixel_safe(_engine->GetSurface(), cast.x, cast.y, 0xFF00FF00);
                #endif
            }
            #ifdef DEBUG_RAYCAST_VIS
             else SDLfunc_putpixel_safe(_engine->GetSurface(), cast.x, cast.y, 0xFFFF00FF);
            #endif
        }
    }

    // follow outer border on Y axis
    if(dir.x)
    {
        Vector2di border(dir.x < 0 ? -1 : 0, 0); // X axis correction
        start = srcpos + border;
        offs.y = 0;
        offs.x = dir.x > 0 ? w : 0;
        for( ; offs.y < h; offs.y += granularity)
        {
            Vector2di cast = start + offs;
            if(CastRayDir(cast, dir, &lastp, &collp, lcf))
            {
                collided = true;
                int32 l = lastp.lensq();
                if(l < maxnear)
                {
                    maxnear = l;
                    if(lastpos)
                        *lastpos = lastp;
                    if(collpos)
                        *collpos = collp;
                }
                #ifdef DEBUG_RAYCAST_VIS
                 SDLfunc_putpixel_safe(_engine->GetSurface(), cast.x, cast.y, 0xFF00FF00);
                #endif
            }
            #ifdef DEBUG_RAYCAST_VIS
             else SDLfunc_putpixel_safe(_engine->GetSurface(), cast.x, cast.y, 0xFFFF00FF);
            #endif
        }
    }

    return collided;
}

// TODO: deprecate
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

